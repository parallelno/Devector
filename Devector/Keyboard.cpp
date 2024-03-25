#include "Keyboard.h"

#include "GLFW/glfw3.h"

dev::Keyboard::Keyboard() 
    : 
    ss(false), us(false), rus(false), terminate(false)
{
    memset(matrix, 0, sizeof(matrix));
    init_map();
}

void dev::Keyboard::KeyHandling(int _key, int _action)
{
    int col, bit;

    switch (_key)
    {
    case GLFW_KEY_F11:
        if (_action == GLFW_PRESS) onreset(true);
        break;
    case GLFW_KEY_F12:
        if (_action == GLFW_PRESS) onreset(false);
        break;
    case GLFW_KEY_PAUSE:
        // on windows ctrl+break seems to magically become scroll lock, wtf
        if (_action == GLFW_PRESS) terminate = true; // this->us;
        break;
        // shift keys
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
        ss = _action == GLFW_PRESS;
        break; // shift/ss
    case GLFW_KEY_LEFT_CONTROL:
        us = _action == GLFW_PRESS;
        break; // ctrl/us
    case GLFW_KEY_LEFT_SUPER:
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_F6:
        this->rus = _action == GLFW_PRESS;
        break; // cmd/rus
        // matrix keys
    default:
        //if (sym == 8 && this->ss) {
        //    sym = 308;
        //}
        //if (sym == 61) {
        //    //sym = plus;
        //    sym = 187;
        //} else if (sym == 59) {
        //    sym = 186;
        //    //sym = semicolon;
        //}
        //console.log("sym=", sym, " keymap: ", keymap[sym]);

        auto it = keymap.find(_key);
        if (it != keymap.end()) {
            //uint32_t colbit = keymap[sym];
            uint32_t colbit = it->second;

            col = (colbit >> 8) & 0377;
            bit = colbit & 0377;
            if (col != -1) {
                if (_action == GLFW_RELEASE) {
                    this->matrix[col] &= ~bit;
                    //printf("UP: this->matrix[%d] = %02x\n", col, this->matrix[col]);
                }
                else {
                    this->matrix[col] |= bit;
                    //printf("DN: this->matrix[%d] = %02x\n", col, this->matrix[col]);
                }
            }
        }
        else {
            printf("unknown scancode: %d\n", _key);
        }
        break;
    }
    //debug = !keyup;
};

int dev::Keyboard::read(int rowbit)
{
    int result = 0;
    for (int i = 0; i < 8; ++i) {
        if ((rowbit & 1) != 0) {
            result |= this->matrix[i];
        }
        rowbit >>= 1;
    }
    return (~result) & 0377;
}

void dev::Keyboard::init_map()
{
    // Keyboard encoding matrix:
    //   │ 7   6   5   4   3   2   1   0
    // ──┼───────────────────────────────
    // 7 │SPC  ^   ]   \   [   Z   Y   X
    // 6 │ W   V   U   T   S   R   Q   P
    // 5 │ O   N   M   L   K   J   I   H
    // 4 │ G   F   E   D   C   B   A   @
    // 3 │ /   .   =   ,   ;   :   9   8
    // 2 │ 7   6   5   4   3   2   1   0
    // 1 │F5  F4  F3  F2  F1  AR2 STR ^\ -
    // 0 │DN  RT  UP  LT  ZAB VK  PS  TAB

    static int keymap_tab[] = {
        /* scancode             column      bit     */
        SDL_SCANCODE_SPACE,         0x780,
        SDL_SCANCODE_GRAVE,         0x701,
        SDL_SCANCODE_RIGHTBRACKET,  0x720,
        SDL_SCANCODE_BACKSLASH,     0x710,
        SDL_SCANCODE_LEFTBRACKET,   0x708,
        SDL_SCANCODE_Z,             0x704,
        SDL_SCANCODE_Y,             0x702,
        SDL_SCANCODE_X,             0x701,

        SDL_SCANCODE_W,             0x680,
        SDL_SCANCODE_V,             0x640,
        SDL_SCANCODE_U,             0x620,
        SDL_SCANCODE_T,             0x610,
        SDL_SCANCODE_S,             0x608,
        SDL_SCANCODE_R,             0x604,
        SDL_SCANCODE_Q,             0x602,
        SDL_SCANCODE_P,             0x601,

        SDL_SCANCODE_O,             0x580,
        SDL_SCANCODE_N,             0x540,
        SDL_SCANCODE_M,             0x520,
        SDL_SCANCODE_L,             0x510,
        SDL_SCANCODE_K,             0x508,
        SDL_SCANCODE_J,             0x504,
        SDL_SCANCODE_I,             0x502,
        SDL_SCANCODE_H,             0x501,

        SDL_SCANCODE_G,             0x480,
        SDL_SCANCODE_F,             0x440,
        SDL_SCANCODE_E,             0x420,
        SDL_SCANCODE_D,             0x410,
        SDL_SCANCODE_C,             0x408,
        SDL_SCANCODE_B,             0x404,
        SDL_SCANCODE_A,             0x402,
        SDL_SCANCODE_MINUS,         0x401, // 189:-@

        SDL_SCANCODE_SLASH,         0x380,
        SDL_SCANCODE_PERIOD,        0x340,
        SDL_SCANCODE_EQUALS,        0x320,
        SDL_SCANCODE_COMMA,         0x310,
        SDL_SCANCODE_SEMICOLON,     0x308,
        SDL_SCANCODE_APOSTROPHE,    0x304,
        SDL_SCANCODE_9,             0x302,
        SDL_SCANCODE_8,             0x301,

        SDL_SCANCODE_7,             0x280,
        SDL_SCANCODE_6,             0x240,
        SDL_SCANCODE_5,             0x220,
        SDL_SCANCODE_4,             0x210,
        SDL_SCANCODE_3,             0x208,
        SDL_SCANCODE_2,             0x204,
        SDL_SCANCODE_1,             0x202,
        SDL_SCANCODE_0,             0x201,

        SDL_SCANCODE_F5,            0x180,
        SDL_SCANCODE_F4,            0x140,
        SDL_SCANCODE_F3,            0x120,
        SDL_SCANCODE_F2,            0x110,
        SDL_SCANCODE_F1,            0x108,
        SDL_SCANCODE_ESCAPE,        0x104,
        SDL_SCANCODE_F8,            0x102, // STR
        SDL_SCANCODE_F7,            0x101, // ^\ ?

        SDL_SCANCODE_DOWN,          0x080,
        SDL_SCANCODE_RIGHT,         0x040,
        SDL_SCANCODE_UP,            0x020,
        SDL_SCANCODE_LEFT,          0x010,
        SDL_SCANCODE_BACKSPACE,     0x008,
        SDL_SCANCODE_RETURN,        0x004,
        SDL_SCANCODE_RALT,          0x002,
        SDL_SCANCODE_TAB,           0x001,
    };

    for (unsigned i = 0; i < sizeof(keymap_tab) / sizeof(keymap_tab[0]); i += 2) {
        SDL_Scancode scan = (SDL_Scancode)keymap_tab[i];
        uint32_t colbit = keymap_tab[i + 1];
        keymap[scan] = colbit;
    }
}
