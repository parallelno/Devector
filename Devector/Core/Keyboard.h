#pragma once
#ifndef DEV_KEYBOARD_H
#define DEV_KEYBOARD_H

#include <cstdint>
#include <map>
#include <functional>

#include "Utils/Types.h"

namespace dev
{
    class Keyboard
    {
    private:
        uint8_t m_encodingMatrix[8];
        using KeyCode = int;
        using RowColumnCode = int;
        std::map<KeyCode, RowColumnCode> m_keymap;

    public:
        bool m_keySS, m_keyUS, m_keyRus;
        bool m_terminate;
        std::function<void(bool)> onreset;

        Keyboard();
        void KeyHandling(int _key, int _action);
        auto Read(int _rows) -> uint8_t;

    private:
        void init_map();
    };
}
#endif // !DEV_KEYBOARD_H