#pragma once

#include <string>
#include "GLFW/glfw3.h"

#include "Utils/Types.h"
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
#include "UI/HexViewerWindow.h"
#include "UI/TraceLogWindow.h"

#include "Core/Hardware.h"
#include "Core/Debugger.h"

namespace dev
{
	class DevectorApp : public ImGuiApp
	{
		const std::string APP_NAME = "Devector";
		const std::string FONT_CODE_PATH_DEFAULT = "Devector";
		static constexpr int RECENT_FILES_MAX = 10;
		const std::wstring EXT_ROM = L".ROM";
		const std::wstring EXT_FDD = L".FDD";
		
		struct LoadingRes
		{
			enum class Status {
				NONE, 
				CHECK_MOUNTED, 
				SAVE_DISCARD, 
				OPEN_FILE,
				OPEN_POPUP_SAVE_DISCARD, 
				POPUP_SAVE_DISCARD,
				OPEN_POPUP_SELECT_DRIVE,
				POPUP_SELECT_DRIVE,
				ALWAYS_DISCARD,
				DISCARD, 
				ALWAYS_SAVE,
				SAVE,
				LOAD,
				UPDATE_RECENT,
			};

			const char* POPUP_SELECT_DRIVE = "Fdd Setup";
			const char* POPUP_SAVE_DISCARD = "Save or Discard?";

			Status status = Status::NONE;
			std::wstring path;
			int driveIdx = 0;
			bool autoBoot = false;
			bool recent = false; // opened from the recent files menu
			std::wstring pathFddUpdated;
			int driveIdxUpdated = 0;
		};
		LoadingRes m_loadingRes; // loading resource info

		std::unique_ptr <dev::Hardware> m_hardwareP;
		std::unique_ptr <dev::Debugger> m_debuggerP;
		std::unique_ptr <dev::BreakpointsWindow>m_breakpointsWindowP;
		std::unique_ptr <dev::HardwareStatsWindow> m_hardwareStatsWindowP;
		std::unique_ptr <dev::DisasmWindow> m_disasmWindowP;
		std::unique_ptr <dev::WatchpointsWindow>m_watchpointsWindowP;
		std::unique_ptr <dev::DisplayWindow>m_displayWindowP;
		std::unique_ptr <dev::MemDisplayWindow>m_memDisplayWindowP;
		std::unique_ptr <dev::HexViewerWindow>m_hexViewerWindowP;
		std::unique_ptr <dev::TraceLogWindow>m_traceLogWindowP;

		bool m_hardwareStatsWindowShow = true;
		bool m_disasmWindowShow = true;
		bool m_memoryMapWindowShow = true;

		// path, driveIdx, autoBoot
		std::list<std::tuple<std::wstring, int, bool>> m_recentFilePaths;

		// TODO: combine all reqs to/from the UI into one object because all
		// such requests are handled sequentially (no collisions) during the same or the next update
		ReqHexViewer m_reqHexViewer; // requests to HexViewerWindow
		ReqDisasm m_reqDisasm; // requests to DisasmWindow
		bool m_reqMainWindowReload = false;  // requests to this
		bool m_reqHardwareStatsReset = false; // requests to HardwareStatsWindow
		GLFWkeyfun ImGui_ImplGlfw_KeyCallback;
		GLUtils m_glUtils;

	public:
		DevectorApp(const std::string& _stringPath, nlohmann::json _settingsJ);
		~DevectorApp();

		virtual void Update();

	protected:
		void WindowsInit();
		void SettingsInit();
		void RecentFilesInit();
		void RecentFilesStore();
		void RecentFilesUpdate(const std::wstring& _path, const int _driveIdx = -1, const bool _autoBoot = false);
		void AppStyleInit();
		void MainMenuUpdate();
		void LoadRom(const std::wstring& _path);
		void LoadFdd(const std::wstring& _path, const int _driveIdx, const bool _autoBoot);
		void Reload();
		static void KeyHandling(GLFWwindow* _window, int _key, int _scancode, int _action, int _modes);
		void DrawSaveDiscardFddPopup();
		void CheckMountedFdd();
		void SaveDiscardFdd();
		void SaveUpdatedFdd();
		void OpenFile();
		void DrawSelectDrivePopup();
	};
}