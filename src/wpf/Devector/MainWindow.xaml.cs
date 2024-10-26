using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

using dev;

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

            AllocConsole();  // This will open a console window
            
            HAL obj = new HAL(42, 3.14f);

            obj.field1 += 20;

            // Call the method from the C++/CLI class
            obj.DisplayData();

            var err = HAL.DisplayData2();
        }
    }
}