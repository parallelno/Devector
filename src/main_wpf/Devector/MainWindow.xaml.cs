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

using SharpDX;
using SharpDX.Direct3D9;

using dev;

using static System.Net.Mime.MediaTypeNames;
using System.Numerics;

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
            Closed += MainWindow_Closed;
        }

        protected override void OnRender(DrawingContext drawingContext)
        {
            base.OnRender(drawingContext);
            // This will only be called when WPF determines a redraw is needed
            RedrawMyContent();
        }

        private int counter = 0;

        private bool m_inited = false;

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            RedrawMyContent();
        }

        HAL m_hal;



        //////////////////////////////////
        ///
        /// 
        /// 
        bool glInited = false;

        private void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            var viewportW = viewport.ActualWidth;
            var viewportH = viewport.ActualHeight;
            m_hal.Init(viewport.Handle, (int)viewportW, (int)viewportH);        // Call the C++/CLI function

            glInited = true;
        }

        void RedrawMyContent()
        {

            if (!glInited) return;

            counter++;
            var cc = m_hal.GetCC();
            label.Content = "Counter: " + cc.ToString();


            var viewportW = viewport.ActualWidth;
            var viewportH = viewport.ActualHeight;
            
            m_hal.UpdateData(true, (int)viewportW, (int)viewportH);
            m_hal.DrawDisplay((int)viewportW, (int)viewportH);
        }

        private void MainWindow_Closed(object? sender, EventArgs e)
        {
            
        }


    }
}