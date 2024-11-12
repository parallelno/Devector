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

#define CONCATENATE(x, y) x##y

namespace dev 
{

    public ref class HAL
    {
        static constexpr float FRAME_PXL_SIZE_W = 1.0f / Display::FRAME_W;
        static constexpr float FRAME_PXL_SIZE_H = 1.0f / Display::FRAME_H;
        static constexpr float SCANLINE_HIGHLIGHT_MUL = 0.3f;

        Hardware* m_hardwareP;
        Debugger* m_debuggerP;
        WinGlUtils* m_winGlUtilsP;

        GLUtils::Vec4* m_activeArea_pxlSizeP;
        GLUtils::Vec4* m_scrollV_crtXY_highlightMulP;
        GLUtils::Vec4* m_bordsLRTBP;

        int64_t m_ccLast = -1; // to force the first stats update
        int64_t m_ccLastRun = 0;

        GLuint m_vramShaderId = -1;
        GLUtils::MaterialId m_vramMatId;
        GLuint m_vramTexId = -1;
        bool m_isGLInited = false;
        bool m_displayIsHovered = false;
        const char* m_contextMenuName = "##displayCMenu";
        int m_rasterPixel = 0;
        int m_rasterLine = 0;

        HWND m_hwnd_temp = nullptr;

        bool DisplayWindowInit(const GLsizei _viewportW, const GLsizei _viewportH);
        

    public:

        void Init(System::IntPtr hwnd, GLsizei _viewportW, GLsizei _viewportH);


        HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
            const bool _ramDiskClearAfterRestart);

        uint64_t GetCC();
        void Run();

        ~HAL();
        !HAL();

        void UpdateData(const bool _isRunning,
            const GLsizei _viewportW, const GLsizei _viewportH);
        void DrawDisplay(const GLsizei _viewportW, const GLsizei _viewportH);

    };
}

