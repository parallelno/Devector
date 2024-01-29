#include "devectorApp.h"
#include "utils/utils.h"
#include "ui/imGuiUtils.h"
#include "utils/jsonUtils.h"

#include <string>
#include <iostream>
#include <chrono>
#include <filesystem>

dev::DevectorApp::DevectorApp(const nlohmann::json& _settingsJ,
	const int _mainWindowWidth, const int _mainWindowHeight)
	:
	ImGuiApp(APP_NAME, _mainWindowWidth, _mainWindowHeight)
{
	SettingsInit(_settingsJ);

	WindowsInit();
}

void dev::DevectorApp::WindowsInit()
{
}

void dev::DevectorApp::SettingsInit(const nlohmann::json& _settingsJ)
{
}

void dev::DevectorApp::Update()
{
}