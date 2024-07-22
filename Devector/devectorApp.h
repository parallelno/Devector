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
#include "UI/AboutWindow.h"
#include "UI/FeedbackWindow.h"
#include "UI/RecorderWindow.h"

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
			enum class State {
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
				EXIT,
			};
			enum class Type { // defines the action during State = OPEN_FILE
				OPEN_FILE_DIALOG,
				RECENT,
				SAVE_THEN_EXIT,
			};

			const char* POPUP_SELECT_DRIVE = "Fdd Setup";
			const char* POPUP_SAVE_DISCARD = "Save or Discard?";

			State state = State::NONE;
			std::wstring path;
			int driveIdx = 0;
			bool autoBoot = false;
			Type type = Type::OPEN_FILE_DIALOG;
			std::wstring pathFddUpdated;
			int driveIdxUpdated = 0;

			void Init(const State& _state, const Type _type = Type::OPEN_FILE_DIALOG, const std::wstring& _path = L"",
				const int _driveIdx = -1, bool _autoBoot = false) 
			{
				if (state == LoadingRes::State::EXIT) return;
				state = _state;
				type = _type;
				path = _path;
				driveIdx = _driveIdx;
				autoBoot = _autoBoot;
			}
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
		std::unique_ptr <dev::AboutWindow>m_aboutWindowP;
		std::unique_ptr <dev::FeedbackWindow>m_feedbackWindowP;
		std::unique_ptr <dev::RecorderWindow>m_recorderWindowP;

		bool m_displayWindowVisible = false;
		bool m_disasmWindowVisible = false;
		bool m_hardwareStatsWindowVisible = false;
		bool m_breakpointsWindowVisisble = false;
		bool m_watchpointsWindowVisible = false;
		bool m_memDisplayWindowVisible = false;
		bool m_hexViewerWindowVisible = false;
		bool m_traceLogWindowVisible = false;
		bool m_aboutWindowVisible = false;
		bool m_feedbackWindowVisible = false;
		bool m_recorderWindowVisible = false;

		bool m_ruslat = false;
		bool m_restartOnLoadFdd = false;
		int m_rustLatSwitched = 0;

		bool m_debuggerAttached = false;

		// path, driveIdx, autoBoot
		std::list<std::tuple<std::wstring, int, bool>> m_recentFilePaths;

		ReqUI m_reqUI;

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
		void ResLoadingStatusHandling();
		void ReqUIHandling();
		void DebugAttach();
		void RestartOnLoadFdd();
	};
}