#include <stdio.h>
#include <string>

#include "utils/imgui_app.h"

// #include "imgui_impl_glfw.h"
// #include "imgui_impl_opengl3.h"

#include "utils/consts.h"
#include "utils/utils.h"

dev::ImGuiApp::ImGuiApp(
		nlohmann::json _settingsJ, const std::string& _stringPath, const std::string& _title) 
	:
	m_settingsJ(_settingsJ),
	m_title(_title), m_stringPath(_stringPath),
	m_status(AppStatus::NOT_INITED)
{
	m_width = dev::GetJsonInt(m_settingsJ, "mainWindowWidth", false, dev::MAIN_WINDOW_W);
	m_height = dev::GetJsonInt(m_settingsJ, "mainWindowHeight", false, dev::MAIN_WINDOW_H);
	m_posX = dev::GetJsonInt(m_settingsJ, "mainWindowX", false, dev::MAIN_WINDOW_X);
	m_posY = dev::GetJsonInt(m_settingsJ, "mainWindowY", false, dev::MAIN_WINDOW_Y);

	m_autoUpdateThread = std::thread(&ImGuiApp::AutoUpdate, this);

	// Setup SDL
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMEPAD))
	{
		dev::Log("Error: SDL_Init(): %s\n", SDL_GetError());
		m_status = AppStatus::FAILED_INIT;
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
		dev::Log("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		m_status = AppStatus::FAILED_CREATE_WINDOW;
		return;
	}

	//{
	//	int displayIndex = SDL_GetDisplayForWindow(m_window);
	//	// Variables to store the display bounds (including screen width and height)
	//	SDL_Rect displayBounds;
	//	
	//	// Get the bounds of the display that corresponds to the displayIndex
	//	if (SDL_GetDisplayBounds(displayIndex, &displayBounds) != 0) {
	//		// Error handling in case the display bounds couldn't be retrieved
	//		dev::Log("SDL_GetDisplayBounds failed: {}", SDL_GetError());
	//		return;
	//	}

	//	// Store the screen width and height from the display bounds
	//	int screenWidth = displayBounds.w;
	//	int screenHeight = displayBounds.h;

	//	// Center the window if position is negative
	//	m_posX = (m_posX < 0) ? (screenWidth - m_width) / 2 : m_posX;
	//	m_posY = (m_posY < 0) ? (screenHeight - m_height) / 2 : m_posY;
	//}
	SDL_SetWindowPosition(m_window, m_posX, m_posY);

	// Initialize GLAD
	//if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	//{
	//	dev::Log("Failed to initialize GLAD\n");
	//	m_status = AppStatus::FAILED_INIT;
	//	return;
	//}

	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	m_gl_context = SDL_GL_CreateContext(m_window);
	SDL_GL_MakeCurrent(m_window, m_gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync
	SDL_ShowWindow(m_window);

	m_status = AppStatus::INITED;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	//ImPlot::CreateContext();
	m_io = &(ImGui::GetIO());
	//(void)m_io;
	m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	m_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	m_io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (m_io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForOpenGL(m_window, m_gl_context);  
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	m_hWndMain = GetActiveWindow();
}

dev::ImGuiApp::~ImGuiApp()
{
	SettingsSave(m_stringPath);

	m_autoUpdateThread.join();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DestroyContext(m_gl_context);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

void dev::ImGuiApp::glfw_error_callback(int _error, const char* _description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", _error, _description);
}

// Main loop
void dev::ImGuiApp::Run()
{
	m_status = AppStatus::RUN;
	bool window_should_close = false;

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
			if (event.type == SDL_EVENT_QUIT)
				window_should_close = true;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && 
				event.window.windowID == SDL_GetWindowID(m_window)
				)
				window_should_close = true;
		}
		if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();

		RequestHandler();

		ImGui::NewFrame();
	   
		
		//ImGui::DockSpaceOverViewport();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		//if (true)
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
		static bool open = true;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("MainWindow", &open, window_flags);

		m_width = (int)(ImGui::GetWindowWidth());
		m_height = (int)(ImGui::GetWindowHeight());
		auto size = ImGui::GetWindowPos();
		m_posX = int(size.x);
		m_posY = int(size.y);

		ImGui::PopStyleVar();

		// Create a DockSpace node where any window can be docked
		ImGuiID dockspace_id = ImGui::GetID("MainWindowDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

		Update();
		
		ImGui::End();
		ImGui::PopStyleVar(2);

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)m_io->DisplaySize.x, (int)m_io->DisplaySize.y);
		glClearColor(m_backColor.x * m_backColor.w, 
					m_backColor.y * m_backColor.w, 
					m_backColor.z * m_backColor.w, 
					m_backColor.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, 
		//so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		if (m_io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
		// 	GLFWwindow* backup_current_context = glfwGetCurrentContext();
		 	ImGui::UpdatePlatformWindows();
		 	ImGui::RenderPlatformWindowsDefault();
		// 	glfwMakeContextCurrent(backup_current_context);
		}

		SDL_GL_SwapWindow(m_window);

		m_status = m_status != AppStatus::WAIT_FOR_SAVING && 
			m_status != AppStatus::EXIT &&
			window_should_close ? AppStatus::CHECK_MOUNTED_FDDS : m_status;
	}
}

void dev::ImGuiApp::AutoUpdate()
{
	for (; m_status != AppStatus::EXIT; dev::ThreadSleep(AUTO_UPDATE_COOLDOWN))
	{
		if (m_hWndMain) {
			auto currendDpiScale = GetDpiForWindow(m_hWndMain) / WINDOW_DPI_DEFAULT;
			bool isDpiUpdated = m_dpiScale != currendDpiScale;
			if (m_req == static_cast<int32_t>(REQ::NONE) && isDpiUpdated)
			{
				Request(REQ::LOAD_FONT);
			}
			
			Request(REQ::CHECK_WINDOW_SIZE_POS);
		}

	}
}

void dev::ImGuiApp::Request(const REQ _req)
{
	m_req = static_cast<int32_t>(_req);
}

void dev::ImGuiApp::RequestHandler()
{
	auto req = static_cast<REQ>(static_cast<int32_t>(m_req));

	switch (req)
	{
	case REQ::LOAD_FONT:
		LoadFonts();
		break;
	case REQ::CHECK_WINDOW_SIZE_POS:
	{
		auto width = GetSettingsInt("mainWindowWidth", dev::MAIN_WINDOW_W);
		auto height = GetSettingsInt("mainWindowHeight", dev::MAIN_WINDOW_H);

		if (width != m_width || height != m_height)
		{
			SettingsUpdate("mainWindowWidth", m_width);
			SettingsUpdate("mainWindowHeight", m_height);

			SettingsSave(m_stringPath);
		}

		auto posX = GetSettingsInt("mainWindowX", dev::MAIN_WINDOW_X);
		auto posY = GetSettingsInt("mainWindowY", dev::MAIN_WINDOW_Y);

		if (posX != m_posX || posY != m_posY)
		{
			SettingsUpdate("mainWindowX", m_posX);
			SettingsUpdate("mainWindowY", m_posY);

			SettingsSave(m_stringPath);
		}
		break;
	}
	default:
		return; 
	};

	m_req = static_cast<int32_t>(REQ::NONE);
}

void dev::ImGuiApp::LoadFonts()
{

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();

	//io.Fonts->AddFontDefault(); // adds a default ImGui font.

	m_dpiScale = GetDpiForWindow(m_hWndMain) / WINDOW_DPI_DEFAULT;

	auto fontCodePath = dev::GetJsonString(m_settingsJ, "fontPath", false, DEFAULT_FONT_PATH);
	m_fontSize = (float)dev::GetJsonDouble(m_settingsJ, "fontSize", false, DEFAULT_FONT_SIZE) * m_dpiScale;

	if (!fontCodePath.empty() && dev::IsFileExist(fontCodePath))
	{
		m_font = io.Fonts->AddFontFromFileTTF(fontCodePath.c_str(), m_fontSize);
	}

	auto fontCommentPath = dev::GetJsonString(m_settingsJ, "fontItalicPath", false, DEFAULT_FONT_ITALIC_PATH);
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
	return dev::GetJsonObject(m_settingsJ, "recentFiles", false);
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