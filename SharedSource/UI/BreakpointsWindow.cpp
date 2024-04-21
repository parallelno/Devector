#include <format>
#include "BreakpointsWindow.h"
#include "Utils/Consts.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/StringUtils.h"

dev::BreakpointsWindow::BreakpointsWindow(Debugger& _debugger,
	const float* const _fontSizeP, const float* const _dpiScaleP, int& _reqDisasmUpdate, int& _reqDisasmUpdateData)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
	m_debugger(_debugger),
	m_reqDisasmUpdate(_reqDisasmUpdate), m_reqDisasmUpdateData(_reqDisasmUpdateData)
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

#define CHECK_IF_ITEM_CLICKED		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {showItemContextMenu = true; editedBreakpointAddr = addr; }

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
			auto bpData = bp.GetData();
			ImGui::TableNextRow(ImGuiTableRowFlags_None, 21.0f);

			// isActive
			ImGui::TableNextColumn();
						
			bool isActive = (bool)bpData.status;
			ImGui::Checkbox(std::format("##0x{:04X}", addr).c_str(), &isActive);

			if (isActive != (bool)bpData.status)
			{
				m_debugger.SetBreakpointStatus(addr, isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED);
				m_reqDisasmUpdate = REQ_DISASM_DRAW;
			}
			// Addr
			ImGui::TableNextColumn();
			ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(0x909090FF));
			const bool isSelected = selectedAddr == (int)addr;
			if (ImGui::Selectable(bpData.GetAddrMappingS(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
			{
				selectedAddr = addr;

				m_reqDisasmUpdate = REQ_DISASM_BRK;
				m_reqDisasmUpdateData = addr;
			}
			CHECK_IF_ITEM_CLICKED

			ImGui::PopStyleColor();

			// Condition
			const char* cond = bpData.autoDel ? "Auto delete" : "";
			DrawProperty(cond);
			CHECK_IF_ITEM_CLICKED

			// Comment
			DrawProperty(bp.GetComment());
			CHECK_IF_ITEM_CLICKED
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
				m_reqDisasmUpdate = REQ_DISASM_DRAW;
			}
			else if (ImGui::MenuItem("Delete All")) {
				m_debugger.DelBreakpoints();
				m_reqDisasmUpdate = REQ_DISASM_DRAW;
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

				if (bp.GetData().IsActive()) {
					if (ImGui::MenuItem("Disable")) {
						m_debugger.SetBreakpointStatus(editedBreakpointAddr, Breakpoint::Status::DISABLED);
						m_reqDisasmUpdate = REQ_DISASM_DRAW;
					}
				}
				else {
					if (ImGui::MenuItem("Enable")) {
						m_debugger.SetBreakpointStatus(editedBreakpointAddr, Breakpoint::Status::ACTIVE);
						m_reqDisasmUpdate = REQ_DISASM_DRAW;
					}
				}
				if (ImGui::MenuItem("Delete")) {
					m_debugger.DelBreakpoint(editedBreakpointAddr);
					m_reqDisasmUpdate = REQ_DISASM_DRAW;
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

	static auto bpData = Breakpoint::Data(0xFF);
	static uint8_t mappingPages = bpData.mappingPages;
	static bool isActive = bpData.IsActive();
	static bool isAutoDel = bpData.autoDel;
	static GlobalAddr addrOld = 0xFF;
	static std::string addrS = "0xFF";
	//static std::string conditionS = "";
	static std::string commentS = "";
	static ImVec2 buttonSize = { 65.0f, 25.0f };

	// Init for a new BP
	if (_reqPopup == ReqPopup::INIT_ADD) {
		_reqPopup = ReqPopup::ADD;
		commentS = "";
	}
	// Init for editing BP
	if (_reqPopup == ReqPopup::INIT_EDIT)
	{
		_reqPopup = ReqPopup::EDIT;

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
			mappingPages = static_cast<int>(mainRam) | 
				(static_cast<int>(mainRamDiskP0)<< 1) |
				(static_cast<int>(mainRamDiskP1)<< 2) |
				(static_cast<int>(mainRamDiskP2)<< 3) |
				(static_cast<int>(mainRamDiskP3)<< 4);

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
			int addr = dev::StrHexToInt(addrS.c_str());
			std::string warningS = "";
			if (addr > Memory::MEMORY_MAIN_LEN - 1) {
				warningS = "Too large address";
			}
			//ImGui::SameLine(); 
			ImGui::TextColored(COLOR_WARNING, warningS.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();

			if (warningS.empty()) {
				ImGui::BeginDisabled();
			}
			// OK button
			if (ImGui::Button("Ok", buttonSize) && !warningS.empty())
			{
				if (_reqPopup == ReqPopup::EDIT && addrOld != addr) 
				{
					m_debugger.DelBreakpoint(addrOld);
				}
				m_debugger.AddBreakpoint(addr, mappingPages, isActive ? Breakpoint::Status::ACTIVE : Breakpoint::Status::DISABLED, isAutoDel, commentS);
				m_reqDisasmUpdate = REQ_DISASM_DRAW;
				ImGui::CloseCurrentPopup();
				_reqPopup = ReqPopup::NONE;
			}
			if (warningS.empty()) {
				ImGui::EndDisabled();
			}

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