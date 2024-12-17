#pragma once
#include <string>
#include <windows.h>

#include <iostream>
#include <array>

#include <windows.h>

#include "utils/types.h"
#include "utils/consts.h"
#include "utils/json_utils.h"

#include "core/hardware.h"
#include "core/debugger.h"

#include "win_gl_utils.h"

namespace dev 
{

    public ref class HAL
    {
        Hardware* m_hardwareP;
        Debugger* m_debuggerP;
        WinGlUtils* m_winGlUtilsP;

    public:
		literal float FRAME_PXL_SIZE_W = 1.0f / Display::FRAME_W;
		literal float FRAME_PXL_SIZE_H = 1.0f / Display::FRAME_H;
		literal float SCANLINE_HIGHLIGHT_MUL = 0.3f;

		literal float ACTIVE_AREA_W = Display::ACTIVE_AREA_W;
		literal float ACTIVE_AREA_H = Display::ACTIVE_AREA_H;
		literal float SCAN_ACTIVE_AREA_TOP = Display::SCAN_ACTIVE_AREA_TOP;
		literal float BORDER_VISIBLE = Display::BORDER_VISIBLE;

		//enum class Req
		#include "core/hardware_consts.h"

		#include "core/memory_consts.h"

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

		auto GetMaterialParamId(System::IntPtr _hWnd,
			const Id _materialId, System::String^ _paramName)
			-> Id;

		auto UpdateMaterialParam(System::IntPtr _hWnd,
			const Id _materialId, const Id _paramId,
			const System::Numerics::Vector4^ _paramVal)
			-> int;

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
		int LoadRom(System::String^ _path);
		int LoadFdd(System::String^ _path, const int _driveIdx, const bool _autoBoot);
		int LoadRecording(System::String^ _path);
    };
}

