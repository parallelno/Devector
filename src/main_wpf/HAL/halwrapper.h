#pragma once
#include <string>
#include <windows.h>

#include <iostream>
#include <array>

#include "utils/types.h"
#include "utils/json_utils.h"

#include "core/hardware.h"
#include "core/debugger.h"

#include "utils/gl_utils.h"

#define CONCATENATE(x, y) x##y

namespace dev 
{

    public ref class HAL
    {
        static constexpr float FRAME_PXL_SIZE_W = 1.0f / Display::FRAME_W;
        static constexpr float FRAME_PXL_SIZE_H = 1.0f / Display::FRAME_H;

        Hardware* m_hardwareP;
        Debugger* m_debuggerP;

        GLUtils* m_gl_utilsP;

        GLUtils::Vec4* m_activeArea_pxlSizeP;
        GLUtils::Vec4* m_scrollV_crtXY_highlightMulP;
        GLUtils::Vec4* m_bordsLRTBP;

        GLuint m_vramShaderId = -1;
        GLUtils::MaterialId m_vramMatId;
        GLuint m_vramTexId = -1;
        bool m_isGLInited = false;
        bool m_displayIsHovered = false;
        const char* m_contextMenuName = "##displayCMenu";

        void RenderTextureOnHWND(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH);
        void Render(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH);

        bool DisplayWindowInit();

    public:

        void Init(System::IntPtr hwnd);


        HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
            const bool _ramDiskClearAfterRestart);

        uint64_t GetCC();
        void Run();

        ~HAL();
        !HAL();


        void RenderTexture(System::IntPtr _hwnd, GLsizei _viewportW, GLsizei _viewportH);

    };
}

