using System.Windows.Input;

namespace Devector
{
    public static class SDLScancodes
    {
        public enum Scancodes
        {
            Unknown = 0,
            A = 4,
            B = 5,
            C = 6,
            D = 7,
            E = 8,
            F = 9,
            G = 10,
            H = 11,
            I = 12,
            J = 13,
            K = 14,
            L = 15,
            M = 16,
            N = 17,
            O = 18,
            P = 19,
            Q = 20,
            R = 21,
            S = 22,
            T = 23,
            U = 24,
            V = 25,
            W = 26,
            X = 27,
            Y = 28,
            Z = 29,
            Num1 = 30,
            Num2 = 31,
            Num3 = 32,
            Num4 = 33,
            Num5 = 34,
            Num6 = 35,
            Num7 = 36,
            Num8 = 37,
            Num9 = 38,
            Num0 = 39,
            Return = 40,
            Escape = 41,
            Backspace = 42,
            Tab = 43,
            Space = 44,
            Minus = 45,
            Equals = 46,
            Leftbracket = 47,
            Rightbracket = 48,
            Backslash = 49,
            Nonushash = 50,
            Semicolon = 51,
            Apostrophe = 52,
            Grave = 53,
            Comma = 54,
            Period = 55,
            Slash = 56,
            Capslock = 57,
            F1 = 58,
            F2 = 59,
            F3 = 60,
            F4 = 61,
            F5 = 62,
            F6 = 63,
            F7 = 64,
            F8 = 65,
            F9 = 66,
            F10 = 67,
            F11 = 68,
            F12 = 69,
            Printscreen = 70,
            Scrolllock = 71,
            Pause = 72,
            Insert = 73,
            Home = 74,
            Pageup = 75,
            Delete = 76,
            End = 77,
            Pagedown = 78,
            Right = 79,
            Left = 80,
            Down = 81,
            Up = 82,
            Numlockclear = 83,
            KpDivide = 84,
            KpMultiply = 85,
            KpMinus = 86,
            KpPlus = 87,
            KpEnter = 88,
            Kp1 = 89,
            Kp2 = 90,
            Kp3 = 91,
            Kp4 = 92,
            Kp5 = 93,
            Kp6 = 94,
            Kp7 = 95,
            Kp8 = 96,
            Kp9 = 97,
            Kp0 = 98,
            KpPeriod = 99,
            Nonusbackslash = 100,
            Application = 101,
            Power = 102,
            KpEquals = 103,
            F13 = 104,
            F14 = 105,
            F15 = 106,
            F16 = 107,
            F17 = 108,
            F18 = 109,
            F19 = 110,
            F20 = 111,
            F21 = 112,
            F22 = 113,
            F23 = 114,
            F24 = 115,
            Execute = 116,
            Help = 117,
            Menu = 118,
            Select = 119,
            Stop = 120,
            Again = 121,
            Undo = 122,
            Cut = 123,
            Copy = 124,
            Paste = 125,
            Find = 126,
            Mute = 127,
            Volumeup = 128,
            Volumedown = 129,
            KpComma = 133,
            KpEqualsas400 = 134,
            International1 = 135,
            International2 = 136,
            International3 = 137,
            International4 = 138,
            International5 = 139,
            International6 = 140,
            International7 = 141,
            International8 = 142,
            International9 = 143,
            Lang1 = 144,
            Lang2 = 145,
            Lang3 = 146,
            Lang4 = 147,
            Lang5 = 148,
            Lang6 = 149,
            Lang7 = 150,
            Lang8 = 151,
            Lang9 = 152,
            Alterase = 153,
            Sysreq = 154,
            Cancel = 155,
            Clear = 156,
            Prior = 157,
            Return2 = 158,
            Separator = 159,
            Out = 160,
            Oper = 161,
            Clearagain = 162,
            Crsel = 163,
            Exsel = 164,
            Kp00 = 176,
            Kp000 = 177,
            Thousandsseparator = 178,
            Decimalseparator = 179,
            Currencyunit = 180,
            Currencysubunit = 181,
            KpLeftparen = 182,
            KpRightparen = 183,
            KpLeftbrace = 184,
            KpRightbrace = 185,
            KpTab = 186,
            KpBackspace = 187,
            KpA = 188,
            KpB = 189,
            KpC = 190,
            KpD = 191,
            KpE = 192,
            KpF = 193,
            KpXor = 194,
            KpPower = 195,
            KpPercent = 196,
            KpLess = 197,
            KpGreater = 198,
            KpAmpersand = 199,
            KpDblampersand = 200,
            KpVerticalbar = 201,
            KpDblverticalbar = 202,
            KpColon = 203,
            KpHash = 204,
            KpSpace = 205,
            KpAt = 206,
            KpExclam = 207,
            KpMemstore = 208,
            KpMemrecall = 209,
            KpMemclear = 210,
            KpMemadd = 211,
            KpMemsubtract = 212,
            KpMemmultiply = 213,
            KpMemdivide = 214,
            KpPlusminus = 215,
            KpClear = 216,
            KpClearentry = 217,
            KpBinary = 218,
            KpOctal = 219,
            KpDecimal = 220,
            KpHexadecimal = 221,
            Lctrl = 224,
            Lshift = 225,
            Lalt = 226,
            Lgui = 227,
            Rctrl = 228,
            Rshift = 229,
            Ralt = 230,
            Rgui = 231,
            Mode = 257,
            Sleep = 258,
            Wake = 259,
            Channelincrement = 260,
            Channeldcrement = 261,
            MediaPlay = 262,
            MediaPause = 263,
            MediaRecord = 264,
            MediaFastforward = 265,
            MediaRewind = 266,
            MediaNexttrack = 267,
            MediaPreviousTrack = 268,
            MediaStop = 269,
            MediaEject = 270,
            MediaPlayPause = 271,
            MediaSelect = 272,
            AcNew = 273,
            AcOpen = 274,
            AcClose = 275,
            AcExit = 276,
            AcSave = 277,
            AcPrint = 278,
            AcProperties = 279,
            AcSearch = 280,
            AcHome = 281,
            AcBack = 282,
            AcForward = 283,
            AcStop = 284,
            AcRefresh = 285,
            AcBookmarks = 286,
            Softleft = 287,
            Softright = 288,
            Call = 289,
            Endcall = 290
        }

        public const int SDL_EVENT_KEY_DOWN = 0x300;
        public const int SDL_EVENT_KEY_UP = 0x301;

        public static Scancodes GetScanCode(Key key)
        {
            switch (key)
            {
                case Key.A:
                    return Scancodes.A;
                case Key.B:
                    return Scancodes.B;
                case Key.C:
                    return Scancodes.C;
                case Key.D:
                    return Scancodes.D;
                case Key.E:
                    return Scancodes.E;
                case Key.F:
                    return Scancodes.F;
                case Key.G:
                    return Scancodes.G;
                case Key.H:
                    return Scancodes.H;
                case Key.I:
                    return Scancodes.I;
                case Key.J:
                    return Scancodes.J;
                case Key.K:
                    return Scancodes.K;
                case Key.L:
                    return Scancodes.L;
                case Key.M:
                    return Scancodes.M;
                case Key.N:
                    return Scancodes.N;
                case Key.O:
                    return Scancodes.O;
                case Key.P:
                    return Scancodes.P;
                case Key.Q:
                    return Scancodes.Q;
                case Key.R:
                    return Scancodes.R;
                case Key.S:
                    return Scancodes.S;
                case Key.T:
                    return Scancodes.T;
                case Key.U:
                    return Scancodes.U;
                case Key.V:
                    return Scancodes.V;
                case Key.W:
                    return Scancodes.W;
                case Key.X:
                    return Scancodes.X;
                case Key.Y:
                    return Scancodes.Y;
                case Key.Z:
                    return Scancodes.Z;
                case Key.NumPad1:
                    return Scancodes.Kp1;
                case Key.NumPad2:
                    return Scancodes.Kp2;
                case Key.NumPad3:
                    return Scancodes.Kp3;
                case Key.NumPad4:
                    return Scancodes.Kp4;
                case Key.NumPad5:
                    return Scancodes.Kp5;
                case Key.NumPad6:
                    return Scancodes.Kp6;
                case Key.NumPad7:
                    return Scancodes.Kp7;
                case Key.NumPad8:
                    return Scancodes.Kp8;
                case Key.NumPad9:
                    return Scancodes.Kp9;
                case Key.NumPad0:
                    return Scancodes.Kp0;
                case Key.Enter:
                    return Scancodes.Return;
                case Key.Escape:
                    return Scancodes.Escape;
                case Key.Back:
                    return Scancodes.Backspace;
                case Key.Tab:
                    return Scancodes.Tab;
                case Key.Space:
                    return Scancodes.Space;
                case Key.OemMinus:
                    return Scancodes.Minus;
                case Key.OemPlus:
                    return Scancodes.Equals;
                case Key.OemOpenBrackets:
                    return Scancodes.Leftbracket;
                case Key.OemCloseBrackets:
                    return Scancodes.Rightbracket;
                case Key.OemPipe:
                    return Scancodes.Backslash;
                case Key.OemQuotes:
                    return Scancodes.Apostrophe;
                case Key.OemSemicolon:
                    return Scancodes.Semicolon;
                case Key.OemComma:
                    return Scancodes.Comma;
                case Key.OemPeriod:
                    return Scancodes.Period;
                case Key.OemQuestion:
                    return Scancodes.Slash;
                case Key.CapsLock:
                    return Scancodes.Capslock;
                case Key.F1:
                    return Scancodes.F1;
                case Key.F2:
                    return Scancodes.F2;
                case Key.F3:
                    return Scancodes.F3;
                case Key.F4:
                    return Scancodes.F4;
                case Key.F5:
                    return Scancodes.F5;
                case Key.F6:
                    return Scancodes.F6;
                case Key.F7:
                    return Scancodes.F7;
                case Key.F8:
                    return Scancodes.F8;
                case Key.F9:
                    return Scancodes.F9;
                case Key.F10:
                    return Scancodes.F10;
                case Key.F11:
                    return Scancodes.F11;
                case Key.F12:
                    return Scancodes.F12;
                case Key.Insert:
                    return Scancodes.Insert;
                case Key.Delete:
                    return Scancodes.Delete;
                case Key.Home:
                    return Scancodes.Home;
                case Key.End:
                    return Scancodes.End;
                case Key.PageUp:
                    return Scancodes.Pageup;
                case Key.PageDown:
                    return Scancodes.Pagedown;
                case Key.Left:
                    return Scancodes.Left;
                case Key.Right:
                    return Scancodes.Right;
                case Key.Up:
                    return Scancodes.Up;
                case Key.Down:
                    return Scancodes.Down;
                case Key.NumLock:
                    return Scancodes.Numlockclear;
                case Key.LeftCtrl:
                    return Scancodes.Lctrl;
                case Key.RightCtrl:
                    return Scancodes.Rctrl;
                case Key.LeftShift:
                    return Scancodes.Lshift;
                case Key.RightShift:
                    return Scancodes.Rshift;
                case Key.LeftAlt:
                    return Scancodes.Lalt;
                case Key.RightAlt:
                    return Scancodes.Ralt;
                case Key.LWin:
                    return Scancodes.Lgui;
                case Key.RWin:
                    return Scancodes.Rgui;
                default:
                    return Scancodes.Unknown;
            }
        }
    }
}

