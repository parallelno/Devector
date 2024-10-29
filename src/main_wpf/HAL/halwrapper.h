#pragma once
#include <string>
#include <windows.h>

#include <iostream>
#include <array>

#include "utils/types.h"
#include "utils/json_utils.h"

#include "core/hardware.h"
#include "core/debugger.h"

namespace dev 
{

    public ref class HAL
    {
        Hardware* m_hardwareP;
        Debugger* m_debuggerP;

        void RenderTextureOnHWND(HWND hWnd);

    public:

        void Init();


        HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
            const bool _ramDiskClearAfterRestart);

        uint64_t GetCC();
        void Run();

        ~HAL();
        !HAL();


        void RenderTexture(System::IntPtr hwnd)
        {
            HWND hWnd = static_cast<HWND>(hwnd.ToPointer());
            RenderTextureOnHWND(hWnd);
        }

    };
}

