#pragma once
#ifndef DEV_DEVECTORAPP_H
#define DEV_DEVECTORAPP_H

#include "Utils/Status.h"
#include "Utils/JsonUtils.h"
#include "Utils/ImGuiApp.h"
#include "UI/HardwareStatsWindow.h"
#include "UI/DisasmWindow.h"
#include "UI/DisplayWindow.h"
#include "UI/BreakpointsWindow.h"
#include "UI/WatchpointsWindow.h"
#include "Hardware.h"

#include <thread>
#include <mutex>
#include <string>
#include <atomic>

namespace dev
{
	class DevectorApp : public ImGuiApp
	{
		const std::string APP_NAME = "Devector";
		const std::string FONT_CODE_PATH_DEFAULT = "Devector";
		static constexpr int RECENT_FILES_MAX = 10;
		const std::string LABELS_FILENAME = "debug.txt";

		const std::string m_stringPath;

		std::unique_ptr <dev::Hardware> m_hardwareP;

		std::unique_ptr <dev::HardwareStatsWindow> m_hardwareStatsWindowP;
		std::unique_ptr <dev::DisasmWindow> m_disasmWindowP;
		std::unique_ptr <dev::DisplayWindow>m_displayWindowP;
		std::unique_ptr <dev::BreakpointsWindow>m_breakpointsWindowP;
		std::unique_ptr <dev::WatchpointsWindow>m_watchpointsWindowP;

		bool m_hardwareStatsWindowShow = true;
		bool m_disasmWindowShow = true;
		bool m_memoryMapWindowShow = true;

		std::list<std::wstring> m_recentFilePaths;

	public:
		DevectorApp(const std::string& _stringPath, nlohmann::json _settingsJ,
			const int _mainWindowWidth, const int _mainWindowHeight);

		virtual void Update();

	protected:
		void WindowsInit();
		void SettingsInit();
		void RecentFilesInit();
		void RecentFilesStore();
		void RecentFilesUpdate(const std::wstring& _filePath);
		void AppStyleInit();
		void MainMenuUpdate();
		void LoadRom(const std::wstring& _filePath);

	};

}
#endif // !DEVECTORAPP_H