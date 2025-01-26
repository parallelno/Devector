#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstdint>
#include <codecvt>
#include "libtinyfiledialogs/tinyfiledialogs.h"

#include "devector_app.h"

#include "utils/utils.h"
#include "utils/json_utils.h"
#include "utils/str_utils.h"
#include "core/fdd_consts.h"

dev::DevectorApp::DevectorApp(
	const std::string& _settingsPath, nlohmann::json _settingsJ,
	const std::string& _rom_fdd_recPath)
	:
	ImGuiApp(_settingsJ, _settingsPath, APP_NAME),
	m_glUtils(true)
{
	if (m_status == AppStatus::INITED) {
		Init(_rom_fdd_recPath);
	}
}

void dev::DevectorApp::Init(const std::string& _rom_fdd_recPath)
{
	SettingsInit();
	HardwareInit();
	WindowsInit();
	Load(_rom_fdd_recPath);

	// Set up the event callback function
	SDL_SetEventFilter(DevectorApp::EventFilter, this);

	m_hardwareP->Request(Hardware::Req::RUN);
}

dev::DevectorApp::~DevectorApp()
{
	m_debuggerP->GetDebugData().SaveDebugData();

	SettingsUpdate("breakpointsWindowVisisble", m_breakpointsWindowVisisble);
	SettingsUpdate("hardwareStatsWindowVisible", m_hardwareStatsWindowVisible);
	SettingsUpdate("disasmWindowVisible", m_disasmWindowVisible);
	SettingsUpdate("watchpointsWindowVisible", m_watchpointsWindowVisible);
	SettingsUpdate("displayWindowVisible", m_displayWindowVisible);
	SettingsUpdate("memDisplayWindowVisible", m_memDisplayWindowVisible);
	SettingsUpdate("hexViewerWindowVisible", m_hexViewerWindowVisible);
	SettingsUpdate("traceLogWindowVisible", m_traceLogWindowVisible);
	SettingsUpdate("recorderWindowVisible", m_recorderWindowVisible);
	SettingsUpdate("keyboardWindowVisible", m_keyboardWindowVisible);
	SettingsUpdate("searchWindowVisible", m_searchWindowVisible);
	SettingsUpdate("debugdataWindowVisible", m_debugdataWindowVisible);
}

void dev::DevectorApp::HardwareInit()
{
	std::string pathBootData = GetSettingsString("bootPath", "boot//boot.bin");
	m_restartOnLoadFdd = GetSettingsBool("restartOnLoadFdd", true);
	m_ramDiskClearAfterRestart = GetSettingsBool("ramDiskClearAfterRestart", false);
	m_ramDiskDataPath = GetSettingsString("ramDiskDataPath", "ramDisks.bin");

	m_hardwareP = std::make_unique < dev::Hardware>(pathBootData, m_ramDiskDataPath, m_ramDiskClearAfterRestart);
	m_debuggerP = std::make_unique < dev::Debugger>(*m_hardwareP);
}

void dev::DevectorApp::WindowsInit()
{
	m_hardwareStatsWindowP = std::make_unique<dev::HardwareStatsWindow>(*m_hardwareP, &m_dpiScale, m_ruslat);
	m_disasmWindowP = std::make_unique<dev::DisasmWindow>(*m_hardwareP, *m_debuggerP, 
		m_fontItalic, &m_dpiScale, m_reqUI);
	m_displayWindowP = std::make_unique<dev::DisplayWindow>(*m_hardwareP, &m_dpiScale, m_glUtils, m_reqUI);
	m_breakpointsWindowP = std::make_unique<dev::BreakpointsWindow>(*m_hardwareP, &m_dpiScale, m_reqUI);
	m_watchpointsWindowP = std::make_unique<dev::WatchpointsWindow>(*m_hardwareP, &m_dpiScale, m_reqUI);
	m_memDisplayWindowP = std::make_unique<dev::MemDisplayWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_glUtils, m_reqUI);
	m_hexViewerWindowP = std::make_unique<dev::HexViewerWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
	m_traceLogWindowP = std::make_unique<dev::TraceLogWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
	m_aboutWindowP = std::make_unique<dev::AboutWindow>(&m_dpiScale);
	m_feedbackWindowP = std::make_unique<dev::FeedbackWindow>(&m_dpiScale);
	m_recorderWindowP = std::make_unique<dev::RecorderWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
	m_keyboardWindowP = std::make_unique<dev::KeyboardWindow>(*m_hardwareP, &m_dpiScale, m_glUtils, m_reqUI, m_pathImgKeyboard);
	m_searchWindowP = std::make_unique<dev::SearchWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
	m_debugdataWindowP = std::make_unique<dev::DebugDataWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
}

void dev::DevectorApp::SettingsInit()
{
	Request(Req::LOAD_FONT);
	AppStyleInit();

	m_breakpointsWindowVisisble = GetSettingsBool("breakpointsWindowVisisble", false);
	m_hardwareStatsWindowVisible = GetSettingsBool("hardwareStatsWindowVisible", false);
	m_disasmWindowVisible = GetSettingsBool("disasmWindowVisible", false);
	m_watchpointsWindowVisible = GetSettingsBool("watchpointsWindowVisible", false);
	m_displayWindowVisible = GetSettingsBool("displayWindowVisible", true);
	m_memDisplayWindowVisible = GetSettingsBool("memDisplayWindowVisible", false);
	m_hexViewerWindowVisible = GetSettingsBool("hexViewerWindowVisible", false);
	m_traceLogWindowVisible = GetSettingsBool("traceLogWindowVisible", false);
	m_recorderWindowVisible = GetSettingsBool("recorderWindowVisible", false);
	m_keyboardWindowVisible = GetSettingsBool("keyboardWindowVisible", false);
	m_searchWindowVisible = GetSettingsBool("searchWindowVisible", false);
	m_debugdataWindowVisible = GetSettingsBool("debugdataWindowVisible", false);

	m_pathImgKeyboard = GetSettingsString("pathImgKeyboard", "images//vector_keyboard.jpg");

	RecentFilesInit();
	
	m_mountRecentFddImg = GetSettingsBool("m_mountRecentFddImg", true);
	if (m_mountRecentFddImg) m_reqUI.type = ReqUI::Type::LOAD_RECENT_FDD_IMG;
}

void dev::DevectorApp::Load(const std::string& _rom_fdd_recPath)
{
	// load the rom/fdd/rec image if it was send via the console command
	if (_rom_fdd_recPath.empty()) return;

	bool isRunning = m_hardwareP->Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	if (isRunning) m_hardwareP->Request(Hardware::Req::STOP);

	auto path = _rom_fdd_recPath;
	auto ext = StrToUpper(dev::GetExt(path));

	if (ext == EXT_ROM)
	{
		RecentFilesUpdate(FileType::ROM, path);
	}
	else if (ext == EXT_FDD)
	{
		RecentFilesUpdate(FileType::FDD, path, 0, true);
	}
	else if (ext == EXT_REC)
	{
		LoadRecording(path);
	}
	else {
		dev::Log("Unsupported file type: {}", path);
		m_loadingRes.state = LoadingRes::State::NONE;
		return;
	}

	RecentFilesStore();
	Reload();

	if (isRunning) m_hardwareP->Request(Hardware::Req::RUN);
}

// UI thread
void dev::DevectorApp::Update()
{
	ReqUIHandling();
	
	MainMenuUpdate();
	
	DebugAttach();

	LoadingResStatusHandling();

	bool isRunning = m_hardwareP->Request(Hardware::Req::IS_RUNNING)->at("isRunning");

	m_hardwareStatsWindowP->Update(m_hardwareStatsWindowVisible, isRunning);
	m_disasmWindowP->Update(m_disasmWindowVisible, isRunning);
	m_displayWindowP->Update(m_displayWindowVisible, isRunning);
	m_breakpointsWindowP->Update(m_breakpointsWindowVisisble, isRunning);
	m_watchpointsWindowP->Update(m_watchpointsWindowVisible, isRunning);
	m_memDisplayWindowP->Update(m_memDisplayWindowVisible, isRunning);
	m_hexViewerWindowP->Update(m_hexViewerWindowVisible, isRunning);
	m_traceLogWindowP->Update(m_traceLogWindowVisible, isRunning);
	m_aboutWindowP->Update(m_aboutWindowVisible, isRunning);
	m_feedbackWindowP->Update(m_feedbackWindowVisible, isRunning);
	m_recorderWindowP->Update(m_recorderWindowVisible, isRunning);
	m_keyboardWindowP->Update(m_keyboardWindowVisible, isRunning);
	m_searchWindowP->Update(m_searchWindowVisible, isRunning);
	m_debugdataWindowP->Update(m_debugdataWindowVisible, isRunning);

	// context menues to edit the debug data
	DrawEditLabelWindow(*m_hardwareP, m_debuggerP->GetDebugData(), m_reqUI);
	DrawEditConstWindow(*m_hardwareP, m_debuggerP->GetDebugData(), m_reqUI);
	DrawEditCommentWindow(*m_hardwareP, m_debuggerP->GetDebugData(), m_reqUI);
	DrawEditMemEditWindow(*m_hardwareP, m_debuggerP->GetDebugData(), m_reqUI);


	if (m_status == AppStatus::REQ_PREPARE_FOR_EXIT)
	{
		m_loadingRes.Init(LoadingRes::State::CHECK_MOUNTED, LoadingRes::Type::SAVE_THEN_EXIT);
		m_status = AppStatus::PREPARE_FOR_EXIT;
	}

	if (m_restartOnLoadFdd) RestartOnLoadFdd();
}

// auto press ruslat after loading fdd
void dev::DevectorApp::RestartOnLoadFdd()
{
	// ruslat
	auto ruslatHistoryJ = *m_hardwareP->Request(Hardware::Req::GET_RUSLAT_HISTORY);
	auto m_ruslatHistory = ruslatHistoryJ["data"].get<uint32_t>();
	bool newRusLat = (m_ruslatHistory & 0b1000) != 0;

	if (newRusLat != m_ruslat) {
		if (m_rustLatSwitched++ > 2)
		{
			m_rustLatSwitched = 0;
			auto romEnabledJ = *m_hardwareP->Request(Hardware::Req::IS_MEMROM_ENABLED);
			if (romEnabledJ["data"]) {
				m_hardwareP->Request(Hardware::Req::RESTART);
			}
		}
	}
	m_ruslat = (m_ruslatHistory & 0b1000) != 0;
}

void dev::DevectorApp::Reload()
{
	if (m_recentFilePaths.empty()) return;
	// get latest recent path
	const auto& [fileType, path, driveIdx, autoBoot] = m_recentFilePaths.front();
	
	switch (fileType) {
	case FileType::ROM:
		LoadRom(path);
		break;

	case FileType::FDD:
		LoadFdd(path, driveIdx, autoBoot);
		break;

	case FileType::REC:
		LoadRecording(path);
		break;
	}
}

void dev::DevectorApp::MainMenuUpdate()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				m_debuggerP->GetDebugData().SaveDebugData();
				m_loadingRes.Init(LoadingRes::State::CHECK_MOUNTED, LoadingRes::Type::OPEN_FILE_DIALOG);
			}
			if (ImGui::BeginMenu("Recent Files"))
			{
				for (const auto& [fileType, path, driveIdx, autoBoot] : m_recentFilePaths)
				{
					std::string itemS = path;
					if (fileType == FileType::FDD)
					{
						itemS += std::format(":{}", driveIdx);
						itemS += autoBoot ? "A" : "";
					}

					if (ImGui::MenuItem(itemS.c_str()))
					{
						m_debuggerP->GetDebugData().SaveDebugData();
						m_loadingRes.Init(LoadingRes::State::CHECK_MOUNTED, LoadingRes::Type::RECENT, fileType, path, driveIdx, autoBoot);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Save Rec"))
			{
				m_loadingRes.Init(LoadingRes::State::CHECK_MOUNTED, LoadingRes::Type::SAVE_REC_FILE_DIALOG, FileType::REC);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Quit", "Alt+F4")) { m_status = AppStatus::REQ_PREPARE_FOR_EXIT; }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools"))
		{
			ImGui::MenuItem(m_displayWindowP->m_name.c_str(), NULL, &m_displayWindowVisible);
			ImGui::MenuItem(m_hardwareStatsWindowP->m_name.c_str(), NULL, &m_hardwareStatsWindowVisible);
			ImGui::MenuItem(m_disasmWindowP->m_name.c_str(), NULL, &m_disasmWindowVisible);
			ImGui::MenuItem(m_breakpointsWindowP->m_name.c_str(), NULL, &m_breakpointsWindowVisisble);
			ImGui::MenuItem(m_watchpointsWindowP->m_name.c_str(), NULL, &m_watchpointsWindowVisible);
			ImGui::MenuItem(m_memDisplayWindowP->m_name.c_str(), NULL, &m_memDisplayWindowVisible);
			ImGui::MenuItem(m_hexViewerWindowP->m_name.c_str(), NULL, &m_hexViewerWindowVisible);
			ImGui::MenuItem(m_traceLogWindowP->m_name.c_str(), NULL, &m_traceLogWindowVisible);
			ImGui::MenuItem(m_recorderWindowP->m_name.c_str(), NULL, &m_recorderWindowVisible);
			ImGui::MenuItem(m_keyboardWindowP->m_name.c_str(), NULL, &m_keyboardWindowVisible);
			ImGui::MenuItem(m_searchWindowP->m_name.c_str(), NULL, &m_searchWindowVisible);
			ImGui::MenuItem(m_debugdataWindowP->m_name.c_str(), NULL, &m_debugdataWindowVisible);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			//ImGui::MenuItem(m_feedbackWindowP->m_name.c_str(), NULL, &m_feedbackWindowVisible);
			if (ImGui::MenuItem("zx-pk.ru: Vector06C Development"))
			{
				dev::OsOpenInShell("https://zx-pk.ru/threads/34480-programmirovanie.html");
			}
			if (ImGui::MenuItem("Vector06C Software Catalog"))
			{
				dev::OsOpenInShell("https://caglrc.cc/scalar/recent20/");
			}
			if (ImGui::MenuItem("Telegram Channel: Guides & Updates"))
			{
				dev::OsOpenInShell("https://t.me/devector06C");
			}
			if (ImGui::MenuItem("zx-pk.ru: Devector Discussions"))
			{
				dev::OsOpenInShell("https://zx-pk.ru/threads/35808-devector-emulyator-kompyutera-vektor-06ts.html");
			}
			ImGui::MenuItem(m_aboutWindowP->m_name.c_str(), NULL, &m_aboutWindowVisible);
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void dev::DevectorApp::LoadingResStatusHandling()
{
	// handle the statuses
	switch (m_loadingRes.state)
	{
	case LoadingRes::State::CHECK_MOUNTED:
		CheckMountedFdd();
		break;

	case LoadingRes::State::SAVE_DISCARD:
		SaveDiscardFdd();
		break;

	case LoadingRes::State::OPEN_POPUP_SAVE_DISCARD:
		ImGui::OpenPopup(m_loadingRes.POPUP_SAVE_DISCARD);
		m_loadingRes.state = LoadingRes::State::POPUP_SAVE_DISCARD;
		break;

	case LoadingRes::State::POPUP_SAVE_DISCARD:
		DrawSaveDiscardFddPopup();
		break;

	case LoadingRes::State::ALWAYS_DISCARD:
		SettingsUpdate("showSaveDiscardFddDialog", false);
		SettingsUpdate("discardFddChanges", true);
		[[fallthrough]];
	case LoadingRes::State::DISCARD:
		m_loadingRes.state = LoadingRes::State::OPEN_FILE;
		break;

	case LoadingRes::State::ALWAYS_SAVE:
		SettingsUpdate("showSaveDiscardFddDialog", false);
		SettingsUpdate("discardFddChanges", false);
		[[fallthrough]];
	case LoadingRes::State::SAVE:
		SaveUpdatedFdd();
		m_loadingRes.state = LoadingRes::State::OPEN_FILE;
		break;

	case LoadingRes::State::OPEN_FILE: 
	{
		switch (m_loadingRes.type)
		{
		case LoadingRes::Type::RECENT:
			m_loadingRes.state = LoadingRes::State::LOAD;
			break;

		case LoadingRes::Type::SAVE_THEN_EXIT:
			m_status = AppStatus::EXIT;
			m_loadingRes.state = LoadingRes::State::EXIT;
			break;

		case LoadingRes::Type::SAVE_REC_FILE_DIALOG:
			SaveFile();
			break;

		case LoadingRes::Type::OPEN_FILE_DIALOG:
			OpenFile();
			break;
		}

		break;
	}
	case LoadingRes::State::OPEN_POPUP_SELECT_DRIVE:
		ImGui::OpenPopup(m_loadingRes.POPUP_SELECT_DRIVE);
		m_loadingRes.state = LoadingRes::State::POPUP_SELECT_DRIVE;
		break;

	case LoadingRes::State::POPUP_SELECT_DRIVE:
		DrawSelectDrivePopup();
		break;

	case LoadingRes::State::LOAD:
	{
		switch (m_loadingRes.fileType)
		{
		case FileType::ROM:
			LoadRom(m_loadingRes.path);
			break;
		case FileType::FDD:
			LoadFdd(m_loadingRes.path, m_loadingRes.driveIdx, m_loadingRes.autoBoot);
			m_prepare_for_exit = true;
			break;
		case FileType::REC:
			LoadRecording(m_loadingRes.path);
			break;
		}

		m_loadingRes.state = LoadingRes::State::UPDATE_RECENT;
		break;
	}
	case LoadingRes::State::UPDATE_RECENT:
		RecentFilesUpdate(m_loadingRes.fileType, m_loadingRes.path, m_loadingRes.driveIdx, m_loadingRes.autoBoot);
		RecentFilesStore();
		m_loadingRes.state = LoadingRes::State::NONE;
		break;

	default:
		break;
	}
}

void dev::DevectorApp::RecentFilesInit()
{
	auto recentFiles = GetSettingsObject("recentFiles");
	for (const auto& fileType_path_driveIdx_autoBoot : recentFiles)
	{
		FileType fileType = static_cast<FileType>(fileType_path_driveIdx_autoBoot[0]);
		auto path = fileType_path_driveIdx_autoBoot[1];
		int driveIdx = fileType_path_driveIdx_autoBoot[2];
		bool autoBoot = fileType_path_driveIdx_autoBoot[3];

		m_recentFilePaths.push_back({ fileType, path, driveIdx, autoBoot });
	}
}

void dev::DevectorApp::RecentFilesStore()
{
	nlohmann::json recentFiles;
	for (const auto& [fileType, path, driveIdx, autoBoot] : m_recentFilePaths)
	{
		nlohmann::json item = { fileType, path, driveIdx, autoBoot };

		recentFiles.push_back(item);
	}
	SettingsUpdate("recentFiles", recentFiles);
	SettingsSave(m_settingsPath);
}

void dev::DevectorApp::RecentFilesUpdate(const FileType _fileType, const std::string& _path, const int _driveIdx, const bool _autoBoot)
{
	// remove if it contains
	m_recentFilePaths.remove_if(
		[&_path](const auto& tuple) {
			return std::get<1>(tuple) == _path;
		}
	);

	// add a new one
	m_recentFilePaths.push_front({ _fileType, _path, _driveIdx, _autoBoot });
	// check the amount, remove last if excids
	if (m_recentFilePaths.size() > RECENT_FILES_MAX)
	{
		m_recentFilePaths.pop_back();
	}
}

bool dev::DevectorApp::EventFilter(void* _userdata, SDL_Event* _event)
{
	// Retrieve the user pointer to access the class instance
	DevectorApp* appP = static_cast<DevectorApp*>(_userdata);
	
	auto scancode = _event->key.scancode;
	auto action = _event->type;

	if (appP && appP->GetStatus() != AppStatus::EXIT)
	{
		auto displayFocused = appP->m_displayWindowP->IsFocused();
		auto keyboardFocused = appP->m_keyboardWindowP->IsFocused();

		if (action == SDL_EVENT_KEY_DOWN || action == SDL_EVENT_KEY_UP)
		{
			if (displayFocused || keyboardFocused){
				appP->m_hardwareP->Request(Hardware::Req::KEY_HANDLING, { { "scancode", scancode }, { "action", action} });
				return false; // do not pass the event to SDL
			}
		}
	}

	return true; // pass the event to SDL
}

void dev::DevectorApp::AppStyleInit()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.83f, 0.83f, 0.83f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.12f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.24f, 0.25f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.08f, 0.07f, 0.07f, 0.11f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.13f, 0.45f, 0.80f, 0.52f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.50f, 0.83f, 0.63f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.24f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.45f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.61f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.57f, 0.82f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.13f, 0.43f, 0.78f, 0.55f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.14f, 0.56f, 0.97f, 0.83f);
	colors[ImGuiCol_Button] = ImVec4(0.22f, 0.56f, 1.00f, 0.69f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.05f, 0.37f, 0.74f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.08f, 0.35f, 0.70f, 0.69f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.31f, 0.70f, 0.64f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.08f, 0.35f, 0.60f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.53f, 0.55f, 0.75f, 0.11f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.35f, 0.41f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.27f, 0.29f, 0.31f, 0.86f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.07f, 0.36f, 0.71f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.09f, 0.35f, 0.66f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.45f, 0.44f, 0.53f, 0.32f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.53f, 0.69f, 0.84f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.15f, 0.40f, 0.93f, 0.66f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.45f, 0.54f, 0.73f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.24f, 0.24f, 0.24f, 0.82f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.24f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.35f, 0.34f, 0.40f, 0.11f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.02f, 0.41f, 0.87f, 0.84f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.04f, 0.43f, 0.88f, 0.76f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

// check if any fdds were updated
void dev::DevectorApp::CheckMountedFdd()
{
	m_loadingRes.state = LoadingRes::State::OPEN_FILE;
	for (int driveIdx = 0; driveIdx < Fdc1793::DRIVES_MAX; driveIdx++)
	{
		auto fddInfo = *m_hardwareP->Request(
			Hardware::Req::GET_FDD_INFO, { {"driveIdx", driveIdx} });
		
		if (fddInfo["updated"])
		{
			m_loadingRes.pathFddUpdated = fddInfo["path"];
			m_loadingRes.driveIdxUpdated = driveIdx;
			m_loadingRes.state = LoadingRes::State::SAVE_DISCARD;
			break;
		}
	}
}

// check if we need to open a popup
void dev::DevectorApp::SaveDiscardFdd()
{
	if (GetSettingsBool("showSaveDiscardFddDialog", true))
	{
		m_loadingRes.state = LoadingRes::State::OPEN_POPUP_SAVE_DISCARD;
	}
	else {
		auto discardFddChanges = GetSettingsBool("discardFddChanges", true);
		m_loadingRes.state = discardFddChanges ? LoadingRes::State::OPEN_FILE : LoadingRes::State::SAVE;
	}
}

// Popup. Save or Discard mounted updated fdd image?
void dev::DevectorApp::DrawSaveDiscardFddPopup()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window when appearing
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(m_loadingRes.POPUP_SAVE_DISCARD, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static const char* diskNames[] = { "A", "B", "C", "D" };
		ImGui::Text("Previously mounted disk %s was updated. Save or discard changes?", 
			diskNames[m_loadingRes.driveIdxUpdated]);
		ImGui::Text(m_loadingRes.pathFddUpdated.c_str());

		ImGui::NewLine();
		static bool doNotAskAgain = false;
		ImGui::Checkbox("##oldFddDoNotAskAgain", &doNotAskAgain);
		ImGui::SameLine();
		ImGui::Text("Don't ask again");
		ImGui::Separator();

		if (ImGui::Button("Save", ImVec2(120, 0)))
		{
			m_loadingRes.state = doNotAskAgain ? LoadingRes::State::ALWAYS_SAVE : LoadingRes::State::SAVE;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Discard", ImVec2(120, 0)))
		{
			m_loadingRes.state = doNotAskAgain ? LoadingRes::State::ALWAYS_DISCARD : LoadingRes::State::DISCARD;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// store the mounted updated fdd image
void dev::DevectorApp::SaveUpdatedFdd()
{
	auto res = *m_hardwareP->Request(
		Hardware::Req::GET_FDD_IMAGE, { {"driveIdx", m_loadingRes.driveIdxUpdated} });
	auto data = res["data"];
	dev::SaveFile(m_loadingRes.pathFddUpdated, data);	
	m_hardwareP->Request(
			Hardware::Req::RESET_UPDATE_FDD, { {"driveIdx", m_loadingRes.driveIdxUpdated} });

	// check remaining updated fdds
	m_loadingRes.state = LoadingRes::State::CHECK_MOUNTED;
}

// Open the file dialog
void dev::DevectorApp::OpenFile()
{
	bool isRunning = m_hardwareP->Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	if (isRunning) m_hardwareP->Request(Hardware::Req::STOP);

	const char* filters[] = {"*.rom", "*.fdd", "*.rec"};
	const char* filename = tinyfd_openFileDialog(
		"Open File", "", sizeof(filters)/sizeof(const char*), filters, nullptr, 0);
	
	if (filename)
	{
		std::string path = std::string(filename);

		auto ext = dev::StrToUpper(dev::GetExt(path));

		if (ext == EXT_ROM)
		{
			LoadRom(path);
			m_loadingRes.fileType = FileType::ROM;
			m_loadingRes.path = path;
			m_loadingRes.autoBoot = false;
			m_loadingRes.state = LoadingRes::State::UPDATE_RECENT;
		}
		else if (ext == EXT_FDD)
		{
			m_loadingRes.fileType = FileType::FDD;
			m_loadingRes.path = path;
			m_loadingRes.state = LoadingRes::State::OPEN_POPUP_SELECT_DRIVE;
		}
		else if (ext == EXT_REC)
		{
			LoadRecording(path);
			m_loadingRes.fileType = FileType::REC;
			m_loadingRes.path = path;
			m_loadingRes.autoBoot = false;
			m_loadingRes.state = LoadingRes::State::UPDATE_RECENT;
		}
		else {
			dev::Log("Not supported file type: {}", path);
			m_loadingRes.state = LoadingRes::State::NONE;
		}
	}
	else {
		m_loadingRes.state = LoadingRes::State::NONE;
	}


	if (isRunning) m_hardwareP->Request(Hardware::Req::RUN);
}

// Open the file dialog
void dev::DevectorApp::SaveFile()
{
	m_loadingRes.state = LoadingRes::State::NONE;

	bool isRunning = m_hardwareP->Request(Hardware::Req::IS_RUNNING)->at("isRunning");
	if (isRunning) m_hardwareP->Request(Hardware::Req::STOP);

	switch (m_loadingRes.fileType)
	{
	case FileType::REC: 
	{
		const char* filters[] = {"*.rom", "*.fdd", "*.rec"};
		const char* filename = tinyfd_saveFileDialog(
			"Save File", "file_name.rec", sizeof(filters)/sizeof(const char*), filters, nullptr);
	
		if (filename)
		{
			auto result = m_hardwareP->Request(Hardware::Req::DEBUG_RECORDER_SERIALIZE);
			if (result)
			{
				nlohmann::json::binary_t binaryData = result->at("data").get<nlohmann::json::binary_t>();
				std::vector<uint8_t> data(binaryData.begin(), binaryData.end());

				std::string path = std::string(filename);

				if (!data.empty()) dev::SaveFile(path, data, true);
			}
		}
		break;
	}
	}
	
	if (isRunning) m_hardwareP->Request(Hardware::Req::RUN);
}

void dev::DevectorApp::DrawSelectDrivePopup()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window when appearing
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(m_loadingRes.POPUP_SELECT_DRIVE, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Specify the drive to mount the FDD file as \nwell as the auto boot option if required.");
		ImGui::Separator();
		static int driveSelect = 0;
		ImGui::Combo("##DriveSelect", &driveSelect, "Drive A Boot\0Drive A\0Drive B\0Drive C\0Drive D\0");

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			m_loadingRes.autoBoot = (driveSelect == 0);
			m_loadingRes.driveIdx = dev::Max(driveSelect - 1, 0); // "0" and "1" are both associated with FDisk 0
			m_loadingRes.state = LoadingRes::State::LOAD;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) 
		{ 
			m_loadingRes.state = LoadingRes::State::NONE;
			ImGui::CloseCurrentPopup(); 
		}
		ImGui::EndPopup();
	}
}

void dev::DevectorApp::ReqUIHandling()
{
	switch (m_reqUI.type)
	{
	case ReqUI::Type::RELOAD_ROM_FDD_REC:
		Reload();
		m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
		break;

	case ReqUI::Type::LOAD_RECENT_FDD_IMG:
		MountRecentFddImg();
		m_reqUI.type = ReqUI::Type::NONE;
		break;
	}
}


void dev::DevectorApp::LoadRom(const std::string& _path)
{
	auto result = dev::LoadFile(_path);
	if (!result || result->empty()) {
		dev::Log("Error occurred while loading the file. Path: {}. "
			"Please ensure the file exists and you have the correct permissions to read it.", _path);
		return;
	}

	m_hardwareP->Request(Hardware::Req::STOP);
	m_hardwareP->Request(Hardware::Req::RESET);
	m_hardwareP->Request(Hardware::Req::RESTART);

	auto reqData = nlohmann::json({ {"data", *result}, {"addr", Memory::ROM_LOAD_ADDR} });
	m_hardwareP->Request(Hardware::Req::SET_MEM, reqData);

	m_hardwareP->Request(Hardware::Req::DEBUG_RESET, { {"resetRecorder", true}}); // has to be called after Hardware loading Rom because it stores the last state of Hardware
	m_debuggerP->GetDebugData().LoadDebugData(_path);
	m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
	m_hardwareP->Request(Hardware::Req::RUN);

	Log("File loaded: {}", _path);
}

void dev::DevectorApp::LoadFdd(const std::string& _path, const int _driveIdx, const bool _autoBoot)
{
	auto fddResult = dev::LoadFile(_path);
	if (!fddResult || fddResult->empty()) {
		dev::Log("Fdc1793 Error: loading error. "
			"Ensure the file exists and its permissions are correct. Path: {}", _path);
		return;
	}

	auto origSize = fddResult->size();
	auto fddimg = *fddResult;

	if (fddimg.size() > FDD_SIZE) {
		dev::Log("Fdc1793 Warning: disk image is too big. "
			"It size will be concatenated to {}. Original size: {} bytes, path: {}", FDD_SIZE, origSize, _path);
		fddimg.resize(FDD_SIZE);
	}

	if (_autoBoot) m_hardwareP->Request(Hardware::Req::STOP);

	// loading the fdd data
	m_hardwareP->Request(Hardware::Req::LOAD_FDD, {
		{"data", fddimg },
		{"driveIdx", _driveIdx},
		{"path", _path}
		});

	m_debuggerP->GetDebugData().LoadDebugData(_path);

	if (_autoBoot)
	{
		m_hardwareP->Request(Hardware::Req::RESET);
		m_hardwareP->Request(Hardware::Req::DEBUG_RESET, { {"resetRecorder", true} }); // has to be called after Hardware loading FDD image because it stores the last state of Hardware
		m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
		m_hardwareP->Request(Hardware::Req::RUN);
	}

	Log("File loaded: {}", _path);
}

void dev::DevectorApp::LoadRecording(const std::string& _path)
{
	auto result = dev::LoadFile(_path);
	if (!result || result->empty()) {
		dev::Log("Error occurred while loading the file. Path: {}. "
			"Please ensure the file exists and you have the correct permissions to read it.", _path);
		return;
	}

	m_hardwareP->Request(Hardware::Req::STOP);
	m_hardwareP->Request(Hardware::Req::RESET);
	m_hardwareP->Request(Hardware::Req::RESTART);

	m_hardwareP->Request(Hardware::Req::DEBUG_RECORDER_DESERIALIZE, { {"data", nlohmann::json::binary(*result)} });

	m_hardwareP->Request(Hardware::Req::DEBUG_RESET, { {"resetRecorder", false} }); // has to be called after Hardware loading Rom because it stores the last state of Hardware
	m_debuggerP->GetDebugData().LoadDebugData(_path);
	m_reqUI.type = ReqUI::Type::DISASM_UPDATE;

	Log("File loaded: {}", _path);
}


void dev::DevectorApp::DebugAttach()
{
	bool requiresDebugger = m_disasmWindowVisible || m_breakpointsWindowVisisble || m_watchpointsWindowVisible ||
		m_hexViewerWindowVisible || m_traceLogWindowVisible || m_recorderWindowVisible;
	if (requiresDebugger != m_debuggerAttached)
	{
		m_debuggerAttached = requiresDebugger;

		if (requiresDebugger) {
			m_hardwareP->Request(Hardware::Req::DEBUG_RESET, { {"resetRecorder", true} }); // has to be called before enabling debugging, because Hardware state was changed
			m_reqUI.type = ReqUI::Type::DISASM_UPDATE;
		}

		m_hardwareP->Request(Hardware::Req::DEBUG_ATTACH, { { "data", requiresDebugger } });
	}
}

void dev::DevectorApp::MountRecentFddImg()
{
	for (const auto& [fileType, path, driveIdx, autoBoot] : m_recentFilePaths)
	{
		if (fileType == FileType::FDD) {
			LoadFdd(path, driveIdx, autoBoot);
			break;
		}
	}
}