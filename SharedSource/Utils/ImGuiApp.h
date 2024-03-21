#pragma once
#ifndef DEV_IMGUI_APP_H
#define DEV_IMGUI_APP_H

#include <stdio.h>
#include <string>
#include <atomic>
#include <thread>
#include <Windows.h>

#include "Utils/ImGuiUtils.h"
#include "GLFW/glfw3.h" // Will drag system OpenGL headers
#include "imgui.h"

#include "Utils/JsonUtils.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Stu
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

namespace dev {

    class ImGuiApp
    {
    public:
        enum class AppStatus {
            not_inited,
            inited,
            init_failed,
            create_window_failed,
            opengl_init_failed
        };

        ImGuiApp(nlohmann::json _settingsJ, const std::string& _title = "New Window", int _width = 1280, int _heigth = 720);
        ~ImGuiApp();

        void Run();
        bool Inited() const;

        virtual void Update() {};

    protected:
        HWND m_hWndMain;
        int m_width;
        int m_heigth;
        std::string m_title;
        ImVec4 m_backColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        AppStatus m_status = AppStatus::not_inited;
        std::atomic_bool m_close_req = false;

        static void glfw_error_callback(int _error, const char* _description);
        GLFWwindow* m_window = nullptr;
        ImGuiIO* m_io = nullptr;

        static constexpr float WINDOW_DPI_DEFAULT = 96.0f;
        static constexpr double AUTO_UPDATE_COOLDOWN = 1.0;

        nlohmann::json m_settingsJ;

        ImFont* m_font = nullptr;
        ImFont* m_fontItalic = nullptr;
        float m_fontSize = 10.0f;
        float m_dpiScale = 1.0f;

        enum class REQ : int32_t
        {
            NONE,
            LOAD_FONT,
        };
        std::atomic_char32_t m_req;

        std::thread m_autoUpdateThread;

        // reqs
        void AutoUpdate();
        void Request(const REQ _req);
        void RequestHandler();

        void LoadFonts();
    };
}
#endif // !DEV_IMGUI_APP_H