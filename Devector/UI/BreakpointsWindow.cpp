#include <format>
#include "BreakpointsWindow.h"
#include "Utils/Types.h"
#include "Utils/Consts.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StrUtils.h"

dev::BreakpointsWindow::BreakpointsWindow(Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP, ReqDisasm& _reqDisasm)
	:
	BaseWindow("Breakpoints", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_debugger(_debugger),
	m_reqDisasm(_reqDisasm)
{}

void dev::BreakpointsWindow::Update(bool& _visible)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible)) 
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

		auto breakpoints = m_debugger.GetBreakpoints();

		for (const auto& [addr, bp] : breakpoints)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);

			// isActive
			ImGui::TableNextColumn();
						
			bool isActive = (bool)bp.status;
			ImGui::Checkbox(std::format("##0x{:04X}", addr).c_str(), &isActive);

			if (isActive != (bool)bp.status)
			{
				m_debugger.SetBreakpointStatus(addr, isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED);
				m_reqDisasm.type = ReqDisasm::Type::UPDATE;
			}
			// Addr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)addr;
			if (ImGui::Selectable(bp.GetAddrMappingS(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
			{
				selectedAddr = addr;
				m_reqDisasm.type = ReqDisasm::Type::UPDATE_ADDR;
				m_reqDisasm.addr = addr;
			}
			ImVec2 rowMin = ImGui::GetItemRectMin();
			CheckIfItemClicked(rowMin, showItemContextMenu, addr, editedBreakpointAddr, reqPopup);

			ImGui::PopStyleColor();

			// Condition
			const char* cond = bp.autoDel ? "Auto delete" : "";
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
				for (const auto& [addr, bp] : breakpoints) {
					m_debugger.SetBreakpointStatus(addr, Breakpoint::Status::DISABLED);
				}
				m_reqDisasm.type = ReqDisasm::Type::UPDATE;
			}
			else if (ImGui::MenuItem("Delete All")) {
				m_debugger.DelBreakpoints();
				m_reqDisasm.type = ReqDisasm::Type::UPDATE;
			}
			ImGui::EndPopup();
		}

		// the item context menu
		if (showItemContextMenu) ImGui::OpenPopup("BpItemMenu");
		if (ImGui::BeginPopup("BpItemMenu"))
		{
			if (editedBreakpointAddr >= 0)
			{
				const auto& bp = breakpoints.at(editedBreakpointAddr);

				if (bp.IsActive()) {
					if (ImGui::MenuItem("Disable")) {
						m_debugger.SetBreakpointStatus(editedBreakpointAddr, Breakpoint::Status::DISABLED);
						m_reqDisasm.type = ReqDisasm::Type::UPDATE;
					}
				}
				else {
					if (ImGui::MenuItem("Enable")) {
						m_debugger.SetBreakpointStatus(editedBreakpointAddr, Breakpoint::Status::ACTIVE);
						m_reqDisasm.type = ReqDisasm::Type::UPDATE;
					}
				}
				if (ImGui::MenuItem("Delete")) {
					m_debugger.DelBreakpoint(editedBreakpointAddr);
					m_reqDisasm.type = ReqDisasm::Type::UPDATE;
				}
				else if (ImGui::MenuItem("Edit")) {
					reqPopup = ReqPopup::INIT_EDIT;
				};
			}
			ImGui::EndPopup();
		}
		
		DrawPopup(reqPopup, breakpoints, editedBreakpointAddr);
	}

	ImGui::PopStyleVar(2);
}

void dev::BreakpointsWindow::DrawPopup(ReqPopup& _reqPopup, const Debugger::Breakpoints& _pbs, int _addr)
{
	if (_reqPopup == ReqPopup::NONE) {
		return;
	}
	else if (_reqPopup == ReqPopup::INIT_ADD || _reqPopup == ReqPopup::INIT_EDIT) {
		ImGui::OpenPopup("##BpEdit");
	}

	static uint8_t mappingPages = Breakpoint::MAPPING_PAGES_ALL;
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
		addrOld = bp.addr;
		mappingPages = bp.mappingPages;
		isAutoDel = bp.autoDel;
		addrS = std::format("{:04X}", bp.addr);
		selectedOp = static_cast<int>(bp.operand);
		selectedCond = static_cast<int>(bp.cond);
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
			bool mainRam = mappingPages & Breakpoint::MAPPING_RAM;
			bool mainRamDiskP0 = mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE0;
			bool mainRamDiskP1 = mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE1;
			bool mainRamDiskP2 = mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE2;
			bool mainRamDiskP3 = mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE3;
			DrawProperty2EditableCheckBox("Ram", "##BPContextAccessRam", &mainRam, "To check the main ram");
			DrawProperty2EditableCheckBox("Ram Disk Page 0", "##BPContextAccessRamDiskP0", &mainRamDiskP0, "To check the Ram-Disk page 0");
			DrawProperty2EditableCheckBox("Ram Disk Page 1", "##BPContextAccessRamDiskP1", &mainRamDiskP1, "To check the Ram-Disk page 1");
			DrawProperty2EditableCheckBox("Ram Disk Page 2", "##BPContextAccessRamDiskP2", &mainRamDiskP2, "To check the Ram-Disk page 2");
			DrawProperty2EditableCheckBox("Ram Disk Page 3", "##BPContextAccessRamDiskP3", &mainRamDiskP3, "To check the Ram-Disk page 3");
			mappingPages = static_cast<int>(mainRam) | 
				(static_cast<int>(mainRamDiskP0)<< 1) |
				(static_cast<int>(mainRamDiskP1)<< 2) |
				(static_cast<int>(mainRamDiskP2)<< 3) |
				(static_cast<int>(mainRamDiskP3)<< 4);

			// auto delete
			DrawProperty2EditableCheckBox("Auto Delete", "##BPContextAutoDel", &isAutoDel, "Removes the breakpoint when execution halts");
			// Operand
			DrawProperty2Combo("Operand", "##BPContextOperand", &selectedOp, 
				dev::bpOperandsS, IM_ARRAYSIZE(dev::bpOperandsS), "CC - CPU Cicles counted from the last reset/reboot/reload");
			// Condition
			DrawProperty2Combo("Condition", "##BPContextCondition", &selectedCond, dev::bpCondsS, IM_ARRAYSIZE(dev::bpCondsS), "");
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
					m_debugger.DelBreakpoint(addrOld);
				}
				m_debugger.AddBreakpoint(addr, mappingPages, 
					isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED, 
					isAutoDel, static_cast<Breakpoint::Operand>(selectedOp),
					static_cast<Breakpoint::Condition>(selectedCond), val, commentS);
				m_reqDisasm.type = ReqDisasm::Type::UPDATE;
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