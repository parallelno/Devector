#pragma once
#ifndef DEV_DEVECTORAPP_H
#define DEV_DEVECTORAPP_H
#include <string>
#include "GLFW/glfw3.h"

#include "Types.h"
#include "Utils/Status.h"
#include "Utils/JsonUtils.h"
#include "Utils/ImGuiApp.h"
#include "Utils/GLUtils.h"

#include "UI/BreakpointsWindow.h"
#include "UI/WatchpointsWindow.h"
#include "UI/DisasmWindow.h"
#include "UI/HardwareStatsWindow.h"
#include "UI/DisplayWindow.h"
#include "UI/MemDisplayWindow.h"
#include "UI/MemViewerWindow.h"

#include "Hardware.h"
#include "Debugger.h"

namespace dev
{
	class DevectorApp : public ImGuiApp
	{
		const std::string APP_NAME = "Devector";
		const std::string FONT_CODE_PATH_DEFAULT = "Devector";
		static constexpr int RECENT_FILES_MAX = 10;
		const std::string LABELS_FILENAME = "debug.txt";

		std::unique_ptr <dev::Hardware> m_hardwareP;
		std::unique_ptr <dev::Debugger> m_debuggerP;
		std::unique_ptr <dev::BreakpointsWindow>m_breakpointsWindowP;
		std::unique_ptr <dev::HardwareStatsWindow> m_hardwareStatsWindowP;
		std::unique_ptr <dev::DisasmWindow> m_disasmWindowP;
		std::unique_ptr <dev::WatchpointsWindow>m_watchpointsWindowP;
		std::unique_ptr <dev::DisplayWindow>m_displayWindowP;
		std::unique_ptr <dev::MemDisplayWindow>m_memDisplayWindowP;
		std::unique_ptr <dev::MemViewerWindow>m_memViewerWindowP;

		bool m_hardwareStatsWindowShow = true;
		bool m_disasmWindowShow = true;
		bool m_memoryMapWindowShow = true;

		std::list<std::wstring> m_recentFilePaths;

		int m_reqDisasmUpdate = REQ_DISASM_NONE;
		int m_reqDisasmUpdateData = 0;
		bool m_reset = false;
		GLFWkeyfun ImGui_ImplGlfw_KeyCallback;
		GLUtils m_glUtils;

	public:
		DevectorApp(const std::string& _stringPath, nlohmann::json _settingsJ);

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
		static void KeyHandling(GLFWwindow* _window, int _key, int _scancode, int _action, int _modes);
	};

}
#endif // !DEVECTORAPP_H