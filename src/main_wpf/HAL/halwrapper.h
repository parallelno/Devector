#pragma once
#include <string>
#include <windows.h>

#include <iostream>
#include <array>

#include <windows.h>

#include "utils/types.h"
#include "utils/json_utils.h"

#include "core/hardware.h"
#include "core/debugger.h"

#include "utils/win_gl_utils.h"

namespace dev 
{

    public ref class HAL
    {
        Hardware* m_hardwareP;
        Debugger* m_debuggerP;
        WinGlUtils* m_winGlUtilsP;

        int64_t m_ccLast = -1; // to force the first stats update
        int64_t m_ccLastRun = 0;

        //Id m_vramShaderId = INVALID_ID;
        //Id m_vramMatId = INVALID_ID;
        //Id m_vramTexId = INVALID_ID;

        bool m_glInited = false;
        bool m_displayIsHovered = false;
        const char* m_contextMenuName = "##displayCMenu";
        int m_rasterPixel = 0;
        int m_rasterLine = 0;

        //bool DisplayWindowInit(const GLsizei _viewportW, const GLsizei _viewportH);
        

    public:
		literal float FRAME_PXL_SIZE_W = 1.0f / Display::FRAME_W;
		literal float FRAME_PXL_SIZE_H = 1.0f / Display::FRAME_H;
		literal float SCANLINE_HIGHLIGHT_MUL = 0.3f;

		literal float ACTIVE_AREA_W = Display::ACTIVE_AREA_W;
		literal float ACTIVE_AREA_H = Display::ACTIVE_AREA_H;
		literal float SCAN_ACTIVE_AREA_TOP = Display::SCAN_ACTIVE_AREA_TOP;

		enum class Req : int {
			NONE = 0,
			RUN,
			STOP,
			IS_RUNNING,
			EXIT,
			RESET,			// reboot the pc, enable the ROM
			RESTART,		// reboot the pc, disable the ROM
			EXECUTE_INSTR,
			EXECUTE_FRAME,
			EXECUTE_FRAME_NO_BREAKS,
			GET_CC,
			GET_REGS,
			GET_REG_PC,
			GET_BYTE_GLOBAL,
			GET_BYTE_RAM,
			GET_THREE_BYTES_RAM,
			GET_WORD_STACK,
			GET_DISPLAY_DATA,
			GET_MEMORY_MAPPING,
			GET_GLOBAL_ADDR_RAM,
			GET_FDC_INFO,
			GET_FDD_INFO,
			GET_FDD_IMAGE,
			GET_RUSLAT_HISTORY,
			GET_PALETTE,
			GET_SCROLL_VERT,
			GET_STEP_OVER_ADDR,
			GET_IO_PORTS,
			GET_IO_PORTS_IN_DATA,
			GET_IO_PORTS_OUT_DATA,
			GET_IO_DISPLAY_MODE,
			GET_IO_PALETTE_COMMIT_TIME,
			SET_IO_PALETTE_COMMIT_TIME,
			GET_DISPLAY_BORDER_LEFT,
			SET_DISPLAY_BORDER_LEFT,
			GET_DISPLAY_IRQ_COMMIT_PXL,
			SET_DISPLAY_IRQ_COMMIT_PXL,
			SET_MEM,
			SET_CPU_SPEED,
			IS_MEMROM_ENABLED,
			KEY_HANDLING,
			LOAD_FDD,
			RESET_UPDATE_FDD,
			DEBUG_ATTACH,
			DEBUG_RESET,

			DEBUG_RECORDER_RESET,
			DEBUG_RECORDER_PLAY_FORWARD,
			DEBUG_RECORDER_PLAY_REVERSE,
			DEBUG_RECORDER_GET_STATE_RECORDED,
			DEBUG_RECORDER_GET_STATE_CURRENT,
			DEBUG_RECORDER_SERIALIZE,
			DEBUG_RECORDER_DESERIALIZE,

			DEBUG_BREAKPOINT_ADD,
			DEBUG_BREAKPOINT_DEL,
			DEBUG_BREAKPOINT_DEL_ALL,
			DEBUG_BREAKPOINT_GET_STATUS,
			DEBUG_BREAKPOINT_SET_STATUS,
			DEBUG_BREAKPOINT_ACTIVE,
			DEBUG_BREAKPOINT_DISABLE,
			DEBUG_BREAKPOINT_GET_ALL,
			DEBUG_BREAKPOINT_GET_UPDATES,

			DEBUG_WATCHPOINT_ADD,
			DEBUG_WATCHPOINT_DEL_ALL,
			DEBUG_WATCHPOINT_DEL,
			DEBUG_WATCHPOINT_GET_UPDATES,
			DEBUG_WATCHPOINT_GET_ALL,

		};

        bool CreateGfxContext(System::IntPtr hwnd, GLsizei _viewportW, GLsizei _viewportH);


        HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
            const bool _ramDiskClearAfterRestart);

        ~HAL();
        !HAL();
        
        auto InitShader(System::IntPtr _hWnd,
            System::String^ _vtxShaderS, System::String^ _fragShaderS)
            -> Id;

		auto InitMaterial(
			System::IntPtr _hWnd,
			Id _shaderId,
			cli::array<System::Int32>^ _textureIds,
			cli::array<System::String^>^ _shaderParamNames,
			cli::array<System::Numerics::Vector4>^ _shaderParamValues,
			int _framebufferW, int _framebufferH)
			-> Id;

        auto InitTexture(System::IntPtr _hWnd, const GLsizei _w, const GLsizei _h)
            -> Id;

		void UpdateFrameTexture(System::IntPtr _hWnd, 
			Id _textureId, const bool _vsync);

		auto Draw(System::IntPtr _hWnd, const Id _materialId,
			const GLsizei _viewportW, const GLsizei _viewportH)
			->int;

        ////////////////////////////////////////////////////////////
		// 
        // Requests
        // 
		////////////////////////////////////////////////////////////
        auto Request(const Req _req, System::String^ _dataS)
            -> System::Text::Json::JsonDocument^;

		auto ReqCC() -> uint64_t;
		bool ReqIsRunning();
		void ReqRun();
    };
}

