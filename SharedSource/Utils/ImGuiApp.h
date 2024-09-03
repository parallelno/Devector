#pragma once

#include <stdio.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <Windows.h>

#include "imgui.h"
#include "GLFW/glfw3.h" // Will drag system OpenGL headers

#include "JsonUtils.h"

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
            NOT_INITED,
            FAILED_INIT,
            FAILED_CREATE_WINDOW,
            FAILED_OPENGL_INIT,
            INITED,
            RUN,
            CHECK_MOUNTED_FDDS, // check/save mounted updated ffds
            WAIT_FOR_SAVING,
            EXIT,
        };

        ImGuiApp(nlohmann::json _settingsJ, const std::string& _stringPath, const std::string& _title = "New Window");
        ~ImGuiApp();

        void Run();
        bool IsInited() const { return m_status == AppStatus::INITED || m_status == AppStatus::RUN; };

        virtual void Update() {};

    protected:
        const std::string m_stringPath;

        HWND m_hWndMain;
        int m_width;
        int m_height;
        int m_posX;
        int m_posY;
        std::string m_title;
        ImVec4 m_backColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
        AppStatus m_status = AppStatus::NOT_INITED;

        static void glfw_error_callback(int _error, const char* _description);
        GLFWwindow* m_window = nullptr;
        ImGuiIO* m_io = nullptr;

        static constexpr float WINDOW_DPI_DEFAULT = 96.0f;
        static constexpr double AUTO_UPDATE_COOLDOWN = 1.0;

        ImFont* m_font = nullptr;
        ImFont* m_fontItalic = nullptr;
        float m_fontSize = 10.0f;
        float m_dpiScale = 1.0f;

        enum class REQ : int32_t
        {
            NONE,
            LOAD_FONT,
            CHECK_WINDOW_SIZE_POS,
        };
        std::atomic_char32_t m_req;

        std::thread m_autoUpdateThread;

        // reqs
        void AutoUpdate();
        void Request(const REQ _req);
        void RequestHandler();
        void LoadFonts();
        void SettingsUpdate(const std::string& _fieldName, nlohmann::json _json);
        void SettingsSave(const std::string& _path);
        auto GetSettingsString(const std::string& _fieldName, const std::string& _defaultValue) -> std::string;
        auto GetSettingsObject(const std::string& _fieldName ) -> nlohmann::json;
        int GetSettingsInt(const std::string& _fieldName, int _defaultValue);
        bool GetSettingsBool(const std::string& _fieldName, bool _defaultValue);

    private:
        std::mutex m_settingsMutex;
        nlohmann::json m_settingsJ;
    };
}