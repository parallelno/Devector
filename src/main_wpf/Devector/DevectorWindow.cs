using System.Windows;
using dev;

namespace Devector
{
    public abstract class DevectorWindow : Window
    {
        protected HAL? Hal;

        public DevectorWindow()
        {
            var app = (Devector.App)System.Windows.Application.Current;
            Hal = app.Hal;

            LocationChanged += WindowLocationChanged;
            SizeChanged += WindowSizeChanged;
        }

        private void WindowLocationChanged(object? sender, EventArgs e)
        {
            // TODO: store the location to settings
        }

        private void WindowSizeChanged(object sender, SizeChangedEventArgs e)
        {
            // TODO: store the size to settings
        }
    }
}
