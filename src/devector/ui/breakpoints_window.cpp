#include <format>

#include "ui/breakpoints_window.h"
#include "utils/types.h"
#include "utils/consts.h"
#include "utils/str_utils.h"

dev::BreakpointsWindow::BreakpointsWindow(Hardware& _hardware,
	const float* const _fontSizeP, const float* const _dpiScaleP, ReqUI& _reqUI)
	:
	BaseWindow("Breakpoints", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_hardware(_hardware), m_reqUI(_reqUI)
{}

void dev::BreakpointsWindow::Update(bool& _visible)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		DrawTable();
		ImGui::End();
	}
}

void dev::BreakpointsWindow::DrawProperty(const std::string& _name, const ImVec2& _aligment)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();
}

void dev::BreakpointsWindow::CheckIfItemClicked(const ImVec2& _rowMin, bool& _showItemContextMenu,
	const int _addr, int& _editedBreakpointAddr, ReqPopup& _reqPopup)
{
	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
	{
		_showItemContextMenu = true; 
		_editedBreakpointAddr = _addr;
	}

	// if double clicked, open the wp edit window
	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		ImVec2 rowMax = ImGui::GetItemRectMax();

		if (ImGui::IsMouseHoveringRect(_rowMin, rowMax))
		{
			_editedBreakpointAddr = _addr;
			_reqPopup = ReqPopup::INIT_EDIT;
		}
	}
}

void dev::BreakpointsWindow::DrawTable()
{
	static int selectedAddr = -1;
	bool showItemContextMenu = false;
	static int editedBreakpointAddr = -1;
	static ReqPopup reqPopup = ReqPopup::NONE;
	const int COLUMNS_COUNT = 4;

	const char* tableName = "##Breakpoints";
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 5.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });

	static ImGuiTableFlags flags =
		ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_SizingStretchSame |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Hideable;
	if (ImGui::BeginTable(tableName, COLUMNS_COUNT, flags))
	{
		ImGui::TableSetupColumn("##BPActive", ImGuiTableColumnFlags_WidthFixed, 25);
		ImGui::TableSetupColumn("GlobalAddr", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthFixed, 180);
		ImGui::TableSetupColumn("Comment", ImGuiTableColumnFlags_WidthStretch);
		
		ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
		for (int column = 0; column < COLUMNS_COUNT; column++)
		{
			ImGui::TableSetColumnIndex(column);
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve the name passed to TableSetupColumn()
			if (column == 0) {
				DrawHelpMarker(
					"Define an address and conditions; halts execution when the\n"
					"program counter reaches it under specified conditions\n\n"

					"Left-click in empty space: Open context menu\n"
					"Left-click on item: Open item context menu\n"
					"Double left-click in empty space: Create a new breakpoint");
			}
			else {
				ImGui::TableHeader(column_name);
			}
		}

		PushStyleCompact(1.0f, 0.0f);

		UpdateBreakpoints();

		for (const auto& [addr, bp] : m_breakpoints)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);

			// isActive
			ImGui::TableNextColumn();
						
			bool isActive = bp.data.structured.status == Breakpoint::Status::ACTIVE;
			ImGui::Checkbox(std::format("##0x{:04X}", addr).c_str(), &isActive);
			Breakpoint::Status newStatus = isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED;

			if (newStatus != bp.data.structured.status)
			{
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_SET_STATUS, 
					{ {"addr", addr},
					{"status", static_cast<uint8_t>(newStatus)}});
				m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
			}
			// Addr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)addr;
			if (ImGui::Selectable(bp.GetAddrMappingS(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
			{
				selectedAddr = addr;
				m_reqUI.type = ReqUI::Type::DISASM_UPDATE_ADDR;
				m_reqUI.globalAddr = addr;
			}
			ImVec2 rowMin = ImGui::GetItemRectMin();
			CheckIfItemClicked(rowMin, showItemContextMenu, addr, editedBreakpointAddr, reqPopup);

			ImGui::PopStyleColor();

			// Condition
			std::string cond = bp.GetConditionS();
			DrawProperty(cond);
			CheckIfItemClicked(rowMin, showItemContextMenu, addr, editedBreakpointAddr, reqPopup);

			// Comment
			DrawProperty(bp.comment);
			CheckIfItemClicked(rowMin, showItemContextMenu, addr, editedBreakpointAddr, reqPopup);
		}

		PopStyleCompact();
		ImGui::EndTable();

		// double-click to add a new item
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
			!ImGui::IsAnyItemHovered())
		{
			ImVec2 tableMin = ImGui::GetItemRectMin();
			ImVec2 tableMax = ImGui::GetItemRectMax();
			if (ImGui::IsMouseHoveringRect(tableMin, tableMax))
			{
				reqPopup = ReqPopup::INIT_ADD;
			}
		}
		// the context menu
		if (ImGui::BeginPopupContextItem("BpContextMenu",
			ImGuiPopupFlags_NoOpenOverItems |
			ImGuiPopupFlags_NoOpenOverExistingPopup |
			ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Add New")) {
				reqPopup = ReqPopup::INIT_ADD;
			}
			else if (ImGui::MenuItem("Disable All")) 
			{
				for (const auto& [addr, bp] : m_breakpoints) {
					m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DISABLE, { {"addr", addr} });
				}
				m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
			}
			else if (ImGui::MenuItem("Delete All")) 
			{
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL_ALL);
				m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
			}
			ImGui::EndPopup();
		}

		// the item context menu
		if (showItemContextMenu) ImGui::OpenPopup("BpItemMenu");
		if (ImGui::BeginPopup("BpItemMenu"))
		{
			if (editedBreakpointAddr >= 0)
			{
				const auto& bp = m_breakpoints.at(editedBreakpointAddr);

				if (bp.IsActive()) {
					if (ImGui::MenuItem("Disable")) {
						m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DISABLE, { {"addr", editedBreakpointAddr} });
						m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
					}
				}
				else {
					if (ImGui::MenuItem("Enable")) {
						m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ACTIVE, { {"addr", editedBreakpointAddr} });
						m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
					}
				}
				if (ImGui::MenuItem("Delete")) {
					m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL, { {"addr", editedBreakpointAddr}});
					m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
				}
				else if (ImGui::MenuItem("Edit")) {
					reqPopup = ReqPopup::INIT_EDIT;
				};
			}
			ImGui::EndPopup();
		}
		
		DrawPopup(reqPopup, m_breakpoints, editedBreakpointAddr);
	}

	ImGui::PopStyleVar(2);
}

void dev::BreakpointsWindow::DrawPopup(ReqPopup& _reqPopup, const Breakpoints::BpMap& _pbs, int _addr)
{
	if (_reqPopup == ReqPopup::NONE) {
		return;
	}
	else if (_reqPopup == ReqPopup::INIT_ADD || _reqPopup == ReqPopup::INIT_EDIT) {
		ImGui::OpenPopup("##BpEdit");
	}

	static Breakpoint::MemPages memPages = Breakpoint::MAPPING_PAGES_ALL;
	static bool isActive = true;
	static bool isAutoDel = false;
	static Addr addrOld = 0xFF;
	static std::string addrS = "FF";
	static int selectedOp = 0;
	static int selectedCond = 0;
	static std::string valueS = "0";
	static std::string commentS = "";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	// Init for a new BP
	if (_reqPopup == ReqPopup::INIT_ADD) {
		_reqPopup = ReqPopup::ADD;
		commentS = "";
		addrOld = 0xFF;
		addrS = "FF";
	}
	// Init for editing BP
	if (_reqPopup == ReqPopup::INIT_EDIT)
	{
		_reqPopup = ReqPopup::EDIT;

		const auto& bp = _pbs.at(_addr);
		isActive = bp.IsActive();
		addrOld = bp.data.structured.addr;
		memPages = bp.data.structured.memPages;
		isAutoDel = bp.data.structured.autoDel;
		addrS = std::format("{:04X}", bp.data.structured.addr);
		selectedOp = static_cast<int>(bp.data.structured.operand);
		selectedCond = static_cast<int>(bp.data.structured.cond);
		commentS = bp.comment;
	}

	if (ImGui::BeginPopup("##BpEdit"))
	{
		static ImGuiTableFlags flags =
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_SizingStretchSame |
			ImGuiTableFlags_ContextMenuInBody;

		if (ImGui::BeginTable("##BPContextMenu", 2, flags))
		{
			ImGui::TableSetupColumn("##BPContextMenuName", ImGuiTableColumnFlags_WidthFixed, 140);
			ImGui::TableSetupColumn("##BPContextMenuVal", ImGuiTableColumnFlags_WidthFixed, 140);
			// status
			DrawProperty2EditableCheckBox("Active", "##BPContextStatus", &isActive, "Disable the breakpoint");
			// addr
			DrawProperty2EditableS("Address", "##BPContextAddress", &addrS, "FFFF",
				"A hexademical address in the format 0xFF or FF.");
			// mapping
			bool ram = memPages.ram;
			bool rd00 = memPages.rdisk0page0;
			bool rd01 = memPages.rdisk0page1;
			bool rd02 = memPages.rdisk0page2;
			bool rd03 = memPages.rdisk0page3;
			bool rd10 = memPages.rdisk1page0;
			bool rd11 = memPages.rdisk1page1;
			bool rd12 = memPages.rdisk1page2;
			bool rd13 = memPages.rdisk1page3;
			bool rd20 = memPages.rdisk2page0;
			bool rd21 = memPages.rdisk2page1;
			bool rd22 = memPages.rdisk2page2;
			bool rd23 = memPages.rdisk2page3;
			bool rd30 = memPages.rdisk3page0;
			bool rd31 = memPages.rdisk3page1;
			bool rd32 = memPages.rdisk3page2;
			bool rd33 = memPages.rdisk3page3;
			bool rd40 = memPages.rdisk4page0;
			bool rd41 = memPages.rdisk4page1;
			bool rd42 = memPages.rdisk4page2;
			bool rd43 = memPages.rdisk4page3;
			bool rd50 = memPages.rdisk5page0;
			bool rd51 = memPages.rdisk5page1;
			bool rd52 = memPages.rdisk5page2;
			bool rd53 = memPages.rdisk5page3;
			bool rd60 = memPages.rdisk6page0;
			bool rd61 = memPages.rdisk6page1;
			bool rd62 = memPages.rdisk6page2;
			bool rd63 = memPages.rdisk6page3;
			bool rd70 = memPages.rdisk7page0;
			bool rd71 = memPages.rdisk7page1;
			bool rd72 = memPages.rdisk7page2;
			bool rd73 = memPages.rdisk7page3;

			DrawProperty2EditableCheckBox("Ram", "##BPContextAccessRam", &ram, "To check the main ram");
			DrawProperty2EditableCheckBox4("Ram Disk 1", "##BPCARD0P0", "##BPCARD0P1", "##BPCARD0P2", "##BPCARD0P3", &rd00, &rd01, &rd02, &rd03, "To check the Ram-Disk1 pages 0,1,2,3");
			DrawProperty2EditableCheckBox4("Ram Disk 2", "##BPCARD1P0", "##BPCARD1P1", "##BPCARD1P2", "##BPCARD1P3", &rd10, &rd11, &rd12, &rd13, "To check the Ram-Disk2 pages 0,1,2,3");
			DrawProperty2EditableCheckBox4("Ram Disk 3", "##BPCARD2P0", "##BPCARD2P1", "##BPCARD2P2", "##BPCARD2P3", &rd20, &rd21, &rd22, &rd23, "To check the Ram-Disk3 pages 0,1,2,3");
			DrawProperty2EditableCheckBox4("Ram Disk 4", "##BPCARD3P0", "##BPCARD3P1", "##BPCARD3P2", "##BPCARD3P3", &rd30, &rd31, &rd32, &rd33, "To check the Ram-Disk4 pages 0,1,2,3");
			DrawProperty2EditableCheckBox4("Ram Disk 5", "##BPCARD4P0", "##BPCARD4P1", "##BPCARD4P2", "##BPCARD4P3", &rd40, &rd41, &rd42, &rd43, "To check the Ram-Disk5 pages 0,1,2,3");
			DrawProperty2EditableCheckBox4("Ram Disk 6", "##BPCARD5P0", "##BPCARD5P1", "##BPCARD5P2", "##BPCARD5P3", &rd50, &rd51, &rd52, &rd53, "To check the Ram-Disk6 pages 0,1,2,3");
			DrawProperty2EditableCheckBox4("Ram Disk 7", "##BPCARD6P0", "##BPCARD6P1", "##BPCARD6P2", "##BPCARD6P3", &rd60, &rd61, &rd62, &rd63, "To check the Ram-Disk7 pages 0,1,2,3");
			DrawProperty2EditableCheckBox4("Ram Disk 8", "##BPCARD7P0", "##BPCARD7P1", "##BPCARD7P2", "##BPCARD7P3", &rd70, &rd71, &rd72, &rd73, "To check the Ram-Disk8 pages 0,1,2,3");

			memPages.ram = ram;
			memPages.rdisk0page0 = rd00;
			memPages.rdisk0page1 = rd01;
			memPages.rdisk0page2 = rd02;
			memPages.rdisk0page3 = rd03;
			memPages.rdisk1page0 = rd10;
			memPages.rdisk1page1 = rd11;
			memPages.rdisk1page2 = rd12;
			memPages.rdisk1page3 = rd13;
			memPages.rdisk2page0 = rd20;
			memPages.rdisk2page1 = rd21;
			memPages.rdisk2page2 = rd22;
			memPages.rdisk2page3 = rd23;
			memPages.rdisk3page0 = rd30;
			memPages.rdisk3page1 = rd31;
			memPages.rdisk3page2 = rd32;
			memPages.rdisk3page3 = rd33;
			memPages.rdisk4page0 = rd40;
			memPages.rdisk4page1 = rd41;
			memPages.rdisk4page2 = rd42;
			memPages.rdisk4page3 = rd43;
			memPages.rdisk5page0 = rd50;
			memPages.rdisk5page1 = rd51;
			memPages.rdisk5page2 = rd52;
			memPages.rdisk5page3 = rd53;
			memPages.rdisk6page0 = rd60;
			memPages.rdisk6page1 = rd61;
			memPages.rdisk6page2 = rd62;
			memPages.rdisk6page3 = rd63;
			memPages.rdisk7page0 = rd70;
			memPages.rdisk7page1 = rd71;
			memPages.rdisk7page2 = rd72;
			memPages.rdisk7page3 = rd73;

			// auto delete
			DrawProperty2EditableCheckBox("Auto Delete", "##BPContextAutoDel", &isAutoDel, "Removes the breakpoint when execution halts");
			// Operand
			DrawProperty2Combo("Operand", "##BPContextOperand", &selectedOp, 
				dev::bpOperandsS, IM_ARRAYSIZE(dev::bpOperandsS), "CC - CPU Cicles counted from the last reset/reboot/reload");
			// Condition
			DrawProperty2Combo("Condition", "##BPContextCondition", &selectedCond, dev::ConditionsS, IM_ARRAYSIZE(dev::ConditionsS), "");
			// Value
			if (selectedCond == 0) ImGui::BeginDisabled();
			DrawProperty2EditableS("Value", "##BPContextValue", &valueS, "FF",
				"A hexademical value in the format FF",
				ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
			if (selectedCond == 0) ImGui::EndDisabled();
			// Comment
			DrawProperty2EditableS("Comment", "##BPContextComment", &commentS, "");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// warning
			int addr = dev::StrHexToInt(addrS.c_str());
			std::string warningS = "";
			warningS = addr >= Memory::MEMORY_MAIN_LEN ? 
				"Too large address" : warningS;
			int val = dev::StrHexToInt(valueS.c_str());
			size_t maxVal = selectedOp == static_cast<int>(Breakpoint::Operand::CC) ?
				UINT64_MAX : selectedOp > static_cast<int>(Breakpoint::Operand::PSW) ?
				UINT16_MAX : UINT8_MAX;
			warningS = val < 0 || val > maxVal ?
				"A value is out of range" : warningS;

			//ImGui::SameLine(); 
			ImGui::TextColored(COLOR_WARNING, warningS.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();

			if (!warningS.empty()) ImGui::BeginDisabled();
			// OK button
			if (ImGui::Button("Ok", buttonSize))
			{
				if (_reqPopup == ReqPopup::EDIT && addrOld != addr)
				{
					m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_DEL, { {"addr", addrOld} });
				}
				Breakpoint::Data bpData
				{
					static_cast<Addr>(addr), memPages,
					isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED,
					isAutoDel, static_cast<Breakpoint::Operand>(selectedOp),
					static_cast<dev::Condition>(selectedCond), static_cast<size_t>(val)
				};
				m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_ADD, {
					{"data0", bpData.data0 },
					{"data1", bpData.data1 },
					{"data2", bpData.data2 },
					{"comment", commentS}
				});
				m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
				ImGui::CloseCurrentPopup();
				_reqPopup = ReqPopup::NONE;
			}
			if (!warningS.empty()) ImGui::EndDisabled();

			// Cancel button
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) 
			{
				ImGui::CloseCurrentPopup();
				_reqPopup = ReqPopup::NONE;
			}

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}

void dev::BreakpointsWindow::UpdateBreakpoints()
{
	size_t updates = m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_GET_UPDATES)->at("updates");

	if (updates == m_updates) return;

	m_updates = updates;

	m_breakpoints.clear();
	auto breakpointsJ = m_hardware.Request(Hardware::Req::DEBUG_BREAKPOINT_GET_ALL);
	if (breakpointsJ)
	{
		for (const auto& breakpointJ : *breakpointsJ)
		{
			Breakpoint::Data bpData{ breakpointJ["data0"], breakpointJ["data1"], breakpointJ["data2"] };

			Breakpoint bp{ std::move(bpData), breakpointJ["comment"] };
			auto addr = bp.data.structured.addr;
			m_breakpoints.emplace(addr, std::move(bp));
		}
	}

}