using System.Configuration;
using System.Data;
using System.Windows;
using System.Runtime.InteropServices;

using dev;
using System.Windows.Threading;

namespace Devector
{
    public partial class App : Application
    {
        [DllImport("kernel32.dll")]
        private static extern bool AllocConsole();

        public const double DISPLAY_REFRESH_RATE = 50;
        public const double DISPLAY_UPDATE_DELAY = 1000 / DISPLAY_REFRESH_RATE;

        public HAL? Hal { get; private set; }

        private DispatcherTimer? _timer;
        public static event EventHandler? DisplayTimer;

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            // opens a console window
            AllocConsole();

            // Define paths for boot data and RAM disk
            string bootDataPath = "boot//boot.bin";
            string ramDiskPath = "ramDisks.bin";
            bool clearRamDiskAfterRestart = true;

            Hal = new HAL(bootDataPath, ramDiskPath, clearRamDiskAfterRestart);

            Hal.ReqRun();

            // init a timer
            _timer = new DispatcherTimer(TimeSpan.FromMilliseconds(DISPLAY_UPDATE_DELAY),
                                         DispatcherPriority.Render,
                                         TimerTick,
                                         Dispatcher.CurrentDispatcher);
            _timer.Start();
        }

        private void TimerTick(object? sender, EventArgs e)
        {
            DisplayTimer?.Invoke(this, EventArgs.Empty);
        }

        protected override void OnExit(ExitEventArgs e)
        {
            // Clean up
            _timer?.Stop();
            _timer = null;
            base.OnExit(e);
        }
    }

}
