#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "scheduler.h"

namespace dev
{
	class DebugDataWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		DebugData::UpdateId m_labelsUpdates = 0;
		DebugData::UpdateId m_constsUpdates = 0;
		DebugData::UpdateId m_commentsUpdates = 0;
		DebugData::UpdateId m_editsUpdates = 0;
		DebugData::UpdateId m_codePerfsUpdates = 0;
		DebugData::UpdateId m_scriptsUpdates = 0;

		DebugData::FilteredElements m_filteredLabels;
		DebugData::FilteredElements m_filteredConsts;
		DebugData::FilteredElements m_filteredComments;
		DebugData::FilteredElements m_filteredEdits;
		DebugData::FilteredElements m_filteredCodePerfs;
		DebugData::FilteredElements m_filteredScripts;

		std::string m_labelFilter;
		std::string m_constFilter;
		std::string m_commentFilter;
		std::string m_editFilter;
		std::string m_codePerfFilter;
		std::string m_scriptFilter;

		std::string m_tempFilter;

		int m_selectedLineIdx = 0;

		enum class ElementType { LABEL = 0, CONST, COMMENT, MEMORY_EDIT, CODE_PERFS, SCRIPTS };

		struct ContextMenu {
			bool openPopup = false;
			ElementType elementType = ElementType::LABEL;
			int addr = 0;
			int oldAddr = 0;
			std::string elementName = "";
			std::string oldElementName = "";
			bool itemHovered = false;
			const char* contextMenuName = "DebugdataMenu";

			void Init(Addr _addr, const std::string& _elementName, const ElementType _elementType, const bool _itemHovered = true)
			{
				openPopup = true;
				elementType = _elementType;
				addr = _addr;
				oldAddr = _addr;
				elementName = _elementName;
				oldElementName = _elementName;
				itemHovered = _itemHovered;
			}

			bool BeginPopup(){
				if (openPopup) {
					ImGui::OpenPopup(contextMenuName);
					openPopup = false;
				}

				return ImGui::BeginPopup(contextMenuName);
			}
		};
		ContextMenu m_contextMenu;

		void UpdateData(const dev::Scheduler::Signals _signals);
		void UpdateAndDrawFilteredElements(
			DebugData::FilteredElements& _filteredElements,
			DebugData::UpdateId& _filteredUpdateId,
			const DebugData::UpdateId& _updateId,
			std::string& _filter,
			ElementType _elementType);


		void Draw(const dev::Scheduler::Signals _signals) override;
		void DrawContextMenu(ContextMenu& _contextMenu);

	public:
		DebugDataWindow(Hardware& _hardware, Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool& _visible, const float* const _dpiScaleP,
			ReqUI& _reqUI);
	};
};