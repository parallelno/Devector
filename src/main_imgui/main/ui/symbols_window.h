#pragma once

#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"

namespace dev
{
	class SymbolsWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		int64_t m_ccLast = -1; // to force the first stats update

		DebugData::UpdateId m_labelsUpdates = 0;
		DebugData::UpdateId m_constsUpdates = 0;
		DebugData::UpdateId m_commentsUpdates = 0;

		DebugData::SymbolAddrList m_filteredLabels;
		DebugData::SymbolAddrList m_filteredConsts;
		DebugData::SymbolAddrList m_filteredComments;

		std::string m_labelFilter;
		std::string m_constFilter;
		std::string m_commentFilter;

		std::string m_tempFilter;

		int m_selectedLineIdx = 0;

		void UpdateData(const bool _isRunning);

		void UpdateAndDrawFilteredSymbols(DebugData::SymbolAddrList& _filteredSymbols,
										DebugData::UpdateId& _updateId, std::string& _filter,
										void (DebugData::*getFilteredFunc)(DebugData::SymbolAddrList& _out, const std::string& _filter) const);

	public:
		SymbolsWindow(Hardware& _hardware, Debugger& _debugger, 
			const float* const _dpiScaleP,
			ReqUI& _reqUI);
		void Update(bool& _visible, const bool _isRunning);
		void Draw(const bool _isRunning);
	};
};