#include "Utils/ImGuiUtils.h"

#include "..\3rdParty\imgui\imgui_internal.h"
#include "..\3rdParty\imgui\imgui.h"

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

static inline ImVec2  operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2  operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

bool dev::TextAligned(const char* _text, const ImVec2& _aligment)
{
	const ImVec2 size_arg = ImVec2(-FLT_MIN, 0.0f);

	auto window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return false;

	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;

	const ImGuiID id = window->GetID(_text);
	const ImVec2 label_size = ImGui::CalcTextSize(_text, NULL, true);

	ImVec2 pos = window->DC.CursorPos;

	ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);

	if (!ImGui::ItemAdd(bb, id)) return false;


	ImGui::RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, _text, NULL, &label_size, _aligment, &bb);
}