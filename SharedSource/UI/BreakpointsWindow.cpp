#include <format>
#include "BreakpointsWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StringUtils.h"

dev::BreakpointsWindow::BreakpointsWindow(Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP, bool& _reqDisasmUpdate)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_debugger(_debugger),
	m_reqDisasmUpdate(_reqDisasmUpdate)
{}

void dev::BreakpointsWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Breakpoints", &open, ImGuiWindowFlags_NoCollapse);

	DrawTable();

	ImGui::End();
}

void dev::BreakpointsWindow::DrawProperty(const std::string& _name, const ImVec2& _aligment)
{
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name.c_str(), _aligment);
	ImGui::PopStyleColor();
}

void dev::BreakpointsWindow::DrawTable()
{
	static int selectedAddr = -1;
	bool showItemContextMenu = false;
	static int editedBreakpointAddr = -1;
	bool showItemEditPopup = false;
	bool showAddNewPopup = false;
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
			const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
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
			auto bpData = bp.GetData();
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);

			// isActive
			ImGui::TableNextColumn();
			bool isActive = (bool)bpData.status;
			ImGui::Checkbox(std::format("##0x{:04X}", addr).c_str(), &isActive);

			if (isActive != (bool)bpData.status)
			{
				m_debugger.SetBreakpointStatus(addr, isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED);
				m_reqDisasmUpdate = true;
			}
			// Addr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)addr;
			if (ImGui::Selectable(bpData.GetAddrMappingS(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedAddr = addr;
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				showItemContextMenu = true;
				editedBreakpointAddr = addr;
			}
			ImGui::PopStyleColor();

			// Condition
			const char* cond = bpData.autoDel ? "Auto delete" : "";
			DrawProperty(cond);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				showItemContextMenu = true;
				editedBreakpointAddr = addr;
			}

			// Comment
			DrawProperty(bp.GetComment());

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				showItemContextMenu = true;
				editedBreakpointAddr = addr;
			}
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
				showAddNewPopup = true;
			}
		}
		// the context menu
		if (ImGui::BeginPopupContextItem("BpContextMenu",
			ImGuiPopupFlags_NoOpenOverItems |
			ImGuiPopupFlags_NoOpenOverExistingPopup |
			ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Add New")) {
				showAddNewPopup = true;
			}
			else if (ImGui::MenuItem("Disable All")) 
			{
				for (const auto& [addr, bp] : breakpoints) {
					m_debugger.SetBreakpointStatus(addr, Breakpoint::Status::DISABLED);
				}
				m_reqDisasmUpdate = true;
			}
			else if (ImGui::MenuItem("Delete All")) {
				m_debugger.DelBreakpoints();
				m_reqDisasmUpdate = true;
			};
			ImGui::EndPopup();
		}

		// the item context menu
		if (showItemContextMenu) ImGui::OpenPopup("BpItemMenu");
		if (ImGui::BeginPopup("BpItemMenu"))
		{
			if (editedBreakpointAddr >= 0)
			{
				const auto& bp = breakpoints.at(editedBreakpointAddr);

				if (bp.GetData().IsActive()) {
					if (ImGui::MenuItem("Disable")) {
						m_debugger.SetBreakpointStatus(editedBreakpointAddr, Breakpoint::Status::DISABLED);
						m_reqDisasmUpdate = true;
					}
				}
				else {
					if (ImGui::MenuItem("Enable")) {
						m_debugger.SetBreakpointStatus(editedBreakpointAddr, Breakpoint::Status::ACTIVE);
						m_reqDisasmUpdate = true;
					}
				}
				if (ImGui::MenuItem("Delete")) {
					m_debugger.DelBreakpoint(editedBreakpointAddr);
					m_reqDisasmUpdate = true;
				}
				else if (ImGui::MenuItem("Edit")) {
					showItemEditPopup = true;
				};
			}
			ImGui::EndPopup();
		}
		
		if (showItemEditPopup || showAddNewPopup) ImGui::OpenPopup("BpEdit");
		DrawPopupEdit(showAddNewPopup, showItemEditPopup, breakpoints, editedBreakpointAddr);

	}

	ImGui::PopStyleVar(2);
}

void dev::BreakpointsWindow::DrawPopupEdit(const bool _addNew, const bool _init, const Debugger::Breakpoints& _pbs, int _addr)
{
	static auto bpData = Breakpoint::Data(0x100);
	static uint8_t mappingPages = bpData.mappingPages;
	static bool isActive = bpData.IsActive();
	static bool isAutoDel = bpData.autoDel;
	static GlobalAddr addrOld = 0x100;
	static std::string addrS = "0x100";
	static std::string conditionS = "";
	static std::string commentS = "";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	if (!_addNew && _init && _addr >= 0)
	{
		const auto& bp = _pbs.at(_addr);
		bpData = bp.GetData();
		isActive = bpData.status == Breakpoint::Status::ACTIVE;
		addrOld = bpData.addr;
		mappingPages = bpData.mappingPages;
		isAutoDel = bpData.autoDel;
		addrS = bpData.GetAddrS();

		//conditionS = bp.GetConditionS();
		commentS = bp.GetComment();
	}

	if (ImGui::BeginPopup("BpEdit"))
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
			DrawProperty2EditableS("Address", "##BPContextAddress", &addrS, "0x100",
				"A hexademical address in the format 0x100 or 100.");
			// mapping
			//DrawProperty2EditableMapping("Mapping", &bpData, "");
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
			mappingPages = mainRam | mainRamDiskP0 << 1 | mainRamDiskP1 << 2 | mainRamDiskP2 << 3 | mainRamDiskP3 << 4;

			// condition
			//DrawProperty2EditableS("Condition", "##BPContextCondition", &conditionS, "");
			// auto delete
			DrawProperty2EditableCheckBox("Auto Delete", "##BPContextAutoDel", &isAutoDel, "Removes the breakpoint when execution halts");
			// comment
			DrawProperty2EditableS("Comment", "##BPContextComment", &commentS, "");


			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			// warning
			bool warning = false;
			int addr = dev::StrHexToInt(addrS.c_str());
			std::string warningS = "";
			if (addr > Memory::MEMORY_MAIN_LEN - 1) {
				warningS = "Too large address";
				warning = true;
			}
			//ImGui::SameLine(); 
			ImGui::TextColored(COLOR_WARNING, warningS.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();

			if (warning) {
				ImGui::BeginDisabled();
			}
			// OK button
			if (ImGui::Button("Ok", buttonSize) && !warning)
			{
				if (!_addNew && addrOld != addr) {
					m_debugger.DelBreakpoint(addrOld);
				}
				m_debugger.AddBreakpoint(addr, mappingPages, isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED, isAutoDel, commentS);
				m_reqDisasmUpdate = true;
				ImGui::CloseCurrentPopup();
			}
			if (warning) {
				ImGui::EndDisabled();
			}

			// Cancel button
			ImGui::SameLine(); ImGui::Text(" "); ImGui::SameLine();
			if (ImGui::Button("Cancel", buttonSize)) ImGui::CloseCurrentPopup();

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}
/*
void dev::BreakpointsWindow::DrawProperty2EditableMapping(const char* _name, Breakpoint::Data* _val, const char* _help)
{
	static bool mainRam = _val->mappingPages & Breakpoint::MAPPING_RAM;
	static bool mainRamDiskP0 = _val->mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE0;
	static bool mainRamDiskP1 = _val->mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE1;
	static bool mainRamDiskP2 = _val->mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE2;
	static bool mainRamDiskP3 = _val->mappingPages & Breakpoint::MAPPING_RAMDISK_PAGE3;

	ImGui::TableNextRow(ImGuiTableRowFlags_None, 30.0f);
	ImGui::TableNextColumn();

	ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
	TextAligned(_name, { 1.0f, 0.5f });
	ImGui::PopStyleColor();

	ImGui::TableNextColumn();
	ImGui::Checkbox("##BPContextMappingRam", &mainRam);
	ImGui::SameLine(); ImGui::Checkbox(" ##BPContextMappingRamDiskP0", &mainRamDiskP0);
	ImGui::SameLine(); ImGui::Checkbox("##BPContextMappingRamDiskP1", &mainRamDiskP1);
	ImGui::SameLine(); ImGui::Checkbox("##BPContextMappingRamDiskP2", &mainRamDiskP2);
	ImGui::SameLine(); ImGui::Checkbox("##BPContextMappingRamDiskP3", &mainRamDiskP3);

	if (*_help != '\0') {
		ImGui::SameLine();
		ImGui::Dummy({ 80,10 });
		ImGui::SameLine();
		dev::DrawHelpMarker(_help);
	}

	_val->mappingPages = mainRam | mainRamDiskP0 << 1 | mainRamDiskP1 << 2 | mainRamDiskP2 << 3 | mainRamDiskP3 << 4;
}
*/