#pragma once

#include <vector>

#include "utils/types.h"
#include "utils/consts.h"
#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "core/hardware.h"
#include "core/debugger.h"

namespace dev 
{
	class HexViewerWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 512;
		static constexpr int DEFAULT_WINDOW_H = 300;

		static constexpr ImU32 BG_COLOR_ADDR = dev::IM_U32(0x303030FF);
		static constexpr ImU32 BG_COLOR_ADDR_HOVER = dev::IM_U32(0x1E4D8CFF);

		static constexpr ImVec4 COLOR_ADDR = dev::IM_VEC4(0x909090FF); 
		static constexpr ImVec4 COLOR_VALUE = dev::IM_VEC4(0xD4D4D4FF);

		static constexpr ImU32 BG_COLOR_BYTE_HOVER = IM_COL32(100, 10, 150, 255);

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		int64_t m_ccLast = -1; // to force the first stats update
		std::array<uint8_t, Memory::MEMORY_MAIN_LEN> m_ram;
		char m_searchAddrS[255] = "";

		enum class Status { NONE = 0, HIGHLIGHT };
		Status m_status = Status::NONE;
		int m_memPageIdx = 0;
		GlobalAddr m_highlightAddr = 0;
		GlobalAddr m_highlightAddrLen = 0;

		void UpdateData(const bool _isRunning);
		void DrawHex(const bool _isRunning);

	public:
		HexViewerWindow(Hardware& _hardware, Debugger& _debugger, const float* const _fontSizeP, 
				const float* const _dpiScaleP, ReqUI& _reqUI);
		void Update(bool& _visible);
	};
};