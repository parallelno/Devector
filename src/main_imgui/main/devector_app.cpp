﻿#include <vector>
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
	SettingsUpdate("executionSpeed", static_cast<int>(m_displayWindowP->GetExecutionSpeed()));
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
	auto executionSpeed = static_cast<Hardware::ExecSpeed>(GetSettingsInt("executionSpeed", static_cast<int>(Hardware::ExecSpeed::NORMAL)));
	
	m_hardwareStatsWindowP = std::make_unique<dev::HardwareStatsWindow>(*m_hardwareP, &m_dpiScale, m_ruslat);
	m_disasmWindowP = std::make_unique<dev::DisasmWindow>(*m_hardwareP, *m_debuggerP, 
		m_fontItalic, &m_dpiScale, m_reqUI);
	m_displayWindowP = std::make_unique<dev::DisplayWindow>(*m_hardwareP, &m_dpiScale, m_glUtils, m_reqUI, m_debuggerP->GetDebugData().GetScripts(), executionSpeed);
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
	LoadDroppedFile();
	
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
	DrawEditCodePerfWindow(*m_hardwareP, m_debuggerP->GetDebugData(), m_reqUI);
	DrawEditScriptWindow(*m_hardwareP, m_debuggerP->GetDebugData(), m_reqUI);


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
	colors[ImGuiCol_Text] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Text", "D4D4D4FF") ));
	colors[ImGuiCol_TextDisabled] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TextDisabled", "858585FF") ));
	colors[ImGuiCol_WindowBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_WindowBg", "1F1F1FFF") ));
	colors[ImGuiCol_ChildBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ChildBg", "1717171F") ));
	colors[ImGuiCol_PopupBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PopupBg", "212121FF") ));
	colors[ImGuiCol_Border] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Border", "3D3D3D40") ));
	colors[ImGuiCol_BorderShadow] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_BorderShadow", "1412121C") ));
	colors[ImGuiCol_FrameBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_FrameBg", "363636FF") ));
	colors[ImGuiCol_FrameBgHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_FrameBgHovered", "2173CC85") ));
	colors[ImGuiCol_FrameBgActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_FrameBgActive", "0080D4A1") ));
	colors[ImGuiCol_TitleBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TitleBg", "262626FF") ));
	colors[ImGuiCol_TitleBgActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TitleBgActive", "1F1F1FFF") ));
	colors[ImGuiCol_TitleBgCollapsed] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TitleBgCollapsed", "2E2E2EFF") ));
	colors[ImGuiCol_MenuBarBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_MenuBarBg", "333333FF") ));
	colors[ImGuiCol_ScrollbarBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarBg", "1414143D") ));
	colors[ImGuiCol_ScrollbarGrab] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarGrab", "3D3D3DFF") ));
	colors[ImGuiCol_ScrollbarGrabHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarGrabHovered", "7D7D7D73") ));
	colors[ImGuiCol_ScrollbarGrabActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarGrabActive", "7575759C") ));
	colors[ImGuiCol_CheckMark] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_CheckMark", "4091D2FF") ));
	colors[ImGuiCol_SliderGrab] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SliderGrab", "216EC78C") ));
	colors[ImGuiCol_SliderGrabActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SliderGrabActive", "248FF7D4") ));
	colors[ImGuiCol_Button] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Button", "398FFFB0") ));
	colors[ImGuiCol_ButtonHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ButtonHovered", "0C5EBEFF") ));
	colors[ImGuiCol_ButtonActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ButtonActive", "0F86FAFF") ));
	colors[ImGuiCol_Header] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Header", "145AB2B0") ));
	colors[ImGuiCol_HeaderHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_HeaderHovered", "004FB3A3") ));
	colors[ImGuiCol_HeaderActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_HeaderActive", "145A99FF") ));
	colors[ImGuiCol_Separator] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Separator", "878CBF1C") ));
	colors[ImGuiCol_SeparatorHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SeparatorHovered", "1A66BFC7") ));
	colors[ImGuiCol_SeparatorActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SeparatorActive", "1A66BFFF") ));
	colors[ImGuiCol_ResizeGrip] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ResizeGrip", "4D596933") ));
	colors[ImGuiCol_ResizeGripHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ResizeGripHovered", "4297F9AB") ));
	colors[ImGuiCol_ResizeGripActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ResizeGripActive", "4297F9F2") ));
	colors[ImGuiCol_Tab] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Tab", "454A4FDC") ));
	colors[ImGuiCol_TabHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabHovered", "125CB6CC") ));
	colors[ImGuiCol_TabActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabActive", "1759A8FF") ));
	colors[ImGuiCol_TabUnfocused] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabUnfocused", "121A26F7") ));
	colors[ImGuiCol_TabUnfocusedActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabUnfocusedActive", "23426BFF") ));
	colors[ImGuiCol_DockingPreview] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_DockingPreview", "72708751") ));
	colors[ImGuiCol_DockingEmptyBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_DockingEmptyBg", "171717FF") ));
	colors[ImGuiCol_PlotLines] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotLines", "878787FF") ));
	colors[ImGuiCol_PlotLinesHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotLinesHovered", "87B0D6FF") ));
	colors[ImGuiCol_PlotHistogram] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotHistogram", "2666EDA8") ));
	colors[ImGuiCol_PlotHistogramHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotHistogramHovered", "738AB9FF") ));
	colors[ImGuiCol_TableHeaderBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableHeaderBg", "333335FF") ));
	colors[ImGuiCol_TableBorderStrong] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableBorderStrong", "3D3D3DD1") ));
	colors[ImGuiCol_TableBorderLight] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableBorderLight", "3B3D40FF") ));
	colors[ImGuiCol_TableRowBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableRowBg", "00000000") ));
	colors[ImGuiCol_TableRowBgAlt] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableRowBgAlt", "5957661C") ));
	colors[ImGuiCol_TextSelectedBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TextSelectedBg", "0569DED6") ));
	colors[ImGuiCol_DragDropTarget] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_DragDropTarget", "FFFF00E6") ));
	colors[ImGuiCol_NavHighlight] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_NavHighlight", "0A6FE0C2") ));
	colors[ImGuiCol_NavWindowingHighlight] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_NavWindowingHighlight", "FFFFFFB3") ));
	colors[ImGuiCol_NavWindowingDimBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_NavWindowingDimBg", "CCCCCC33") ));
	colors[ImGuiCol_ModalWindowDimBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ModalWindowDimBg", "CCCCCC59") ));
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

void dev::DevectorApp::LoadDroppedFile()
{
	auto droppedFilePath = GetDroppedFilePath();
	if (droppedFilePath.empty()) return;

	Load(droppedFilePath);

	ResetDroppedFilePath();
}