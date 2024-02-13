#pragma once
#ifndef DEVECTORAPP_H
#define DEVECTORAPP_H

#include "utils/status.h"
#include "utils/jsonUtils.h"

#include "ui/imGuiApp.h"

#include "ui/hardwareStatsWindow.h"
#include "ui/disasmWindow.h"
#include <unordered_set>
#include <thread>
#include <mutex>
#include <string>

namespace dev
{
	class DevectorApp : public ImGuiApp
	{
		const std::string APP_NAME = "Devector";
		const std::string FONT_CODE_PATH_DEFAULT = "Devector";
		
		nlohmann::json m_settingsJ;
		const std::string m_stringPath;

		ImFont* m_font = nullptr;
		ImFont* m_fontItalic = nullptr;

		std::unique_ptr <dev::HardwareStatsWindow> m_hardwareStatsWindowP;
		std::unique_ptr <dev::DisasmWindow> m_disasmWindowP;

		bool m_hardwareStatsWindowShow = true;
		bool m_disasmWindowShow = true;
		bool m_memoryMapWindowShow = true;

		std::unordered_set<std::wstring> m_recentFilePaths;

	public:
		DevectorApp(const std::string& _stringPath, nlohmann::json _settingsJ,
			const int _mainWindowWidth, const int _mainWindowHeight);

		virtual void Update();

	protected:
		void WindowsInit();
		void SettingsInit();
		void RecentFilesInit();
		void RecentFilesStore();
		void LoadFonts();
		void AppStyleInit();
		void MainMenuUpdate();
	};

}
#endif // !DEVECTORAPP_H