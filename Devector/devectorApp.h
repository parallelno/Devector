#pragma once

#include "utils/status.h"
#include "utils/jsonUtils.h"

#include "ui/imGuiApp.h"
#include <thread>
#include <mutex>
#include <string>

namespace dev
{
	const std::string APP_NAME = "Devector";

	class DevectorApp : public ImGuiApp
	{
	public:
		DevectorApp(const nlohmann::json& _settingsJ,
			const int _mainWindowWidth, const int _mainWindowHeight);

		virtual void Update();

	protected:

		void WindowsInit();
		void SettingsInit(const nlohmann::json& _settingsJ);

		int mainWindowWidth;
		int mainWindowHeight;
	};

}