using System.Configuration;
using System.Data;
using System.Windows;

using dev;

namespace Devector
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            // Define paths for boot data and RAM disk
            string bootDataPath = @"boot//boot.bin";
            string ramDiskPath = @"ramDisks.bin";
            bool clearRamDiskAfterRestart = true;

            Hal = new HAL(bootDataPath, ramDiskPath, clearRamDiskAfterRestart);

            Hal.Run();
        }

        public HAL? Hal { get; private set; }
    }

}
