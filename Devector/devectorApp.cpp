#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include "GLFW/glfw3.h"
#include "DevectorApp.h"
#include "Utils/Utils.h"
#include "Utils/JsonUtils.h"
#include "Utils/StrUtils.h"

dev::DevectorApp::DevectorApp(
	const std::string& _stringPath, nlohmann::json _settingsJ)
	:
	ImGuiApp(_settingsJ, _stringPath, APP_NAME),
	m_glUtils()
{
	SettingsInit();
	WindowsInit();
}

dev::DevectorApp::~DevectorApp()
{
	// TODO: make it working. it is crashing the app because ImGui and probably other objects are dead already.
	/*
	// Save or Discard mounted updated fdd
	bool showSaveDiscardFddDialog = GetSettingsBool("showSaveDiscardFddDialog", true);
	auto discardFddChanges = GetSettingsBool("discardFddChanges", true);
	FddStatus fddStatus = FddStatus::NONE;

	for (int driveIdx = 0; driveIdx < Fdc1793::DRIVES_MAX; driveIdx++) 
	{
		// check if the mounted disk is updated
		auto fddInfo = *m_hardwareP->Request(
			Hardware::Req::GET_FDD_INFO, { {"_driveIdx", driveIdx} });
		
		bool updated = fddInfo["updated"];
		if (!updated) continue;

		auto mountedFddPath = dev::StrToStrW(fddInfo["path"]);

		if (showSaveDiscardFddDialog)
		{
			auto fddStatus = DrawSaveDiscardFddPopup(driveIdx, mountedFddPath);
		}
		else {
			fddStatus = discardFddChanges ? FddStatus::ALWAYS_DISCARD : FddStatus::ALWAYS_SAVE;
		}

		switch (fddStatus)
		{
		case FddStatus::ALWAYS_DISCARD:
			SettingsUpdate("showSaveDiscardFddDialog", false);
			SettingsUpdate("discardFddChanges", true);
			break;

		case FddStatus::ALWAYS_SAVE:
			SettingsUpdate("showSaveDiscardFddDialog", false);
			SettingsUpdate("discardFddChanges", false);

		case FddStatus::SAVE:
			// store the mounted updated fdd image
			auto res = *m_hardwareP->Request(
				Hardware::Req::GET_FDD_IMAGE, { {"_driveIdx", driveIdx} });
			auto data = res["data"];
			dev::SaveFile(mountedFddPath, data);
			break;
		}

	}
	*/
}

void dev::DevectorApp::WindowsInit()
{
	std::wstring pathBootData = dev::StrToStrW(GetSettingsString("bootPath", ""));
	bool restartOnLoadFdd = GetSettingsBool("restartOnLoadFdd", false);

	m_hardwareP = std::make_unique < dev::Hardware>(pathBootData);
	m_debuggerP = std::make_unique < dev::Debugger>(*m_hardwareP);
	m_hardwareStatsWindowP = std::make_unique<dev::HardwareStatsWindow>(*m_hardwareP, &m_fontSize, &m_dpiScale, m_reqHardwareStatsReset, restartOnLoadFdd);
	m_disasmWindowP = std::make_unique<dev::DisasmWindow>(*m_hardwareP, *m_debuggerP, 
		m_fontItalic, &m_fontSize, &m_dpiScale, m_reqDisasm, m_reqHardwareStatsReset, m_reqMainWindowReload);
	m_displayWindowP = std::make_unique<dev::DisplayWindow>(*m_hardwareP, &m_fontSize, &m_dpiScale, m_glUtils);
	m_breakpointsWindowP = std::make_unique<dev::BreakpointsWindow>(*m_debuggerP, &m_fontSize, &m_dpiScale, m_reqDisasm);
	m_watchpointsWindowP = std::make_unique<dev::WatchpointsWindow>(*m_debuggerP, &m_fontSize, &m_dpiScale, m_reqHexViewer);
	m_memDisplayWindowP = std::make_unique<dev::MemDisplayWindow>(*m_hardwareP, *m_debuggerP, &m_fontSize, &m_dpiScale, m_glUtils, m_reqHexViewer);
	m_hexViewerWindowP = std::make_unique<dev::HexViewerWindow>(*m_hardwareP, *m_debuggerP, &m_fontSize, &m_dpiScale, m_reqHexViewer);
	m_traceLogWindowP = std::make_unique<dev::TraceLogWindow>(*m_hardwareP, *m_debuggerP, &m_fontSize, &m_dpiScale, m_reqDisasm);

	// Set the key callback function
	glfwSetWindowUserPointer(m_window, this);
	ImGui_ImplGlfw_KeyCallback = glfwSetKeyCallback(m_window, DevectorApp::KeyHandling);
}

void dev::DevectorApp::SettingsInit()
{
	Request(REQ::LOAD_FONT);
	AppStyleInit();
	RecentFilesInit();
}

// Function to open a file dialog
bool OpenFileDialog(WCHAR* filePath, int size)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = filePath;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = size;
	ofn.lpstrFilter = L"All Files (*.rom, *.fdd)\0*.rom;*.fdd\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	// Display the open file dialog
	if (GetOpenFileName(&ofn) == TRUE)
		return true; // User selected a file
	else
		return false; // User cancelled or an error occurred
}

void dev::DevectorApp::Update()
{
	if (m_reqMainWindowReload) {
		m_reqMainWindowReload = false;
		Reload();
		m_reqDisasm.type = ReqDisasm::Type::UPDATE; // disasm needs an update after reloading lbels and consts
	}
	MainMenuUpdate();

	m_hardwareStatsWindowP->Update();
	m_disasmWindowP->Update();
	m_displayWindowP->Update();
	m_breakpointsWindowP->Update();
	m_watchpointsWindowP->Update();
	m_memDisplayWindowP->Update();
	m_hexViewerWindowP->Update();
	m_traceLogWindowP->Update();
}

void dev::DevectorApp::LoadRom(const std::wstring& _path)
{
	auto result = dev::LoadFile(_path);
	if (!result || result->empty()) {
		dev::Log(L"Error occurred while loading the file. Path: {}. "
			"Please ensure the file exists and you have the correct permissions to read it.", _path);
		return;
	}

	m_hardwareP->Request(Hardware::Req::STOP);
	m_hardwareP->Request(Hardware::Req::RESET);
	m_hardwareP->Request(Hardware::Req::RESTART);

	auto reqData = nlohmann::json({ {"data", *result}, {"addr", Memory::ROM_LOAD_ADDR} });
	m_hardwareP->Request(Hardware::Req::SET_MEM, reqData);

	m_debuggerP->Reset();
	m_debuggerP->disasm.LoadDebugData(_path);
	m_hardwareP->Request(Hardware::Req::RUN);

	Log(L"File loaded: {}", _path);
}

void dev::DevectorApp::LoadFdd(const std::wstring& _path, const int _driveIdx, const bool _autoBoot)
{
	auto fddResult = dev::LoadFile(_path);
	if (!fddResult || fddResult->empty()) {
		dev::Log(L"Error occurred while loading the file. Path: {}. "
			"Please ensure the file exists and you have the correct permissions to read it.", _path);
		return;
	}
	if (fddResult->size() > FDisk::dataLen) {
		dev::Log(L"Fdc1793: disk image is too big. size: {} bytes, path: {}", fddResult->size(), _path);
		return;
	}

	if (_autoBoot) m_hardwareP->Request(Hardware::Req::STOP);

	// loading the fdd data
	m_hardwareP->Request(Hardware::Req::LOAD_FDD, {
		{"data", *fddResult },
		{"driveIdx", _driveIdx},
		{"path", dev::StrWToStr(_path)}
	});

	if (_autoBoot)
	{
		m_debuggerP->Reset();
		m_hardwareP->Request(Hardware::Req::RESET);
		m_hardwareP->Request(Hardware::Req::RUN);
	}

	Log(L"File loaded: {}", _path);
}

void dev::DevectorApp::Reload()
{
	if (m_recentFilePaths.empty()) return;
	// get latest recent path
	const auto& [path, _driveIdx, _autoBoot] = m_recentFilePaths.front();

	if (_driveIdx < 0) LoadRom(path);
	else LoadFdd(path, _driveIdx, _autoBoot);
}

void dev::DevectorApp::MainMenuUpdate()
{
	static std::wstring pathRecent = L"";
	static bool autoBootRecent = false;
	static int driveIdxRecent = 0;
	bool openFddPopup = false;
	static LoadFddStatus fddStatusPopup = LoadFddStatus::NONE;
	static bool autoBootPopup = false;
	static std::wstring fddPathMounted;
	static int driveIdxPopup = 0;
	static bool reqRecentFilesUpdate = false;

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				WCHAR path[MAX_PATH];

				// Open the file dialog
				if (OpenFileDialog(path, MAX_PATH))
				{
					auto ext = StrToUpper(dev::GetExt(path));
					pathRecent = path;

					if (ext == EXT_ROM) {
						LoadRom(path);
						driveIdxRecent = -1;
						autoBootRecent = false;
						reqRecentFilesUpdate = true;
					}
					else if (ext == EXT_FDD)
					{
						openFddPopup = true;
					}
					else {
						dev::Log(L"Not supported file type: {}", path);
					}
				}
			}
			if (ImGui::BeginMenu("Recent Files"))
			{
				for (const auto& [path, driveIdx, autoBoot] : m_recentFilePaths)
				{
					std::string itemS = dev::StrWToStr(path);
					if (driveIdx >= 0) 
					{
						itemS += std::format(":{}", driveIdx);
						itemS += autoBoot ? "A" : "";
					}
					
					if (ImGui::MenuItem(itemS.c_str()))
					{
						if (driveIdx < 0){
							LoadRom(path);
							pathRecent = path;
							reqRecentFilesUpdate = true;
							driveIdxRecent = -1;
							autoBootRecent = false;
						}
						else
						{
							LoadFdd(path, driveIdx, autoBoot);
							pathRecent = path;
							reqRecentFilesUpdate = true;
							driveIdxRecent = driveIdx;
							autoBootRecent = autoBoot;
						}
					}
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Quit", "Alt+F4")) { m_status = AppStatus::EXIT; }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools"))
		{
			ImGui::MenuItem("Debugger", NULL, &m_hardwareStatsWindowShow);
			ImGui::MenuItem("Memory Map", NULL, &m_memoryMapWindowShow);
			ImGui::EndMenu();
		}
	
		ImGui::EndMenuBar();
	}

	// update recent filepaths
	if (reqRecentFilesUpdate) 
	{
		reqRecentFilesUpdate = false;
		RecentFilesUpdate(pathRecent, driveIdxRecent, autoBootRecent);
		RecentFilesStore();
	}

	// FDD: "boot or mount?" popup window
	if (openFddPopup) ImGui::OpenPopup(POPUP_FDD.c_str());

	ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window when appearing
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(POPUP_FDD.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Specify the drive to mount the FDD file as \nwell as the auto boot option if required.");
		ImGui::Separator();
		static int driveSelect = 0;
		ImGui::Combo("##DriveSelect", &driveSelect, "Drive A Boot\0Drive A\0Drive B\0Drive C\0Drive D\0");

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{ 
			ImGui::CloseCurrentPopup();

			autoBootPopup = ( driveSelect == 0 );
			driveIdxPopup = dev::Max(driveSelect - 1, 0); // "0" and "1" are both associated with FDisk 0
			// check if the mounted disk is updated
			auto fddInfo = *m_hardwareP->Request(
				Hardware::Req::GET_FDD_INFO, { {"_driveIdx", driveIdxPopup} });
			fddStatusPopup = fddInfo["updated"] ? LoadFddStatus::UPDATED : LoadFddStatus::LOAD;
			fddPathMounted = dev::StrToStrW(fddInfo["path"]);
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
	
	// Save or Discard mounted updated fdd image? 
	if (fddStatusPopup == LoadFddStatus::UPDATED)
	{
		if (GetSettingsBool("showSaveDiscardFddDialog", true))
		{
			fddStatusPopup = LoadFddStatus::SAVE_DISCARD_DIALOG;
			ImGui::OpenPopup("Save or Discard");
		}
		else {
			auto discardFddChanges = GetSettingsBool("discardFddChanges", true);
			fddStatusPopup = discardFddChanges ? LoadFddStatus::LOAD : LoadFddStatus::SAVE;
		}
	}
	else if (fddStatusPopup == LoadFddStatus::SAVE)
	{
		// store the mounted updated fdd image
		auto res = *m_hardwareP->Request(
			Hardware::Req::GET_FDD_IMAGE, { {"_driveIdx", driveIdxPopup} });
		auto data = res["data"];
		dev::SaveFile(fddPathMounted, data);
		fddStatusPopup = LoadFddStatus::LOAD;
	}
	else if (fddStatusPopup == LoadFddStatus::LOAD)
	{
		LoadFdd(pathRecent, dev::Max(driveIdxPopup, 0), autoBootPopup);	

		driveIdxRecent = driveIdxPopup;
		autoBootRecent = autoBootPopup;
		reqRecentFilesUpdate = true;
		fddStatusPopup = LoadFddStatus::NONE;
	}

	// Save or Discard mounted updated fdd
	auto fddStatus = DrawSaveDiscardFddPopup(driveIdxPopup, fddPathMounted);
	switch (fddStatus)
	{
	case FddStatus::ALWAYS_DISCARD:
		SettingsUpdate("showSaveDiscardFddDialog", false);
		SettingsUpdate("discardFddChanges", true);
		
	case FddStatus::DISCARD:
		fddStatusPopup = LoadFddStatus::LOAD;
		break;

	case FddStatus::ALWAYS_SAVE:
		SettingsUpdate("showSaveDiscardFddDialog", false);
		SettingsUpdate("discardFddChanges", false);

	case FddStatus::SAVE:
		fddStatusPopup = LoadFddStatus::SAVE;
		break;
	}

}

void dev::DevectorApp::RecentFilesInit()
{
	auto recentFiles = GetSettingsObject("recentFiles");
	for (const auto& path_driveIdx_autoBoot : recentFiles)
	{
		auto path = dev::StrToStrW(path_driveIdx_autoBoot[0]);
		int driveIdx = path_driveIdx_autoBoot[1];
		bool autoBoot = path_driveIdx_autoBoot[2];

		m_recentFilePaths.push_back({ path, driveIdx, autoBoot });
	}
}

void dev::DevectorApp::RecentFilesStore()
{
	nlohmann::json recentFiles;
	for (const auto& [path, driveIdx, autoBoot] : m_recentFilePaths)
	{
		nlohmann::json item = { dev::StrWToStr(path), driveIdx, autoBoot };

		recentFiles.push_back(item);
	}
	SettingsUpdate("recentFiles", recentFiles);
	SettingsSave(m_stringPath);
}

void dev::DevectorApp::RecentFilesUpdate(const std::wstring& _path, const int _driveIdx, const bool _autoBoot)
{
	// remove if it contains
	m_recentFilePaths.remove_if(
		[&_path](const auto& tuple) {
			return std::get<0>(tuple) == _path;
		}
	);

	// add a new one
	m_recentFilePaths.push_front({_path, _driveIdx, _autoBoot });
	// check the amount
	if (m_recentFilePaths.size() > RECENT_FILES_MAX)
	{
		m_recentFilePaths.pop_back();
	}
}

void dev::DevectorApp::KeyHandling(GLFWwindow* _window, int _key, int _scancode, int _action, int _modes)
{
	// Retrieve the user pointer to access the class instance
	// TODO: check if a mouse cursor hovers the Display window or any its controls are selected.
	DevectorApp* instance = static_cast<DevectorApp*>(glfwGetWindowUserPointer(_window));
	if (instance) 
	{
		instance->ImGui_ImplGlfw_KeyCallback(_window, _key, _scancode, _action, _modes);

		if (instance->m_displayWindowP->IsHovered()) 
		{
			instance->m_hardwareP->Request(Hardware::Req::KEY_HANDLING, { { "key", _key }, { "action", _action} });
		}
	}
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

