#pragma once
#include "imgui_impl_glfw.h"

#include <stdio.h>
#include <string>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Stu
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

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

    ImGuiApp(const std::string& title = "New Window", int width = 1280, int m_heigth = 720);
	~ImGuiApp();

    void Run();
    bool Inited() const;

    virtual void Update() {};

    int m_width;
    int m_heigth;
    std::string m_title;
    ImVec4 m_backColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    AppStatus m_status = AppStatus::not_inited;
private:
    static void glfw_error_callback(int error, const char* description);
    GLFWwindow* m_window = nullptr;
    ImGuiIO* m_io = nullptr;
};
