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

using static System.Net.Mime.MediaTypeNames;

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

        void RedrawMyContent()
        {
            
            if (!glInited) return;

            counter++;
            var cc = m_hal.GetCC();
            label.Content = "Counter: " + cc.ToString();


            //var wih = new System.Windows.Interop.WindowInteropHelper(this);
            //IntPtr hWnd = wih.Handle;

            var viewportW = viewport.ActualWidth / 2;
            var viewportH = viewport.ActualHeight / 2;

            var viewportW2 = viewport2.ActualWidth / 1.5;
            var viewportH2 = viewport2.ActualHeight / 1.5;

            //m_hal.RenderTexture(hWnd);        // Call the C++/CLI function
            m_hal.RenderDraw(viewport.Handle, (int)viewportW, (int)viewportH, 1.0f, 0.5f, 0.0f);        // Call the C++/CLI function
            m_hal.RenderDraw2(viewport2.Handle, (int)viewportW2, (int)viewportH2, 0.0f, 0.5f, 1.0f);        // Call the C++/CLI function
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
            //if (!m_inited) m_hal.Init(hWnd, (int)viewportW, (int)viewportH);
            m_hal.RenderInit(viewport.Handle);        // Call the C++/CLI function
            m_hal.RenderInit2(viewport2.Handle);        // Call the C++/CLI function

            glInited = true;
        }

        private void MainWindow_Closed(object? sender, EventArgs e)
        {

            m_hal.RenderDel2(viewport2.Handle);

            m_hal.RenderDel(viewport.Handle);
        }

    }
}