#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "scheduler.h"
#include "ui/debugdata_popup.h"

namespace dev
{
	class DebugDataWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;

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


		void CallbackUpdateData(const dev::Signals _signals,
			dev::Scheduler::SignalData _data);
		void UpdateAndDrawFilteredElements(
			DebugData::FilteredElements& _filteredElements,
			DebugData::UpdateId& _filteredUpdateId,
			const DebugData::UpdateId& _updateId,
			std::string& _filter,
			DebugDataPopup::ElementType _elementType);

		void Draw(const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;

	public:
		DebugDataWindow(Hardware& _hardware, Debugger& _debugger,
			dev::Scheduler& _scheduler,
			bool* _visibleP);
	};
};