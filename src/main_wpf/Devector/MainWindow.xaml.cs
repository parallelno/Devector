using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;


using dev;

using SharpDX.Direct3D11;
using SharpDX.DXGI;

namespace Devector
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        [DllImport("kernel32.dll")]
        private static extern bool AllocConsole();

        public MainWindow()
        {
            InitializeComponent();

            // opens a console window
            AllocConsole();
            
            // Define paths for boot data and RAM disk
            string bootDataPath = @"boot//boot.bin";
            string ramDiskPath = @"ramDisks.bin";
            bool clearRamDiskAfterRestart = true;

            m_hal = new HAL(bootDataPath, ramDiskPath, clearRamDiskAfterRestart);

            m_hal.Run();

            Loaded += MainWindow_Loaded;
        }

        
        private int counter = 0;

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            counter++;
            var cc = m_hal.GetCC();
            label.Content = "Counter: " + cc.ToString();


            var wih = new System.Windows.Interop.WindowInteropHelper(this);
            IntPtr hWnd = wih.Handle;

            m_hal.RenderTexture(hWnd);        // Call the C++/CLI function
        }

        HAL m_hal;



        //////////////////////////////////
        ///
        /// 
        /// 
        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            // do not render here because the window is not formed yet.
        }

    }
}