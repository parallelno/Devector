#include <stdio.h>
#include <string>

#include "imgui_app.h"

#if defined(_WIN32)
	#include <windows.h>
#elif defined(__linux__)
	#include <X11/Xlib.h>
	#include <X11/Xresource.h>
#endif	

#include "utils/consts.h"
#include "utils/utils.h"

dev::ImGuiApp::ImGuiApp(
		nlohmann::json _settingsJ, const std::string& _settingsPath, const std::string& _title) 
	:
	m_settingsJ(_settingsJ),
	m_title(_title), m_settingsPath(_settingsPath),
	m_status(AppStatus::NOT_INITED)
{
	m_width = dev::GetJsonInt(m_settingsJ, "mainWindowWidth", false, dev::MAIN_WINDOW_W);
	m_height = dev::GetJsonInt(m_settingsJ, "mainWindowHeight", false, dev::MAIN_WINDOW_H);
	m_posX = dev::GetJsonInt(m_settingsJ, "mainWindowX", false, dev::MAIN_WINDOW_X);
	m_posY = dev::GetJsonInt(m_settingsJ, "mainWindowY", false, dev::MAIN_WINDOW_Y);

	m_autoUpdateThread = std::thread(&ImGuiApp::AutoUpdate, this);

	// Setup SDL
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		dev::Log("Error: SDL_Init(): {}\n", SDL_GetError());
		m_error = dev::ErrCode::FAILED_SDL_INIT;
		m_status = AppStatus::EXIT;
		return;
	}

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
	const char* glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
	m_window = SDL_CreateWindow(_title.c_str(), m_width, m_height, window_flags);
	if (m_window == nullptr)
	{
		dev::Log("Error: SDL_CreateWindow(): {}\n", SDL_GetError());
		m_error = dev::ErrCode::FAILED_CREATION_WINDOW;
		m_status = AppStatus::EXIT;
		return;
	}

	SDL_SetWindowPosition(m_window, m_posX, m_posY);
	m_gl_context = SDL_GL_CreateContext(m_window);
	SDL_GL_MakeCurrent(m_window, m_gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	SDL_ShowWindow(m_window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_ioP = &(ImGui::GetIO());
	
	static std::string iniPath = dev::GetExecutableDir() + "imgui.ini";
	m_ioP->IniFilename = iniPath.c_str();

	m_ioP->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	m_ioP->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	m_ioP->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	
	// it works unstable on Linux
#if defined(_WIN32)
	m_ioP->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
#endif
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 3.0f;

	// When viewports are enabled we tweak WindowRounding/WindowBg 
	// so platform windows can look identical to regular ones.
	if (m_ioP->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForOpenGL(m_window, m_gl_context);  
	ImGui_ImplOpenGL3_Init(glsl_version);

	m_status = AppStatus::INITED;
}

// Main loop
void dev::ImGuiApp::Run()
{
	m_status = AppStatus::RUN;

	while (m_status != AppStatus::EXIT)
	{

		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			switch (event.type)
            {
            case SDL_EVENT_QUIT: [[fallthrough]];
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (event.window.windowID == SDL_GetWindowID(m_window))
                {
                    m_status = m_prepare_for_exit ? 
								AppStatus::REQ_PREPARE_FOR_EXIT : AppStatus::EXIT;
                }
                break;
            case SDL_EVENT_DROP_FILE:
                m_droppedFilePath = event.drop.data;
                break;
            }
		}
		if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();

		ReqHandling();

		ImGui::NewFrame();
	   
	   	// Setup docking
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
	
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		// No padding
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		static bool open = true;
		ImGui::Begin("MainWindow", &open, window_flags);

		// Submit the DockSpace
		ImGuiID dockspace_id = ImGui::GetID("MainWindowDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);


		// No padding
		ImGui::PopStyleVar(3);

		m_width = (int)(ImGui::GetWindowWidth());
		m_height = (int)(ImGui::GetWindowHeight());
		auto pos = ImGui::GetWindowPos();
		m_posX = int(pos.x);
		m_posY = int(pos.y);

		Update();
		
		ImGui::End();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)m_ioP->DisplaySize.x, (int)m_ioP->DisplaySize.y);
		glClearColor(m_backColor.x, m_backColor.y, m_backColor.z, m_backColor.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, 
		//so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		if (m_ioP->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
		}

		SDL_GL_SwapWindow(m_window);
	}
}

dev::ImGuiApp::~ImGuiApp()
{
	SettingsSave(m_settingsPath);

	m_autoUpdateThread.join();

	if (m_status != AppStatus::NOT_INITED){
		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();

		SDL_GL_DestroyContext(m_gl_context);
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}
}

void dev::ImGuiApp::AutoUpdate()
{
	for (; m_status != AppStatus::EXIT;
			dev::ThreadSleep(AUTO_UPDATE_COOLDOWN))
	{
		auto currendDpiScale = GetDpiScale();
		bool isDpiUpdated = m_dpiScale != currendDpiScale;
		if (isDpiUpdated)
		{
			Request(Req::LOAD_FONT);
		}
			
		Request(Req::CHECK_WINDOW_SIZE_POS);
	}
}

void dev::ImGuiApp::Request(const Req _req, const int64_t _val)
{
	m_reqs.push({ _req, _val});
}

void dev::ImGuiApp::ReqHandling()
{
	if (m_reqs.empty()) return;

	auto result = m_reqs.pop();
	auto req = result->first;
	auto val = result->second;

	switch (req)
	{
	case Req::LOAD_FONT:
		LoadFonts();
		return;
	case Req::CHECK_WINDOW_SIZE_POS:
	{
		auto width = GetSettingsInt("mainWindowWidth", dev::MAIN_WINDOW_W);
		auto height = GetSettingsInt("mainWindowHeight", dev::MAIN_WINDOW_H);

		if (width != m_width || height != m_height)
		{
			SettingsUpdate("mainWindowWidth", m_width);
			SettingsUpdate("mainWindowHeight", m_height);

			SettingsSave(m_settingsPath);
		}

		auto posX = GetSettingsInt("mainWindowX", dev::MAIN_WINDOW_X);
		auto posY = GetSettingsInt("mainWindowY", dev::MAIN_WINDOW_Y);

		if (posX != m_posX || posY != m_posY)
		{
			SettingsUpdate("mainWindowX", m_posX);
			SettingsUpdate("mainWindowY", m_posY);

			SettingsSave(m_settingsPath);
		}
		return;
	}
	};
}

void dev::ImGuiApp::LoadFonts()
{

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();

	//io.Fonts->AddFontDefault(); // adds a default ImGui font.

	m_dpiScale = GetDpiScale();

	auto fontCodePath = dev::GetExecutableDir() + dev::GetJsonString(m_settingsJ, "fontPath", false, DEFAULT_FONT_PATH);
	auto fontSize = (float)dev::GetJsonDouble(m_settingsJ, "fontSize", false, DEFAULT_FONT_SIZE) * m_dpiScale;

	if (!fontCodePath.empty() && dev::IsFileExist(fontCodePath))
	{
		m_font = io.Fonts->AddFontFromFileTTF(fontCodePath.c_str(), fontSize);
	}

	auto fontCommentPath = dev::GetExecutableDir() + dev::GetJsonString(m_settingsJ, "fontItalicPath", false, DEFAULT_FONT_ITALIC_PATH);
	auto fontCommentSize = (float)dev::GetJsonDouble(m_settingsJ, "fontItalicSize", false, DEFAULT_FONT_ITALIC_SIZE);

	if (!fontCommentPath.empty() && dev::IsFileExist(fontCommentPath))
	{
		m_fontItalic = io.Fonts->AddFontFromFileTTF(fontCommentPath.c_str(), fontCommentSize);
	}

	ImGui_ImplOpenGL3_CreateFontsTexture();
}

void dev::ImGuiApp::SettingsUpdate(const std::string& _fieldName, nlohmann::json _json)
{
	std::lock_guard<std::mutex> mlock(m_settingsMutex);
	m_settingsJ[_fieldName] = _json;
}

void dev::ImGuiApp::SettingsSave(const std::string& _path)
{
	std::lock_guard<std::mutex> mlock(m_settingsMutex);
	SaveJson(_path, m_settingsJ);
}

auto dev::ImGuiApp::GetSettingsString(const std::string& _fieldName, const std::string& _defaultValue)
-> std::string
{
	std::lock_guard<std::mutex> mlock(m_settingsMutex);
	return dev::GetJsonString(m_settingsJ, _fieldName, false, _defaultValue);
}

auto dev::ImGuiApp::GetSettingsObject(const std::string& _fieldName)
-> nlohmann::json
{
	std::lock_guard<std::mutex> mlock(m_settingsMutex);
	return dev::GetJsonObject(m_settingsJ, _fieldName, false);
}

int dev::ImGuiApp::GetSettingsInt(const std::string& _fieldName, int _defaultValue)
{
	std::lock_guard<std::mutex> mlock(m_settingsMutex);
	return dev::GetJsonInt(m_settingsJ, _fieldName, false, _defaultValue);
}

bool dev::ImGuiApp::GetSettingsBool(const std::string& _fieldName, bool _defaultValue)
{
	std::lock_guard<std::mutex> mlock(m_settingsMutex);
	return dev::GetJsonBool(m_settingsJ, _fieldName, false, _defaultValue);
}

#if defined(_WIN32)
    #if defined(__MINGW32__) || defined(__MINGW64__)
        #include <VersionHelpers.h>
        #define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
        UINT GetDpiForWindowFallback(HWND hwnd) {
            // Approximation for DPI when `GetDpiForWindow` is unavailable.
            HDC hdc = GetDC(hwnd);
            UINT dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(hwnd, hdc);
            return dpi;
        }
        #define GetDpiForWindow(hwnd) GetDpiForWindowFallback(hwnd)
    #endif
#endif

float dev::ImGuiApp::GetDpiScale()
{
	static constexpr float WINDOW_DPI_DEFAULT = 96.0f;
#if defined(_WIN32)
    HWND m_hWndMain = GetActiveWindow();
	if (m_hWndMain) 
	{
		return GetDpiForWindow(m_hWndMain) / WINDOW_DPI_DEFAULT;
	}
	else {
		return 1.0f;
	}
#elif defined(__linux__)
	   ::Display* dysplayP = XOpenDisplay(NULL);
    if (!dysplayP) {
        std::cerr << "Unable to open X display." << std::endl;
        return 1.0f; // Default scaling factor
    }

    // Get the screen DPI
    char* rms = XResourceManagerString(dysplayP);
    XrmDatabase db;
    XrmValue value;
    char* type = NULL;

    XrmInitialize(); // Initialize X resource manager
    db = XrmGetStringDatabase(rms);

    // Query the DPI from Xft resource (Xft.dpi)
    if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
        float dpi = atof(value.addr);
        XCloseDisplay(dysplayP);
        return dpi / 96.0f; // 96 DPI is the default scale factor
    }

    XCloseDisplay(dysplayP);
    return 1.0f; // Default scaling factor
#endif
}
