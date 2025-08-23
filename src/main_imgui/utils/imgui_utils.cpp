#include "utils/imgui_utils.h"

#include "utils/utils.h"
#include "utils/str_utils.h"
#include "core/disasm.h"
#include "imgui_stdlib.h"

#include "imgui_internal.h"

// Make the UI compact because there are so many fields
// TODO: check if it's not over extensively used
void dev::PushStyleCompact(const float _paddingMulX, const float _paddingMulY)
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::PushStyleVar(
		ImGuiStyleVar_FramePadding,
		ImVec2(style.FramePadding.x * _paddingMulX,
				style.FramePadding.y * _paddingMulY));

	ImGui::PushStyleVar(
		ImGuiStyleVar_ItemSpacing,
		ImVec2(style.ItemSpacing.x * _paddingMulX,
				style.ItemSpacing.y * _paddingMulY));
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
	ImVec2 text_size = ImGui::CalcTextSize(
		_text.c_str(), _text.c_str() + _text.size());
	text_size.x = -FLT_MIN; // fill width (suppresses label)
	text_size.y += ImGui::GetStyle().FramePadding.y; // single pad

	ImGui::PushStyleVar(
		ImGuiStyleVar_FramePadding, { 0, 0 }); // make align with text height
	ImGui::PushStyleColor(
		ImGuiCol_FrameBg, { 0.f, 0.f, 0.f, 0.f }); // remove text input box

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

bool dev::HyperLink(const char* label, bool underlineWhenHoveredOnly)
{
	const ImU32 linkColor = CLR_URL;
	const ImU32 linkHoverColor = CLR_URL_HOVERED;
	const ImU32 linkFocusColor = CLR_URL_HOVERED;

	const ImGuiID id = ImGui::GetID(label);

	ImGuiWindow* const window = ImGui::GetCurrentWindow();
	ImDrawList* const draw = ImGui::GetWindowDrawList();

	const ImVec2 pos(
		window->DC.CursorPos.x,
		window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
	const ImVec2 size = ImGui::CalcTextSize(label);
	ImRect bb(pos, { pos.x + size.x, pos.y + size.y });

	ImGui::ItemSize(bb, 0.0f);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	bool isHovered = false;
	const bool isClicked = ImGui::ButtonBehavior(bb, id, &isHovered, nullptr);
	const bool isFocused = ImGui::IsItemFocused();

	const ImU32 color = isHovered ?
		linkHoverColor :
		isFocused ? linkFocusColor : linkColor;

	draw->AddText(bb.Min, color, label);

	if (isFocused)
		draw->AddRect(bb.Min, bb.Max, color);
	else if (!underlineWhenHoveredOnly || isHovered)
		draw->AddLine({ bb.Min.x, bb.Max.y }, bb.Max, color);

	return isClicked;
}


static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{
	return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}


static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)
{
	return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}


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

	ImVec2 size = ImGui::CalcItemSize(
		size_arg,
		label_size.x + style.FramePadding.x * 2.0f,
		label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);

	if (!ImGui::ItemAdd(bb, id)) return;


	ImGui::RenderTextClipped(bb.Min + style.FramePadding,
		bb.Max - style.FramePadding, _text, NULL, &label_size, _aligment, &bb);
}

void dev::DrawProgramCounter(
	const ImU32 _color, const ImGuiDir _dir,
	const float _dpiScale, const float _posXOffset, const bool _itemHasSize)
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

bool dev::DrawBreakpoint(
	const char* label,
	Breakpoint::Status* _statusP,
	const float _dpiScale,
	const float _posXOffset, const bool _itemHasSize)
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

	ImU32 color = *_statusP == Breakpoint::Status::ACTIVE ?
		DISASM_TBL_COLOR_BREAKPOINT :
		DISASM_TBL_COLOR_BREAKPOINT_DISABLED;

	auto drawPos = bb.Min +
		ImVec2(style.FramePadding.x + g.FontSize * 0.5f, g.FontSize * 0.5f);

	// render the hover highlight
	if (hovered)
		window->DrawList->AddCircleFilled(drawPos,
			g.FontSize * DISASM_TBL_BREAKPOINT_SIZE_HOVERED * _dpiScale,
			DISASM_TBL_COLOR_BREAKPOINT_HOVER, 8);

	// render the breakpoint
	if (*_statusP != Breakpoint::Status::DELETED)
		window->DrawList->AddCircleFilled(
			drawPos,
			g.FontSize * DISASM_TBL_BREAKPOINT_SIZE * _dpiScale, color, 8);

	return pressed && hovered;
}

void dev::DrawProperty2(
	const char* _name, const char* _value,
	const char* _hint, const ImVec4& _valColor)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Dummy(ImVec2(5.0f, 0.0f)); ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, _valColor);
	ImGui::Text(_value);
	ImGui::PopStyleColor();

	if (_hint) {
		ImGui::SameLine();
		dev::DrawHelpMarker(_hint);
	}
}

void dev::DrawProperty2RegPair(
	const char* _name,
	const char* _valueH, const char* _valueL,
	const char* _hint,
	const ImVec4& _valHColor, const ImVec4& _valLColor)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Dummy(ImVec2(5.0f, 0.0f)); ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, _valHColor);
	ImGui::Text(_valueH);
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_Text, _valLColor);
	ImGui::SameLine();
	ImGui::Text(_valueL);
	ImGui::PopStyleColor();

	if (_hint) {
		ImGui::SameLine();
		dev::DrawHelpMarker(_hint);
	}
}

void dev::DrawSeparator2(const char* _text)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::SeparatorText(_text);
	ImGui::TableNextColumn();
	ImGui::SeparatorText("");
}

bool dev::DrawProperty2EditableI(
	const char* _name, const char* _label, int* _value,
	const char* _help, const ImGuiInputTextFlags _flags)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	bool valEntered = ImGui::InputInt(_label, _value, 1, 100, _flags);
	if (_help && *_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({12,10});
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}

	return valEntered;
}

bool dev::DrawProperty2EditableS(
	const char* _name, const char* _label, std::string* _value,
	const char* _hint, const char* _help,
	const ImGuiInputTextFlags _flags, bool* _delButtonPressed)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	auto entered = ImGui::InputTextWithHint(_label, _hint, _value, _flags);

	// Delete button
	if (_delButtonPressed)
	{
		ImGui::SameLine(); ImGui::SameLine();
		if (ImGui::Button(std::format("Delete##{}", _name).c_str()))
		{
			_value->erase();
			*_delButtonPressed = true;
		}
		else
		{
			*_delButtonPressed = false;
		}
	}

	// hint
	if (_help && *_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy(UI_LITTLE_SPACE);
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}

	return entered;
}

void dev::DrawProperty2Combo(
	const char* _name, const char* _label,
	int* _currentItem, const char* const _items[],
	int _itemsCount, const char* _help)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Combo(_label, _currentItem, _items, _itemsCount);
	if (_help && *_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({ 12,10 });
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}
}

void dev::DrawProperty2EditableCheckBox(
	const char* _name, const char* _label,
	bool* _val, const char* _help)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Checkbox(_label, _val);

	if (_help && *_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({ 80,10 });
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}
}

// draws four checkbox in a line
void dev::DrawProperty2EditableCheckBox4(
	const char* _name,
	const char* _label0, const char* _label1,
	const char* _label2, const char* _label3,
	bool* _val0, bool* _val1, bool* _val2, bool* _val3, const char* _help)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Checkbox(_label0, _val0); ImGui::SameLine();
	ImGui::Checkbox(_label1, _val1); ImGui::SameLine();
	ImGui::Checkbox(_label2, _val2); ImGui::SameLine();
	ImGui::Checkbox(_label3, _val3);

	if (_help && *_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({ 8, 10 });
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}
}

void dev::DrawProperty2RadioButtons(
	const char* _name, int* _currentItem,
	const char* const _items[], int _itemsCount, const float _space,
	const char* _help)
{
	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();

	for (int i = 0; i < _itemsCount; i++)
	{
		ImGui::RadioButton(_items[i], _currentItem, i);
		if (i != _itemsCount - 1)
		{
			ImGui::SameLine();
			ImGui::Dummy(ImVec2{ _space, 5.0f });
			ImGui::SameLine();
		}
	}

	if (_help && *_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({ 12,10 });
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}
}

auto dev::DrawPropertyMemoryMapping(Breakpoint::MemPages _memPages)
-> Breakpoint::MemPages
{
	// mapping
	bool ram = _memPages.ram;
	bool rd00 = _memPages.rdisk0page0;
	bool rd01 = _memPages.rdisk0page1;
	bool rd02 = _memPages.rdisk0page2;
	bool rd03 = _memPages.rdisk0page3;
	bool rd10 = _memPages.rdisk1page0;
	bool rd11 = _memPages.rdisk1page1;
	bool rd12 = _memPages.rdisk1page2;
	bool rd13 = _memPages.rdisk1page3;
	bool rd20 = _memPages.rdisk2page0;
	bool rd21 = _memPages.rdisk2page1;
	bool rd22 = _memPages.rdisk2page2;
	bool rd23 = _memPages.rdisk2page3;
	bool rd30 = _memPages.rdisk3page0;
	bool rd31 = _memPages.rdisk3page1;
	bool rd32 = _memPages.rdisk3page2;
	bool rd33 = _memPages.rdisk3page3;
	bool rd40 = _memPages.rdisk4page0;
	bool rd41 = _memPages.rdisk4page1;
	bool rd42 = _memPages.rdisk4page2;
	bool rd43 = _memPages.rdisk4page3;
	bool rd50 = _memPages.rdisk5page0;
	bool rd51 = _memPages.rdisk5page1;
	bool rd52 = _memPages.rdisk5page2;
	bool rd53 = _memPages.rdisk5page3;
	bool rd60 = _memPages.rdisk6page0;
	bool rd61 = _memPages.rdisk6page1;
	bool rd62 = _memPages.rdisk6page2;
	bool rd63 = _memPages.rdisk6page3;
	bool rd70 = _memPages.rdisk7page0;
	bool rd71 = _memPages.rdisk7page1;
	bool rd72 = _memPages.rdisk7page2;
	bool rd73 = _memPages.rdisk7page3;

	DrawProperty2EditableCheckBox(
		"Ram", "##BPContextAccessRam", &ram, "To check the main RAM");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 1", "##BPCARD0P0", "##BPCARD0P1", "##BPCARD0P2", "##BPCARD0P3",
		&rd00, &rd01, &rd02, &rd03, "To check the RAM Disk1 pages 0,1,2,3");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 2", "##BPCARD1P0", "##BPCARD1P1", "##BPCARD1P2", "##BPCARD1P3",
		&rd10, &rd11, &rd12, &rd13, "To check the RAM Disk2 pages 0,1,2,3");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 3", "##BPCARD2P0", "##BPCARD2P1", "##BPCARD2P2", "##BPCARD2P3",
		&rd20, &rd21, &rd22, &rd23, "To check the RAM Disk3 pages 0,1,2,3");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 4", "##BPCARD3P0", "##BPCARD3P1", "##BPCARD3P2", "##BPCARD3P3",
		&rd30, &rd31, &rd32, &rd33, "To check the RAM Disk4 pages 0,1,2,3");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 5", "##BPCARD4P0", "##BPCARD4P1", "##BPCARD4P2", "##BPCARD4P3",
		&rd40, &rd41, &rd42, &rd43, "To check the RAM Disk5 pages 0,1,2,3");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 6", "##BPCARD5P0", "##BPCARD5P1", "##BPCARD5P2", "##BPCARD5P3",
		&rd50, &rd51, &rd52, &rd53, "To check the RAM Disk6 pages 0,1,2,3");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 7", "##BPCARD6P0", "##BPCARD6P1", "##BPCARD6P2", "##BPCARD6P3",
		&rd60, &rd61, &rd62, &rd63, "To check the RAM Disk7 pages 0,1,2,3");
	DrawProperty2EditableCheckBox4(
		"Ram Disk 8", "##BPCARD7P0", "##BPCARD7P1", "##BPCARD7P2", "##BPCARD7P3",
		&rd70, &rd71, &rd72, &rd73, "To check the RAM Disk8 pages 0,1,2,3");

	_memPages.ram = ram;
	_memPages.rdisk0page0 = rd00;
	_memPages.rdisk0page1 = rd01;
	_memPages.rdisk0page2 = rd02;
	_memPages.rdisk0page3 = rd03;
	_memPages.rdisk1page0 = rd10;
	_memPages.rdisk1page1 = rd11;
	_memPages.rdisk1page2 = rd12;
	_memPages.rdisk1page3 = rd13;
	_memPages.rdisk2page0 = rd20;
	_memPages.rdisk2page1 = rd21;
	_memPages.rdisk2page2 = rd22;
	_memPages.rdisk2page3 = rd23;
	_memPages.rdisk3page0 = rd30;
	_memPages.rdisk3page1 = rd31;
	_memPages.rdisk3page2 = rd32;
	_memPages.rdisk3page3 = rd33;
	_memPages.rdisk4page0 = rd40;
	_memPages.rdisk4page1 = rd41;
	_memPages.rdisk4page2 = rd42;
	_memPages.rdisk4page3 = rd43;
	_memPages.rdisk5page0 = rd50;
	_memPages.rdisk5page1 = rd51;
	_memPages.rdisk5page2 = rd52;
	_memPages.rdisk5page3 = rd53;
	_memPages.rdisk6page0 = rd60;
	_memPages.rdisk6page1 = rd61;
	_memPages.rdisk6page2 = rd62;
	_memPages.rdisk6page3 = rd63;
	_memPages.rdisk7page0 = rd70;
	_memPages.rdisk7page1 = rd71;
	_memPages.rdisk7page2 = rd72;
	_memPages.rdisk7page3 = rd73;

	return _memPages;
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

		if (ImGui::IsMouseHoveringRect(textPos,
			ImVec2(textPos.x + textSize.x, textPos.y + textSize.y)))
		{
			mouseAction = UIItemMouseAction::HOVERED;
			_forceHighlight = true;

			// if it's clicked, scroll the disasm to highlighted addr
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
				mouseAction = UIItemMouseAction::LEFT;
			}
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				mouseAction = UIItemMouseAction::RIGHT;
			}
		}
	}

	// draw a highlight box
	if (_forceHighlight)
	{
		ImGui::GetWindowDrawList()->
			AddRectFilled(
				textPos,
				ImVec2(textPos.x + textSize.x, textPos.y + textSize.y),
				DASM_BG_CLR_ADDR_HIGHLIGHT);
	}
	// draw a hexadecimal literal
	ImGui::TextColored(_forceHighlight ? _highlightColor : _color, _operandS);

	return mouseAction;
}

// draws a tooltip during the time = _timer
// _timer > 0.0f sets the timer, _timer == 0.0 evaluates the timer
void dev::DrawTooltipTimer(const char* _text, const float _timer)
{
	static double timer = 0.0f;
	static char tooltip[256] = "";

	if (_text && _timer > 0.0f) {
		timer = _timer;
#if defined(_WIN32)
		strcpy_s(tooltip, 256, _text);
#else
		strcpy(tooltip, _text);
#endif
		return;
	}

	static auto lastTime = std::chrono::system_clock::now();
	using Tm = std::chrono::duration<double, std::milli>;
	Tm elapsedTime = std::chrono::system_clock::now() - lastTime;
	lastTime = std::chrono::system_clock::now();
	timer -= elapsedTime.count() / 1000.0f;

	if (timer <= 0.0f) return;

	ImGui::BeginTooltip();
	ImGui::Text(tooltip);
	ImGui::EndTooltip();
}

auto dev::DrawCodeLine(
	const bool _isRunning, const Disasm::Line& _line, const bool _tab)
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
		if (i == 1 &&
			(mnemonicLen == 3 ||
			immType == CMD_IB_OFF1 || immType == CMD_IW_OFF1))
		{
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_NUMBER, ",");
		}
	}
	// print an immediate operand
	if (immType != CMD_IM_NONE)
	{
		ImGui::SameLine();
		ImGui::Text(" "); ImGui::SameLine();

		auto operand = _line.GetFirstConst();
		const ImVec4* color = &DASM_CLR_CONST;

		if (immType == CMD_IW_OFF1 && !_line.labels.empty())
		{
				operand = _line.GetFirstLabel();
				color = operand.size() >= 1 && operand[0] == '@' ?
					&DASM_CLR_LABEL_LOCAL_IMM  :
					&DASM_CLR_LABEL_GLOBAL_IMM;
		}
		bool immLabel = !operand.empty();

		color = immLabel ? color : &DASM_CLR_NUMBER;
		operand = immLabel ? operand : _line.GetImmediateS();
		uiItemMouseAction = DrawAddr(
			_isRunning, operand.c_str(), *color, DASM_CLR_NUMBER_HIGHLIGHT, 0);

		if (immLabel && uiItemMouseAction != UIItemMouseAction::NONE) {
			ImGui::BeginTooltip();
			ImGui::Text("Address: 0x%04X\n", _line.imm);
			ImGui::EndTooltip();
		}
	}

	return uiItemMouseAction;
}

void dev::DrawDisasmConsts(const Disasm::Line& _line, const int _maxDisasmLabels)
{
	ImGui::TableNextColumn();

	int i = 0;
	for (const auto& const_ : _line.consts)
	{
		if (i) {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_COMMENT, ", ");
			ImGui::SameLine();
		}
		ImGui::TextColored(DASM_CLR_ADDR, const_.c_str());

		if (i++ == _maxDisasmLabels) {
			ImGui::SameLine();
			ImGui::TextColored(DASM_CLR_COMMENT, "...");
			break;
		}
	}
}

auto dev::DrawTransparentButtonWithBorder(
	const char* _label, const ImVec2& _pos,
	const ImVec2& _size, const char* _hint)
-> const ButtonAction
{
	ButtonAction out = ButtonAction::NONE;

	// Set a transparent background color for the button (RGBA with 0.0 alpha)
	// Transparent background
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	// Transparent when hovered
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0.3));
	// Transparent when clicked
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

	// Set a border color (e.g., white)
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White border

	// Ensure borders are enabled and set a border thickness
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);  // Enable border and set its size

	// Draw the button
	ImGui::SetCursorPos(_pos);

	if (ImGui::Button(_label, _size)) {
		out = ButtonAction::RELEASED;
	}

	// Detect if the button is being held
	if (ImGui::IsItemActivated()){
		out = ButtonAction::PRESSED;
	}

	// Pop FrameBorderSize
	ImGui::PopStyleVar();
	// Pop all colors (Button, Hovered, Active, Border)
	ImGui::PopStyleColor(4);

	if (_hint && ImGui::IsItemHovered()) {
		if (ImGui::BeginItemTooltip())
		{
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(_hint);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	return out;
}


void dev::DeleteByIndex(dev::Disasm::LabelList& _labels, int& _idx)
{
	// delete by index or clear if only one
	if (_labels.size() > 1) {
		_labels.erase(_labels.begin() + _idx);
	} else {
		_labels[_idx].clear();
	}

	_idx = 0;
}