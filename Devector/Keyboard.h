#pragma once
#ifndef DEV_KEYBOARD_H
#define DEV_KEYBOARD_H

#include <cstdint>
#include <map>
#include <functional>

#include "Types.h"

namespace dev
{
    class Keyboard
    {
    private:
        uint8_t matrix[8];
        std::map<SDL_Scancode, uint32_t> keymap;

    public:
        bool ss, us, rus;
        bool terminate;
        std::function<void(bool)> onreset;

        Keyboard();
        void KeyHandling(int _key, int _action);
        int read(int rowbit);

    private:
        void init_map();
    };
}
#endif // !DEV_KEYBOARD_H