#include "ImGuiUtils.h"

#include "..\3rdParty\imgui\imgui_internal.h"
#include "..\3rdParty\imgui\imgui.h"
#include "..\3rdParty\imgui\misc\cpp\imgui_stdlib.h"

// Make the UI compact because there are so many fields
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

/*
void dev::DrawTextSelectableColored(const ImVec4& col, const std::string& _text)
{
	ImVec2 text_size = ImGui::CalcTextSize(_text.c_str(), _text.c_str() + _text.size());
	text_size.x = -FLT_MIN; // fill width (suppresses label)
	text_size.y += ImGui::GetStyle().FramePadding.y; // single pad

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 }); // make align with text height
	ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.f, 0.f, 0.f, 0.f }); // remove text input box

	ImGui::PushStyleColor(ImGuiCol_Text, col);


	ImGui::InputTextMultiline(
		"",
		const_cast<char*>(_text.c_str()), // ugly const cast
		_text.size() + 1, // needs to include '\0'
		text_size,
		ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll
	);
	ImGui::PopStyleColor();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
*/
/*
template <typename... Args>
void dev::DrawTextSelectableColored(const ImVec4& col, const std::string& _fmt, Args&&... args)
{
	std::string text = std::vformat(_fmt, std::make_format_args(args...));

	ImVec2 text_size = ImGui::CalcTextSize(text.c_str(), text.c_str() + text.size());
	text_size.x = -FLT_MIN; // fill width (suppresses label)
	text_size.y += ImGui::GetStyle().FramePadding.y; // single pad

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 }); // make align with text height
	ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.f, 0.f, 0.f, 0.f }); // remove text input box

	ImGui::PushStyleColor(ImGuiCol_Text, col);


	ImGui::InputTextMultiline(
		"",
		const_cast<char*>(text.c_str()), // ugly const cast
		text.size() + 1, // needs to include '\0'
		text_size,
		ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll
	);
	ImGui::PopStyleColor();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
*/
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


	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, _text, NULL, &label_size, _aligment, &bb);
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

void dev::DrawBreakpoint(const Breakpoint::Status _status, const float _dpiScale, const float _posXOffset, const bool _itemHasSize)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

	constexpr ImU32 DISASM_TBL_COLOR_BREAKPOINT = dev::IM_U32(0xFF2828C0);
	constexpr ImU32 DISASM_TBL_COLOR_BREAKPOINT_HOVER = dev::IM_U32(0xFF5538FF);
	constexpr ImU32 DISASM_TBL_COLOR_BREAKPOINT_DISABLED = dev::IM_U32(0x606060FF);
	static constexpr float DISASM_TBL_BREAKPOINT_SIZE = 0.35f;
	static constexpr float DISASM_TBL_BREAKPOINT_SIZE_HOVERED = 0.40f;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const ImVec2 totalSize = ImVec2(g.FontSize, g.FontSize);
	ImVec2 pos = window->DC.CursorPos;
	pos.x += _posXOffset * g.FontSize;
    pos.y += window->DC.CurrLineTextBaseOffset;
    const ImRect bb(pos, pos + totalSize);
	auto hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);

    if (_status == Breakpoint::Status::DELETED && !hovered) return;

	ImU32 color = hovered ? DISASM_TBL_COLOR_BREAKPOINT_HOVER :
		_status == Breakpoint::Status::ACTIVE ? DISASM_TBL_COLOR_BREAKPOINT : DISASM_TBL_COLOR_BREAKPOINT_DISABLED;

	float scale = hovered ? DISASM_TBL_BREAKPOINT_SIZE_HOVERED : DISASM_TBL_BREAKPOINT_SIZE;

	ImGui::ItemSize(_itemHasSize ? totalSize : ImVec2{}, 0.0f);
    if (!ImGui::ItemAdd(bb, 0))
        return;

    // Render
    auto drawPos = bb.Min + ImVec2(style.FramePadding.x + g.FontSize * 0.5f, g.FontSize * 0.5f);
    window->DrawList->AddCircleFilled(drawPos, g.FontSize * scale * _dpiScale, color, 8);
}
/*
void dev::HardwareStatsWindow::DrawProperty1(const std::string& _name, const std::string& _value, const ImVec2& _aligment)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();

	ImGui::SameLine();
	TextAligned(_value.c_str(), _aligment);
}
*/
void dev::DrawProperty2(const std::string& _name, const std::string& _value)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	TextAligned(_value.c_str(), { 0.0f, 0.5f });
}

void dev::DrawSeparator2(const std::string& _text)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::SeparatorText(_text.c_str());
	ImGui::TableNextColumn();
	ImGui::SeparatorText("");
}

void dev::DrawProperty2EditableS(const char* _name, const char* _label, std::string* _value, const char* _hint)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::InputTextWithHint(_label, _hint, _value);
}


void dev::DrawProperty2EditableCheckBox(const char* _name, const char* _label, bool* _val)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Checkbox(_label, _val);
}