using System;
using System.Runtime.InteropServices;
using System.Windows.Interop;

namespace Devector;

public class Viewport : HwndHost
{
    private IntPtr _hwnd;

    public new IntPtr Handle => _hwnd;

    protected override HandleRef BuildWindowCore(HandleRef hwndParent)
    {
        _hwnd = CreateWindowEx(
            0,
            "static",
            "",
            WS_CHILD | WS_VISIBLE,
            0, 0,
            (int)Width, (int)Height,
            hwndParent.Handle,
            IntPtr.Zero,
            IntPtr.Zero,
            0);

        return new HandleRef(this, _hwnd);
    }

    protected override void DestroyWindowCore(HandleRef hwnd)
    {
        DestroyWindow(hwnd.Handle);
    }

    [DllImport("user32.dll", SetLastError = true)]
    private static extern IntPtr CreateWindowEx(
        uint dwExStyle,
        string lpClassName,
        string lpWindowName,
        uint dwStyle,
        int x,
        int y,
        int nWidth,
        int nHeight,
        IntPtr hWndParent,
        IntPtr hMenu,
        IntPtr hInstance,
        uint lParam);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool DestroyWindow(IntPtr hwnd);

    private const int WS_CHILD = 0x40000000;
    private const int WS_VISIBLE = 0x10000000;
}