#pragma once

#include <array>
#include <inttypes.h>

namespace dev
{
    class CounterUnit
    {
        int m_latchValue = -1;
        int m_writeState = 0;
        int m_latchMode = 0;
        int m_out = 0;
        int m_value = 0;
        int m_modeInt = 0;

        uint8_t m_writeLsb = 0;
        uint8_t m_writeMsb = 0;
        uint16_t m_loadValue = 0;

        int m_delay;

        union {
            uint32_t m_flags = 0;
            struct {
                bool m_flagArmed : 1;
                bool m_flagLoad : 1;
                bool m_flagEnabled : 1;
                bool m_flagBcd : 1;
            };
        };

    public:
        CounterUnit() { Reset(); }
        void Reset();
        void SetMode(int _mode, int _latchMode, bool _flagBcd);
        void Latch();
        int Clock(int _cycles);
        void Write(uint8_t _w8);
        int Read();
        static uint16_t ToBcd(uint16_t _x);
        static uint16_t FromBcd(uint16_t _x);
    };


    class TimerI8253
    {
    private:
        CounterUnit m_counters[3];
        uint8_t m_controlWord = 0;

    public:
        void Reset();
        void write_cw(uint8_t _w8);
        void Write(int _addr, uint8_t _w8);
        auto Read(int _addr) -> int;
        auto Clock(int _cycles) -> float;
    };
}