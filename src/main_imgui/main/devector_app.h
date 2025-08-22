#pragma once

#include <string>

#include "utils/types.h"
#include "utils/json_utils.h"
#include "utils/gl_utils.h"
#include "imgui_app.h"

#include "ui/breakpoints_window.h"
#include "ui/watchpoints_window.h"
#include "ui/disasm_window.h"
#include "ui/hardware_stats_window.h"
#include "ui/display_window.h"
#include "ui/mem_display_window.h"
#include "ui/hex_viewer_window.h"
#include "ui/trace_log_window.h"
#include "ui/about_window.h"
#include "ui/feedback_window.h"
#include "ui/recorder_window.h"
#include "ui/keyboard_window.h"
#include "ui/search_window.h"
#include "ui/debugdata_window.h"
#include "ui/label_edit_window.h"
#include "ui/const_edit_window.h"
#include "ui/comment_edit_window.h"
#include "ui/memory_edit_window.h"
#include "ui/code_perf_edit_window.h"
#include "ui/script_edit_window.h"


#include "core/hardware.h"
#include "core/debugger.h"
#include "scheduler.h"

namespace dev
{
	static const char* APP_NAME = "Devector";

	class DevectorApp : public ImGuiApp
	{
		static constexpr int RECENT_FILES_MAX = 10;
		const std::string EXT_ROM = ".ROM";
		const std::string EXT_FDD = ".FDD";
		const std::string EXT_REC = ".REC";

		enum class FileType : int {ROM = 0, FDD, REC, UNDEFINED};

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
				SAVE_REC_FILE_DIALOG,
			};

			const char* POPUP_SELECT_DRIVE = "Fdd Setup";
			const char* POPUP_SAVE_DISCARD = "Save or Discard?";

			State state = State::NONE;
			std::string path;
			int driveIdx = 0;
			bool autoBoot = false;
			Type type = Type::OPEN_FILE_DIALOG;
			std::string pathFddUpdated;
			int driveIdxUpdated = 0;
			FileType fileType = FileType::ROM;

			void Init(const State& _state, const Type _type = Type::OPEN_FILE_DIALOG, FileType _fileType = FileType::UNDEFINED, const std::string& _path = "",
				const int _driveIdx = INVALID_ID, bool _autoBoot = false)
			{
				if (state == LoadingRes::State::EXIT) return;
				fileType = _fileType;
				state = _state;
				type = _type;
				path = _path;
				driveIdx = _driveIdx;
				autoBoot = _autoBoot;
			}
		};
		LoadingRes m_loadingRes; // loading resource info

		// windows
		std::unique_ptr <dev::Hardware> m_hardwareP;
		std::unique_ptr <dev::Debugger> m_debuggerP;
		std::unique_ptr <dev::BreakpointsWindow>m_breakpointsWindowP;
		std::unique_ptr <dev::HardwareStatsWindow> m_hardwareStatsWindowP;
		std::unique_ptr <dev::DisasmWindow> m_disasmWindowP;
		std::unique_ptr <dev::WatchpointsWindow> m_watchpointsWindowP;
		std::unique_ptr <dev::DisplayWindow> m_displayWindowP;
		std::unique_ptr <dev::MemDisplayWindow> m_memDisplayWindowP;
		std::unique_ptr <dev::HexViewerWindow> m_hexViewerWindowP;
		std::unique_ptr <dev::TraceLogWindow> m_traceLogWindowP;
		std::unique_ptr <dev::AboutWindow> m_aboutWindowP;
		std::unique_ptr <dev::FeedbackWindow> m_feedbackWindowP;
		std::unique_ptr <dev::RecorderWindow> m_recorderWindowP;
		std::unique_ptr <dev::KeyboardWindow> m_keyboardWindowP;
		std::unique_ptr <dev::SearchWindow> m_searchWindowP;
		std::unique_ptr <dev::DebugDataWindow> m_debugdataWindowP;
		// popup edit debug data windows
		std::unique_ptr <dev::LabelEditWindow> m_labelEditWindowP;
		std::unique_ptr <dev::ConstEditWindow> m_constEditWindowP;
		std::unique_ptr <dev::CommentEditWindow> m_commentEditWindowP;
		std::unique_ptr <dev::MemoryEditWindow> m_memoryEditWindowP;
		std::unique_ptr <dev::CodePerfEditWindow> m_codePerfEditWindowP;
		std::unique_ptr <dev::ScriptEditWindow> m_scriptEditWindowP;

		// windows
		bool m_displayWindowVisible = false;
		bool m_disasmWindowVisible = false;
		bool m_hardwareStatsWindowVisible = false;
		bool m_breakpointsWindowVisible = false;
		bool m_watchpointsWindowVisible = false;
		bool m_memDisplayWindowVisible = false;
		bool m_hexViewerWindowVisible = false;
		bool m_traceLogWindowVisible = false;
		bool m_aboutWindowVisible = false;
		bool m_feedbackWindowVisible = false;
		bool m_recorderWindowVisible = false;
		bool m_keyboardWindowVisible = false;
		bool m_searchWindowVisible = false;
		bool m_debugdataWindowVisible = false;

		std::string m_pathImgKeyboard;

		bool m_ruslat = false;
		bool m_restartOnLoadFdd = false;
		bool m_ramDiskClearAfterRestart = true;
		bool m_mountRecentFddImg = true;
		std::string m_ramDiskDataPath = "";
		int m_rustLatSwitched = 0;

		bool m_debuggerAttached = false;

		// path, file type, driveIdx, autoBoot
		using RecentFile = std::tuple<FileType, std::string, int, bool>;
		using RecentFiles = std::list<RecentFile>;
		RecentFiles m_recentFilePaths;

		GLUtils m_glUtils;

		Scheduler m_scheduler;
		bool m_active = true;
		std::string m_memdisplay_vtxShader = "";
		std::string m_memdisplay_fragShader = "";
		std::string m_display_vtxShader = "";
		std::string m_display_fragShader = "";

	public:
		DevectorApp(const std::string& _settingsPath, nlohmann::json _settingsJ,
					const std::string& _rom_fdd_recPath = "");
		~DevectorApp();

		virtual void Update();

	protected:
		void Init(const std::string& _rom_fdd_recPath);
		void HardwareInit();
		void WindowsInit();
		void SettingsInit();
		void SettingsInitDisplayShaders();
		void SettingsInitMemDisplayShaders();
		void RecentFilesInit();
		void RecentFilesStore();
		void RecentFilesUpdate(const FileType _fileType, const std::string& _path, const int _driveIdx = INVALID_ID, const bool _autoBoot = false);
		void AppStyleInit();
		void MainMenuUpdate();
		void Load(const std::string& _path);
		void LoadRom(const std::string& _path);
		void LoadFdd(const std::string& _path, const int _driveIdx, const bool _autoBoot);
		void LoadRecording(const std::string& _path);
		void CallbackReload(
			const dev::Signals _signals = dev::Signals::NONE,
			dev::Scheduler::SignalData _data = std::nullopt);
		static bool EventFilter(void* _userdata, SDL_Event* _event);
		void DrawSaveDiscardFddPopup();
		void CheckMountedFdd();
		void SaveDiscardFdd();
		void SaveUpdatedFdd();
		void OpenFile();
		void SaveFile();
		void DrawSelectDrivePopup();
		void LoadingResStatusHandling();
		void DebugAttach();
		void RestartOnLoadFdd();
		void CallbackLoadRecentFddImg(
			const dev::Signals _signals, dev::Scheduler::SignalData _data);
		void LoadDroppedFile();
		void SchedulingInit();
	};
}