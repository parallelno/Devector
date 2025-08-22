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
using System.Windows.Controls;
using static dev.HAL;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using static Devector.App;
using System.Reflection;

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
		public event EventHandler? halDisplayUpdate;

		private string m_settingsPath = "";
		private string m_romFddRecPath = "";
		private JsonNode? m_settingsJ = JsonNode.Parse("{}");

		private readonly object _settingsLock = new object();

		// Window visibility
		bool m_breakpointsWindowVisible = false;
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

		string m_pathImgKeyboard = "";

		private const int RECENT_FILES_MAX = 10;
		private const string EXT_ROM = ".ROM";
		private const string EXT_FDD = ".FDD";
		private const string EXT_REC = ".REC";

		bool m_mountRecentFddImg = false;

		/*
		// The User initiated requests in the UI thread
		// for window-to-window communication
		public struct ReqUI
		{
			public enum Type
			{
				NONE = 0,
				DISASM_UPDATE, // redraw disasm
				DISASM_UPDATE_ADDR,
				DISASM_NAVIGATE_TO_ADDR,
				DISASM_NAVIAGATE_NEXT,
				DISASM_NAVIAGATE_PREV,
				HEX_HIGHLIGHT_ON,
				HEX_HIGHLIGHT_OFF,
				RELOAD_ROM_FDD_REC,
				DISPLAY_FRAME_BUFF_UPDATE,
				LOAD_RECENT_FDD_IMG
			};

			public Type type = Type.NONE;
			public uint globalAddr = 0;
			public uint len = 0;

			public ReqUI()
			{
				type = Type.NONE;
				globalAddr = 0;
				len = 0;
			}
		}

		ReqUI m_reqUI = new ReqUI();
		*/

		// path, file type, driveIdx, autoBoot
		public record RecentFile(string Path, int DriveIdx = 0, bool AutoBoot = true);
		private ObservableCollection<RecentFile> m_recentFilePaths = new();

		public ObservableCollection<RecentFile> RecentFilePaths => m_recentFilePaths;

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
										 halDisplayUpdateTimerCallback,
										 Dispatcher.CurrentDispatcher);
			_halDisplayUpdateTmer.Start();


			(m_settingsPath, m_settingsJ, m_romFddRecPath) = HandleArgs(args, DEFAULT_SETTING_PATH);
			SettingsInit();
			HardwareInit();
			WindowsInit();
			Load(new RecentFile( m_romFddRecPath ));

			Hal?.Request(HAL.Req.RUN, "");

			halDisplayUpdate += Update;
		}

		protected override void OnExit(ExitEventArgs e)
		{
			// Clean up
			_halDisplayUpdateTmer?.Stop();
			_halDisplayUpdateTmer = null;

			SettingsUpdate("breakpointsWindowVisisble", m_breakpointsWindowVisible);
			SettingsUpdate("hardwareStatsWindowVisible", m_hardwareStatsWindowVisible);
			SettingsUpdate("disasmWindowVisible", m_disasmWindowVisible);
			SettingsUpdate("watchpointsWindowVisible", m_watchpointsWindowVisible);
			SettingsUpdate("displayWindowVisible", m_displayWindowVisible);
			SettingsUpdate("memDisplayWindowVisible", m_memDisplayWindowVisible);
			SettingsUpdate("hexViewerWindowVisible", m_hexViewerWindowVisible);
			SettingsUpdate("traceLogWindowVisible", m_traceLogWindowVisible);
			SettingsUpdate("recorderWindowVisible", m_recorderWindowVisible);
			SettingsUpdate("keyboardWindowVisible", m_keyboardWindowVisible);

			SettingsSave(m_settingsPath);

			base.OnExit(e);
		}

		private void halDisplayUpdateTimerCallback(object? sender, EventArgs e)
		{
			halDisplayUpdate?.Invoke(this, EventArgs.Empty);
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
			m_breakpointsWindowVisible = GetSettingsBool("breakpointsWindowVisisble", false);
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
				// send a Signal::MOUNT_RECENT_FDD_IMG;
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
			if (recentFiles is not JsonArray array) return;

			foreach (var item in array)
			{
				if (item is not JsonArray fileArray) continue;

				try
				{
                    var path = fileArray[0]?.GetValue<string>() ?? string.Empty;
                    var driveIdx = fileArray[1]?.GetValue<int>() ?? 0;
                    var autoBoot = fileArray[2]?.GetValue<bool>() ?? false;

                    if (!string.IsNullOrEmpty(path))
                    {
                        var recentFile = new RecentFile(path, driveIdx, autoBoot);
                        m_recentFilePaths.Add(recentFile);
                    }
                }
				catch (Exception ex)
				{
					Console.WriteLine($"Warning: invalid recent file data: {ex.Message}");
                    m_recentFilePaths.Clear();
					RecentFilesStore();
                    continue;
				}

			}
		}

		private void RecentFilesUpdate(RecentFile file, bool storeRecentFiles = true)
		{
			// remove if it contains
			RecentFilesRemove(file.Path);

			// add a new one
			m_recentFilePaths.Insert(0, file);

			// check the amount, remove last if excids
			if (m_recentFilePaths.Count > RECENT_FILES_MAX)
			{
				m_recentFilePaths.RemoveAt(m_recentFilePaths.Count - 1);
			}

			if (storeRecentFiles) RecentFilesStore();
		}

		private void RecentFilesRemove(string path)
		{
			for (int i = m_recentFilePaths.Count - 1; i >= 0; i--)
			{
				if (m_recentFilePaths[i].Path == path){
					m_recentFilePaths.RemoveAt(i);
				}
			}
		}

		private void RecentFilesStore()
		{
			var recentFiles = new JsonArray();

			foreach (var recentFilePath in m_recentFilePaths)
			{
				var (pathN, driveIdx, autoBoot) = recentFilePath;
				string path = pathN ?? "";
				var item = new JsonArray()
				{
					path, driveIdx, autoBoot
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

		private void Update(object? sender, EventArgs e)
		{
		}

		public void Load(RecentFile file, bool updateRecentFiles = true)
		{
			if (string.IsNullOrEmpty(file.Path))
				return;

			string ext = System.IO.Path.GetExtension(file.Path).ToUpper();

			switch (ext)
			{
				case EXT_ROM:
					PrintLoadingError(Hal?.LoadRom(file.Path), file.Path);
					break;
				case EXT_FDD:
					PrintLoadingError(Hal?.LoadFdd(file.Path, file.DriveIdx, file.AutoBoot), file.Path);
					break;
				case EXT_REC:
					PrintLoadingError(Hal?.LoadRecording(file.Path), file.Path);
					break;
				default:
					Console.WriteLine($"Unsupported file type: {file.Path}");
					return;
			}

			if (updateRecentFiles)
			{
				RecentFilesUpdate(file);
			}
		}

		public void Reload()
		{
			if (m_recentFilePaths.Count == 0) return;
			// get latest recent path
			Load(m_recentFilePaths.First(), false);
		}


		// Open the file dialog
		public void OpenFile()
		{
			Microsoft.Win32.OpenFileDialog openFileDialog = new Microsoft.Win32.OpenFileDialog();
			openFileDialog.Filter = "ROM files (*.rom)|*.rom|FDD files (*.fdd)|*.fdd|REC files (*.rec)|*.rec";
			bool? result = openFileDialog.ShowDialog();
			string path = result == true ? openFileDialog.FileName : string.Empty;

			if (!string.IsNullOrEmpty(path))
			{
				string ext = System.IO.Path.GetExtension(path).ToUpper();

				if (ext == EXT_FDD)
				{
					// OPEN_POPUP_SELECT_DRIVE
				}

				Load(new RecentFile(path));
			}
		}
    }

}
