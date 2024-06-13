#include "ImGuiUtils.h"

#include "Utils.h"
#include "StrUtils.h"
#include "Core/Disasm.h"
#include "misc\cpp\imgui_stdlib.h"

#include "imgui_internal.h"

// Make the UI compact because there are so many fields
// TODO: check if it's not over extensively used
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
	pos.x += _posXOffset;
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

auto dev::DrawAddr(const bool _isRunning,
	const char* _operandS, const ImVec4& _color, 
	const ImVec4& _highlightColor, bool _forceHighlight)
-> UIItemMouseAction
{
	ImVec2 textPos;
	ImVec2 textSize;

	auto mouseAction = UIItemMouseAction::NONE;
	if (!_isRunning &&
		!ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
	{
		textPos = ImGui::GetCursorScreenPos();
		textSize = ImGui::CalcTextSize(_operandS);

		if (ImGui::IsMouseHoveringRect(textPos, ImVec2(textPos.x + textSize.x, textPos.y + textSize.y)))
		{	
			mouseAction = UIItemMouseAction::HOVERED;
			_forceHighlight = true;

			// if it's clicked, scroll the disasm to highlighted addr
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) mouseAction = UIItemMouseAction::LEFT;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) mouseAction = UIItemMouseAction::RIGHT;
		}
	}

	// draw a highlight box
	if (_forceHighlight)
	{
		ImGui::GetWindowDrawList()->
			AddRectFilled(textPos, ImVec2(textPos.x + textSize.x, textPos.y + textSize.y), DASM_BG_CLR_ADDR_HIGHLIGHT);
	}
	// draw a hexadecimal literal
	ImGui::TextColored(_forceHighlight ? _highlightColor : _color, _operandS);

	return mouseAction;
}

auto dev::DrawCodeLine(const bool _isRunning, const Disasm::Line& _line, const bool _tab)
-> UIItemMouseAction
{
	auto uiItemMouseAction = UIItemMouseAction::NONE;
	auto opcode = _line.opcode;
	auto mnemonic = dev::GetMnemonic(opcode);
	auto mnemonicLen = dev::GetMnemonicLen(opcode);
	auto mnemonicType = dev::GetMnemonicType(opcode);
	auto immType = GetImmediateType(opcode);

	for (int i = 0; i < mnemonicLen; i++)
	{
		const char* str = nullptr;
		const ImVec4* colorP;

		switch (mnemonicType[i])
		{
		case MNT_CMD:
		{
			// draw a mnenonic
			str = _tab ? "\t%s" : "%s";

			switch (GetOpcodeType(opcode))
			{
			case OPTYPE_C__:
			case OPTYPE_CAL:
				colorP = &DASM_CLR_MNEMONIC_BRANCH;
				break;
			case OPTYPE_J__:
			case OPTYPE_JMP:
				colorP = &DASM_CLR_MNEMONIC_BRANCH;
				break;
			case OPTYPE_R__:
			case OPTYPE_RET:
				colorP = &DASM_CLR_MNEMONIC_BRANCH;
				break;
			default:
				colorP = &DASM_CLR_MNEMONIC;
				break;
			}
			break;
		}
		case MNT_IMM:
			ImGui::SameLine();
			colorP = &DASM_CLR_CONST;
			str = " %s";
			break;

		case MNT_REG:
			ImGui::SameLine();
			colorP = &DASM_CLR_REG;
			str = " %s";
			break;

		default:
			colorP = &DASM_CLR_COMMENT;
			str = " %s";
			break;
		}
		ImGui::TextColored(*colorP, str, mnemonic[i]);

		// draw an operand separator
		if (i == 1 && (mnemonicLen == 3 || immType == CMD_IB_OFF1 || immType == CMD_IW_OFF1)) {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_NUMBER, ",");
		}
	}
	// print an immediate operand
	if (immType != CMD_IM_NONE)
	{
		ImGui::SameLine();
		ImGui::Text(" "); ImGui::SameLine();

		const char* operand = _line.GetFirstConst();
		const ImVec4* color = &DASM_CLR_CONST;

		if (immType == CMD_IW_OFF1 && _line.labels) 
		{
				operand = _line.GetFirstLabel();
				color = operand[0] == '@' ? &DASM_CLR_LABEL_LOCAL_IMM  : &DASM_CLR_LABEL_GLOBAL_IMM;
		}
		bool immLabel = operand != nullptr;

		color = operand ? color : &DASM_CLR_NUMBER;
		operand = operand ? operand : _line.GetImmediateS();
		uiItemMouseAction = DrawAddr(_isRunning, operand, *color, DASM_CLR_NUMBER_HIGHLIGHT, 0);
		
		if (immLabel && uiItemMouseAction != UIItemMouseAction::NONE) {
			ImGui::BeginTooltip();
			ImGui::Text("Address: 0x%04X\n", _line.imm);
			ImGui::EndTooltip();
		}
	}

	return uiItemMouseAction;
}
