using System.Windows;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.IO;
using System.Windows.Threading;
//using static System.Net.Mime.MediaTypeNames;

using dev;
using static Devector.Consts;
using Microsoft.VisualBasic.FileIO;
using System.Windows.Shapes;
using System.Text.Json.Nodes;
using System;

namespace Devector
{
	public partial class App : Application
	{
		[DllImport("kernel32.dll")]
		private static extern bool AllocConsole();

		public const double DISPLAY_REFRESH_RATE = 50;
		public const double DISPLAY_UPDATE_DELAY = 1000 / DISPLAY_REFRESH_RATE;

		public HAL? Hal { get; private set; }

		private DispatcherTimer? _halDisplayUpdateTmer;
		public static event EventHandler? halDisplayUpdate;

		private string m_settingsPath = "";
		private string m_rom_fdd_recPath = "";
		private JsonNode? m_settingsJ = JsonNode.Parse("{}");

        private readonly object _settingsLock = new object();

		// Window visibility
		bool m_breakpointsWindowVisisble = false;
		bool m_hardwareStatsWindowVisible = false;
		bool m_disasmWindowVisible = false;
		bool m_watchpointsWindowVisible = false;
		bool m_displayWindowVisible = false;
		bool m_memDisplayWindowVisible = false;
		bool m_hexViewerWindowVisible = false;
		bool m_traceLogWindowVisible = false;
		bool m_recorderWindowVisible = false;
		bool m_keyboardWindowVisible = false;

		string m_pathBootData = "";
		bool m_restartOnLoadFdd = false;
		bool m_ramDiskClearAfterRestart = true;
		string m_ramDiskDataPath = "";

		// Recent files
		string m_pathImgKeyboard = "";

		struct LoadingRes
		{
			public enum State
			{
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
			}

			public enum Type // defines the action during State = OPEN_FILE
            {
				OPEN_FILE_DIALOG,
				RECENT,
				SAVE_THEN_EXIT,
				SAVE_REC_FILE_DIALOG,
			}

			public const string POPUP_SELECT_DRIVE = "Fdd Setup";
			public const string POPUP_SAVE_DISCARD = "Save or Discard?";

			public State state = State.NONE;
			public string path = default;
			public int driveIdx = default;
			public bool autoBoot = default;
			public Type type = Type.OPEN_FILE_DIALOG;
			public string pathFddUpdated = default;
			public int driveIdxUpdated = default;
			public FileType fileType = FileType.ROM;

			public void Init(State _state, Type _type = Type.OPEN_FILE_DIALOG, FileType _fileType = FileType.UNDEFINED, string _path = "", int _driveIdx = INVALID_ID, bool _autoBoot = false)
			{
				if (state == State.EXIT) return;
				fileType = _fileType;
				state = _state;
				type = _type;
				path = _path;
				driveIdx = _driveIdx;
				autoBoot = _autoBoot;
			}

            public LoadingRes(State _state = State.NONE, Type _type = Type.OPEN_FILE_DIALOG, FileType _fileType = FileType.UNDEFINED, string _path = "", int _driveIdx = INVALID_ID, bool _autoBoot = false)
            {
                if (state == State.EXIT) return;
                fileType = _fileType;
                state = _state;
                type = _type;
                path = _path;
                driveIdx = _driveIdx;
                autoBoot = _autoBoot;
            }
        }

		LoadingRes m_loadingRes; // loading resource info

		private const int RECENT_FILES_MAX = 10;
		private const string EXT_ROM = ".ROM";
		private const string EXT_FDD = ".FDD";
		private const string EXT_REC = ".REC";

		bool m_mountRecentFddImg = false;

		enum FileType { ROM = 0, FDD, REC, UNDEFINED };

		// path, file type, driveIdx, autoBoot
		record RecentFile(FileType FileType, string Path, int DriveIdx, bool AutoBoot);
		private List<RecentFile> m_recentFilePaths = new List<RecentFile>();

		protected override void OnStartup(StartupEventArgs e)
		{
			base.OnStartup(e);
			// opens a console window
			AllocConsole();

			Init(e.Args);
		}

		private void Init(string[] args)
		{
			// init a timer
			_halDisplayUpdateTmer = new DispatcherTimer(TimeSpan.FromMilliseconds(DISPLAY_UPDATE_DELAY),
										 DispatcherPriority.Render,
										 halDisplayUpdateCallback,
										 Dispatcher.CurrentDispatcher);
			_halDisplayUpdateTmer.Start();


			// init settings
			(m_settingsPath, m_settingsJ, m_rom_fdd_recPath) = HandleArgs(args, DEFAULT_SETTING_PATH);
			SettingsInit();
			HardwareInit();
			WindowsInit();
			Load(m_rom_fdd_recPath);

            Hal?.Request(HAL.Req.RUN, "");
        }

		private void halDisplayUpdateCallback(object? sender, EventArgs e)
		{
			halDisplayUpdate?.Invoke(this, EventArgs.Empty);
		}

		protected override void OnExit(ExitEventArgs e)
		{
			// Clean up
			_halDisplayUpdateTmer?.Stop();
			_halDisplayUpdateTmer = null;
			base.OnExit(e);
		}

		private (string, JsonNode?, string) HandleArgs(string[] args, string defaultSettingsPath)
		{
			var argsParser = new ArgsParser(args,
				"This is an emulator of the Soviet personal computer Vector06C. It has built-in debugger functionality.");

			var settingsPath = argsParser.GetString("settingsPath",
				"The path to the settings.", false, defaultSettingsPath);

			var rom_fdd_recPath = argsParser.GetString("path",
				"The path to the rom/fdd/rec file.", false, "");

			if (!string.IsNullOrEmpty(rom_fdd_recPath) && !File.Exists(rom_fdd_recPath))
			{
				Console.WriteLine($"A path is invalid: {rom_fdd_recPath}");
				rom_fdd_recPath = "";
			}

			if (!argsParser.IsRequirementSatisfied())
			{
				Console.WriteLine("---Settings parameters are missing");
			}

			var settingsJ = File.Exists(settingsPath) ? JsonNode.Parse(File.ReadAllText(settingsPath)) : JsonNode.Parse("{}");
			return (settingsPath, settingsJ, rom_fdd_recPath);
		}

		private void SettingsInit()
		{
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
			m_pathImgKeyboard = GetSettingsString("pathImgKeyboard", "images//vector_keyboard.jpg");

			RecentFilesInit();

			m_mountRecentFddImg = GetSettingsBool("m_mountRecentFddImg", true);
			if (m_mountRecentFddImg) {
				// TODO: fix it
				//m_reqUI.type = ReqUI::Type::LOAD_RECENT_FDD_IMG;
			}

			m_pathBootData = GetSettingsString("bootPath", "boot//boot.bin");
			m_restartOnLoadFdd = GetSettingsBool("restartOnLoadFdd", true);
			m_ramDiskClearAfterRestart = GetSettingsBool("ramDiskClearAfterRestart", false);
			m_ramDiskDataPath = GetSettingsString("ramDiskDataPath", "ramDisks.bin");
		}

		private void HardwareInit()
		{
			Hal = new HAL(m_pathBootData, m_ramDiskDataPath, m_ramDiskClearAfterRestart);
		}

		private void WindowsInit()
		{
			// TODO: revise

			//m_hardwareStatsWindowP = std::make_unique<dev::HardwareStatsWindow>(*m_hardwareP, &m_dpiScale, m_ruslat);
			//m_disasmWindowP = std::make_unique<dev::DisasmWindow>(*m_hardwareP, *m_debuggerP,
			//    m_fontItalic, &m_dpiScale, m_reqUI);
			//m_displayWindowP = std::make_unique<dev::DisplayWindow>(*m_hardwareP, &m_dpiScale, m_glUtils, m_reqUI);
			//m_breakpointsWindowP = std::make_unique<dev::BreakpointsWindow>(*m_hardwareP, &m_dpiScale, m_reqUI);
			//m_watchpointsWindowP = std::make_unique<dev::WatchpointsWindow>(*m_hardwareP, &m_dpiScale, m_reqUI);
			//m_memDisplayWindowP = std::make_unique<dev::MemDisplayWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_glUtils, m_reqUI);
			//m_hexViewerWindowP = std::make_unique<dev::HexViewerWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
			//m_traceLogWindowP = std::make_unique<dev::TraceLogWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
			//m_aboutWindowP = std::make_unique<dev::AboutWindow>(&m_dpiScale);
			//m_feedbackWindowP = std::make_unique<dev::FeedbackWindow>(&m_dpiScale);
			//m_recorderWindowP = std::make_unique<dev::RecorderWindow>(*m_hardwareP, *m_debuggerP, &m_dpiScale, m_reqUI);
			//m_keyboardWindowP = std::make_unique<dev::KeyboardWindow>(*m_hardwareP, &m_dpiScale, m_glUtils, m_reqUI, m_pathImgKeyboard);
		}

		private void Load(string romFddRecPath)
		{
			if (string.IsNullOrEmpty(romFddRecPath))
				return;

			bool wasRunning = (Hal?.Request(HAL.Req.IS_RUNNING, "").RootElement.TryGetProperty("isRunning", out JsonElement prop) ?? false) && prop.GetBoolean();


			if (wasRunning)
				Hal?.Request(HAL.Req.STOP, "");

			string filePath = romFddRecPath;
			string fileExtension = System.IO.Path.GetExtension(filePath).ToUpper();

			switch (fileExtension)
			{
				case EXT_ROM:
					RecentFilesUpdate(FileType.ROM, filePath);
					break;
				case EXT_FDD:
					RecentFilesUpdate(FileType.FDD, filePath, 0, true);
					break;
				case EXT_REC:
                    PrintLoadingError(Hal?.LoadRecording(filePath), filePath);
                    break;
				default:
					Console.WriteLine($"Unsupported file type: {filePath}");
					m_loadingRes.state = LoadingRes.State.NONE;
					return;
			}

			RecentFilesStore();
			Reload();

			if (wasRunning)
				Hal?.Request(HAL.Req.RUN, "");
		}


		private bool GetSettingsBool(string fieldName, bool defaultValue)
		{
			lock (_settingsLock)
			{
				var value = m_settingsJ?[fieldName];
				return value == null ? defaultValue : value.GetValue<bool>();
            }
		}
		private string GetSettingsString(string fieldName, string defaultValue)
		{
			lock (_settingsLock)
			{
                var value = m_settingsJ?[fieldName];
                return value == null ? defaultValue : value.GetValue<string>();
            }
		}
		private JsonNode? GetSettingsObject(string fieldName)
		{
			lock (_settingsLock)
			{
                var value = m_settingsJ?[fieldName];
                return value == null ? JsonNode.Parse("{}") : value;
            }
		}

		private void SettingsUpdate(string fieldName, JsonNode json)
		{
			lock (_settingsLock)
			{
                m_settingsJ[fieldName] = json;
            }
		}

        private void RecentFilesInit()
		{
			var recentFiles = GetSettingsObject("recentFiles");
			if (recentFiles == null) return;

            if (recentFiles is JsonArray array)
			{
                foreach (var item in array)
                {
                    var fileType = (App.FileType)(item[0].GetValue<int>());
                    var path = item[1].GetValue<string>();
                    var driveIdx = item[2].GetValue<int>();
                    var autoBoot = item[3].GetValue<bool>();

                    var recentFile = new RecentFile(fileType, path, driveIdx, autoBoot);

                    m_recentFilePaths.Add(recentFile);
                }
            }    
		}

        private void RecentFilesUpdate(FileType fileType, string path, int driveIdx = INVALID_ID, bool autoBoot = false)
		{
			// remove if it contains
			m_recentFilePaths.RemoveAll(recentFile => recentFile.Path == path);

			// add a new one
			m_recentFilePaths.Insert(0, new RecentFile(fileType, path, driveIdx, autoBoot));

			// check the amount, remove last if excids
			if (m_recentFilePaths.Count > RECENT_FILES_MAX)
			{
				m_recentFilePaths.RemoveAt(m_recentFilePaths.Count - 1);
			}
		}

        private void RecentFilesStore()
        {
            var recentFiles = new JsonArray();

            for (int i = 0; i < m_recentFilePaths.Count; i++)
            {
                var (fileType, pathN, driveIdx, autoBoot) = m_recentFilePaths[i];
				string path = pathN ?? "";
                var item = new JsonArray()
                {
                    (int)fileType, path, driveIdx, autoBoot
                };

                recentFiles.Add(item);
            }
            SettingsUpdate("recentFiles", recentFiles);
            SettingsSave(m_settingsPath);
        }

        private void SettingsSave(string _path)
		{
            lock (_settingsLock)
            {
                using (var stream = File.Create(_path))
                {
                    using (var writer = new Utf8JsonWriter(stream, new JsonWriterOptions { Indented = true }))
                    {
                        if (m_settingsJ != null) m_settingsJ.WriteTo(writer);
                    }
                }
            }
		}

        public void Reload()
        {
            if (m_recentFilePaths.Count == 0) return;
            // get latest recent path
            var (fileType, path, driveIdx, autoBoot) = m_recentFilePaths.First();

            switch (fileType)
            {
                case FileType.ROM:
                    PrintLoadingError(Hal?.LoadRom(path), path);
                    break;

                case FileType.FDD:
                    PrintLoadingError(Hal?.LoadFdd(path, driveIdx, autoBoot), path);
                    break;

                case FileType.REC:
                    PrintLoadingError(Hal?.LoadRecording(path), path);
                    break;
            }
        }


        private void PrintLoadingError(int? res, string path)
		{
			if (res == null || !File.Exists(path)) return;
			
			ErrCode errCode = (ErrCode)res;

			var fileSize = File.GetAttributes(path);

            string errDescrption;
			switch (errCode)
			{
                case ErrCode.NO_ERRORS:
                    errDescrption = $"File loaded: {path}";
                    break;

                case ErrCode.WARNING_FDD_IMAGE_TOO_BIG:
                    errDescrption = $"Fdc1793 Warning: disk image is too big. " +
                                    $"It size will be concatenated to {FDD_SIZE}. Original size: {fileSize} bytes, path: {path}";

                    break;

				case ErrCode.NO_FILES:
					errDescrption = $"Error occurred while loading the file. Path: {path}. " +
									"Please ensure the file exists and you have the correct permissions to read it.";
                    break;

            }
		}


    }

}
