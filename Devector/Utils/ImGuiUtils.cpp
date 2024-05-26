#include "ImGuiUtils.h"

#include "Utils.h"
#include "StrUtils.h"

#include "imgui_internal.h"
#include "imgui.h"
#include "misc\cpp\imgui_stdlib.h"

// Make the UI compact because there are so many fields
// TODO: check it is not over extensively used
void dev::PushStyleCompact(const float _paddingMulX, const float _paddingMulY)
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x * _paddingMulX, style.FramePadding.y * _paddingMulY));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * _paddingMulX, style.ItemSpacing.y * _paddingMulY));
}

void dev::PopStyleCompact()
{
	ImGui::PopStyleVar(2);
}

void dev::UpdatePropertyPrintStat(const char* _parameterName)
{
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::Text(_parameterName);
	ImGui::TableSetColumnIndex(1);
}

void dev::ColumnClippingEnable(const float _dpiScale)
{
	ImGuiContext& g = *GImGui;

	ImGui::PushClipRect(
		ImGui::GetCursorScreenPos(),
		ImVec2(
			ImGui::GetCursorScreenPos().x + ImGui::GetColumnWidth(),
			ImGui::GetCursorScreenPos().y + g.FontSize * _dpiScale
		), true);
}

void dev::ColumnClippingDisable()
{
	ImGui::PopClipRect();
}

void dev::DrawTextSelectable(const char* _label, const std::string& _text)
{
	ImVec2 text_size = ImGui::CalcTextSize(_text.c_str(), _text.c_str() + _text.size());
	text_size.x = -FLT_MIN; // fill width (suppresses label)
	text_size.y += ImGui::GetStyle().FramePadding.y; // single pad

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 }); // make align with text height
	ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.f, 0.f, 0.f, 0.f }); // remove text input box

	ImGui::InputTextMultiline(
		"",
		const_cast<char*>(_text.c_str()), // ugly const cast
		_text.size() + 1, // needs to include '\0'
		text_size,
		ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll
	);

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void dev::DrawHelpMarker(const char* _text)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(_text);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void dev::DrawPopup(const char* _title, const char* _text)
{
	ImGui::OpenPopup(_title);

	if (ImGui::BeginPopupModal(_title, NULL))
	{
		ImGui::Text(_text);
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

void dev::TextAligned(const char* _text, const ImVec2& _aligment)
{
	const ImVec2 size_arg = ImVec2(-FLT_MIN, 0.0f);

	auto window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return;

	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;

	const ImGuiID id = window->GetID(_text);
	const ImVec2 label_size = ImGui::CalcTextSize(_text, NULL, true);

	ImVec2 pos = window->DC.CursorPos;

	ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);

	if (!ImGui::ItemAdd(bb, id)) return;


	ImGui::RenderTextClipped(bb.Min + style.FramePadding, 
		bb.Max - style.FramePadding, _text, NULL, &label_size, _aligment, &bb);
}

void dev::DrawProgramCounter(const ImU32 _color, const ImGuiDir _dir, const float _dpiScale, const float _posXOffset, const bool _itemHasSize)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	const auto totalSize = ImVec2{g.FontSize, g.FontSize};
	ImVec2 pos = window->DC.CursorPos;
	pos.x += _posXOffset * g.FontSize;
	pos.y += window->DC.CurrLineTextBaseOffset;
	ImGui::ItemSize(_itemHasSize ? totalSize : ImVec2{}, 0.0f);
	const ImRect bb(pos, pos + totalSize);

	if (!ImGui::ItemAdd(bb, 0))
		return;
	
	// Render
	auto drawPos = bb.Min + ImVec2(style.FramePadding.x, 0.0f);
	ImGui::RenderArrow(window->DrawList, drawPos, _color, _dir);
}

bool dev::DrawBreakpoint(const char* label, Breakpoint::Status* _statusP, const float _dpiScale, const float _posXOffset, const bool _itemHasSize)
{
	constexpr ImU32 DISASM_TBL_COLOR_BREAKPOINT = dev::IM_U32(0xFF2828A0);
	constexpr ImU32 DISASM_TBL_COLOR_BREAKPOINT_HOVER = dev::IM_U32(0xFFD010C0);
	constexpr ImU32 DISASM_TBL_COLOR_BREAKPOINT_DISABLED = dev::IM_U32(0x656565D0);
	static constexpr float DISASM_TBL_BREAKPOINT_SIZE = 0.35f;
	static constexpr float DISASM_TBL_BREAKPOINT_SIZE_HOVERED = 0.40f;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
	
    const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

    const ImVec2 totalSize = ImVec2(g.FontSize, g.FontSize);
	ImVec2 pos = window->DC.CursorPos;
	pos.x += _posXOffset * g.FontSize;
    pos.y += window->DC.CurrLineTextBaseOffset;
    const ImRect bb(pos, pos + totalSize);

	ImGui::ItemSize(_itemHasSize ? totalSize : ImVec2{}, 0.0f);
    if (!ImGui::ItemAdd(bb, 0))
        return false;

	auto hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
	bool pressed = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
	bool ctrlPressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

	if (hovered && pressed)
	{
		if (*_statusP == Breakpoint::Status::DELETED){
			*_statusP = Breakpoint::Status::ACTIVE;
		}
		else if (*_statusP == Breakpoint::Status::DISABLED){
			*_statusP = Breakpoint::Status::ACTIVE;
		}
		else if (*_statusP == Breakpoint::Status::ACTIVE) 
		{
			if (ctrlPressed) {
				*_statusP = Breakpoint::Status::DISABLED;
			}
			else {
				*_statusP = Breakpoint::Status::DELETED;
			}
		}
			
		
		ImGui::MarkItemEdited(id);
	}

	ImU32 color = *_statusP == Breakpoint::Status::ACTIVE ? DISASM_TBL_COLOR_BREAKPOINT : DISASM_TBL_COLOR_BREAKPOINT_DISABLED;
    auto drawPos = bb.Min + ImVec2(style.FramePadding.x + g.FontSize * 0.5f, g.FontSize * 0.5f);

	// render the hover highlight
	if (hovered)
		window->DrawList->AddCircleFilled(drawPos, g.FontSize * DISASM_TBL_BREAKPOINT_SIZE_HOVERED * _dpiScale, DISASM_TBL_COLOR_BREAKPOINT_HOVER, 8);

	// render the breakpoint
	if (*_statusP != Breakpoint::Status::DELETED)
		window->DrawList->AddCircleFilled(drawPos, g.FontSize * DISASM_TBL_BREAKPOINT_SIZE * _dpiScale, color, 8);

	return pressed && hovered;
}

void dev::DrawProperty2(const std::string& _name, const std::string& _value, const std::string& _hint)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Dummy(ImVec2(5.0f, 0.0f)); ImGui::SameLine();
	ImGui::Text(_value.c_str());

	if (!_hint.empty()) {
		ImGui::SameLine();
		dev::DrawHelpMarker(_hint.c_str());
	}
}

void dev::DrawSeparator2(const std::string& _text)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::SeparatorText(_text.c_str());
	ImGui::TableNextColumn();
	ImGui::SeparatorText("");
}

void dev::DrawProperty2EditableI(const char* _name, const char* _label, int* _value, 
	const char* _help)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::InputInt(_label, _value);
	if (*_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({12,10});
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}
	*_value = dev::Max(1, *_value);
}

void dev::DrawProperty2EditableS(const char* _name, const char* _label, std::string* _value, 
	const char* _hint, const char* _help, const ImGuiInputTextFlags _flags)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::InputTextWithHint(_label, _hint, _value, _flags);
	if (*_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({12,10});
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}
}

void dev::DrawProperty2EditableCheckBox(const char* _name, const char* _label, 
	bool* _val, const char* _help)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Checkbox(_label, _val);

	if (*_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({ 80,10 });
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}
}

int dev::DrawCodeLine(const bool _tab, const bool _isRunning, const Debugger::DisasmLine& _line,
	std::function<void(const Addr _addr)> _onMouseLeft,
	std::function<void(const Addr _addr)> _onMouseRight)
{
	int addrHighlighted = -1;
	auto cmd_splitted = dev::Split(_line.str, ' ');
	int i = 0;
	for (const auto& cmd_parts : cmd_splitted)
	{
		if (i == 0)
		{
			// draw a mnenonic
			if (_tab){
				ImGui::TextColored(DISASM_TBL_COLOR_MNEMONIC, "\t%s ", cmd_parts.c_str());
			}
			else {
				ImGui::TextColored(DISASM_TBL_COLOR_MNEMONIC, "%s ", cmd_parts.c_str());
			}
		}
		else
		{
			// draw an operand separator
			if (i == 2)
			{
				ImGui::SameLine();
				ImGui::TextColored(DISASM_TBL_COLOR_NUMBER, ", ");
			}

			// draw an operand
			auto operands = dev::Split(cmd_parts, ';');

			int operandIdx = 0;
			for (const auto& operand : operands)
			{
				if (operand[0] == '0' && operands.size() == 1)
				{
					// check if the hexadecimal literal is hovered
					ImGui::SameLine();
					ImVec4 addrColor = DISASM_TBL_COLOR_NUMBER;
					if (!_isRunning &&
						!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
					{
						ImVec2 textPos = ImGui::GetCursorScreenPos();
						ImVec2 textSize = ImGui::CalcTextSize(operand.c_str());
						if (ImGui::IsMouseHoveringRect(textPos, ImVec2(textPos.x + textSize.x, textPos.y + textSize.y)))
						{
							ImGui::GetWindowDrawList()->AddRectFilled(textPos, ImVec2(textPos.x + textSize.x, textPos.y + textSize.y), IM_COL32(100, 10, 150, 255));

							Addr reqUpdateAddr = dev::StrHexToInt(operand.c_str() + 2);
							addrHighlighted = reqUpdateAddr;
							// if it's clicked, scroll the disasm to highlighted addr
							if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
							{
								_onMouseLeft(reqUpdateAddr);
							}

							if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
							{
								_onMouseRight(reqUpdateAddr);
							}

							// highlighted text is white
							addrColor.x = addrColor.y = addrColor.z = addrColor.w = 1.0f;
						}
					}
					// draw a hexadecimal literal
					ImGui::TextColored(addrColor, "%s", operand.c_str());
				}
				else if (cmd_parts.size() <= 2 || cmd_parts == "PSW")
				{
					// draw a reg
					ImGui::SameLine();
					ImGui::TextColored(DISASM_TBL_COLOR_REG, "%s", operand.c_str());
				}
				else
				{
					// draw a const value
					if (operand[0] == '0') {
						ImGui::SameLine();
						ImGui::TextColored(DISASM_TBL_COLOR_COMMENT, ";%s", operand.c_str());
					}
					else {
						// draw a const
						ImGui::SameLine();
						ImGui::TextColored(DISASM_TBL_COLOR_CONST, "%s ", operand.c_str());
					}
				}
				operandIdx++;
			}
		}
		i++;
	}
	return addrHighlighted;
}

void dev::DrawAddr(const bool _isRunning, const Debugger::DisasmLine& _disasmLine, 
			uint8_t _highlightAlpha,
			std::function<void()> _onMouseLeft,
			std::function<void()> _onMouseRight)
{
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, DISASM_TBL_BG_COLOR_ADDR);

	bool drawNormalLiteral = true;
	if (!_isRunning &&
		!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
	{
		ImVec2 textPos = ImGui::GetCursorScreenPos();
		ImVec2 textSize = ImGui::CalcTextSize(_disasmLine.addrS.c_str());
		if ((_highlightAlpha) ||
			ImGui::IsMouseHoveringRect(textPos, ImVec2(textPos.x + textSize.x, textPos.y + textSize.y)))
		{
			ImGui::GetWindowDrawList()->AddRectFilled(textPos, ImVec2(textPos.x + textSize.x, textPos.y + textSize.y), IM_COL32(100, 10, 150, _highlightAlpha));
			// draw a highlighted hexadecimal literal
			ImGui::TextColored(dev::IM_VEC4(0xFFFFFFFF), "%s", _disasmLine.addrS.c_str());
			
			// if it's clicked, scroll the disasm to highlighted addr
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				_onMouseLeft();
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				_onMouseRight();
			}

			drawNormalLiteral = false;
		}
	}
	if (drawNormalLiteral)
	{
		ImGui::TextColored(DISASM_TBL_COLOR_LABEL_MINOR, _disasmLine.addrS.c_str());
	}
}

auto dev::DrawSaveDiscardFddPopup(int _selectedDriveIdx, std::wstring _mountedFddPath)
-> FddStatus
{
	auto out = FddStatus::NONE;

	// Dialog. Save or Discard mounted updated fdd image?
	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window when appearing
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Save or Discard", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static const char* diskNames[] = { "A", "B", "C", "D" };
		ImGui::Text("Previously mounted disk %s was updated\nSave or discard changes?", diskNames[_selectedDriveIdx]);
		ImGui::SameLine();
		dev::DrawHelpMarker(dev::StrWToStr(_mountedFddPath).c_str());

		ImGui::NewLine();
		static bool doNotAskAgain = false;
		ImGui::Checkbox("##oldFddDoNotAskAgain", &doNotAskAgain);
		ImGui::SameLine();
		ImGui::Text("Don't ask again");
		ImGui::Separator();

		if (ImGui::Button("Save", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			// save settings
			if (doNotAskAgain) {
				out = FddStatus::ALWAYS_SAVE;
			}
			out = FddStatus::SAVE;
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Discard", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
			// save settings
			if (doNotAskAgain) {
				out = FddStatus::ALWAYS_DISCARD;
			}
			out = FddStatus::DISCARD;
		}
		ImGui::EndPopup();
	}

	return out;
}