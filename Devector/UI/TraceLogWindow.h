#pragma once

#include <vector>

#include "Utils/Types.h"
#include "Utils/Consts.h"
#include "Ui/BaseWindow.h"
#include "Utils/ImGuiUtils.h"
#include "Utils/Result.h"
#include "Core/Hardware.h"
#include "Core/Debugger.h"

namespace dev
{
	class TraceLogWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 512;
		static constexpr int DEFAULT_WINDOW_H = 300;
		static constexpr float ADDR_W = 50.0f;
		static constexpr float CODE_W = 200.0f;
		static constexpr ImU32 DASM_BG_CLR_ADDR = dev::IM_U32(0x353636FF);
		static constexpr ImVec4 DASM_CLR_LABEL_MINOR = dev::IM_VEC4(0x909090FF);
		static constexpr ImVec4 DASM_CLR_MNEMONIC = dev::IM_VEC4(0x578DDFFF);

		struct ContextMenu {
			enum class Status { NONE = 0, INIT };
			Status status = Status::NONE;
			Addr addr = 0;
			std::string str;
			bool optionCopyAddr = false;
			const char* name = "DisasmItemMenu";

			void Init(Addr _addr, const std::string& _lineS)
			{
				status = Status::INIT;
				addr = _addr;
				str = _lineS;
			}
		};
		ContextMenu m_contextMenu;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqDisasm& m_reqDisasm;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		Disasm::Lines* m_traceLogP;
		size_t m_disasmFilter = 0;

		void UpdateData(const bool _isRunning);
		void DrawLog(const bool _isRunning);
		int DrawDisasmContextMenu(const bool _openContextMenu, int _copyToClipboardAddr);
		void DrawDisasmAddr(const bool _isRunning, const Disasm::Line& _line,
			ReqDisasm& _reqDisasm, ContextMenu& _contextMenu);
		void DrawDisasmCode(const bool _isRunning, const Disasm::Line& _line,
			ReqDisasm& _reqDisasm, ContextMenu& _contextMenu);

	public:
		TraceLogWindow(Hardware& _hardware, Debugger& _debugger, const float* const _fontSizeP, 
				const float* const _dpiScaleP, ReqDisasm& _reqDisasm);
		void Update(bool& _visible);
	};
};