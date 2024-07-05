#pragma once

#include <array>
#include <inttypes.h>

namespace dev
{
    class CounterUnit
    {
        friend class TestOfCounterUnit;

        int latch_value = -1;
        int write_state = 0;
        int latch_mode = 0;
        int out = 0;
        int value = 0;
        int mode_int = 0;

        uint8_t write_lsb = 0;
        uint8_t write_msb = 0;
        uint16_t loadvalue = 0;

        union {
            uint32_t flags = 0;
            struct {
                bool armed : 1;
                bool load : 1;
                bool enabled : 1;
                bool bcd : 1;
            };
        };

        int delay;

    public:
        CounterUnit() { reset(); }
        void reset();
        void SetMode(int new_mode, int new_latch_mode, int new_bcd_mode);
        void Latch(uint8_t w8);
        int Count(int incycles);
        void write_value(uint8_t w8);
        int read_value();
        static uint16_t tobcd(uint16_t x);
        static uint16_t frombcd(uint16_t x);
    };


    class TimerI8253
    {
    private:
        CounterUnit counters[3];
        uint8_t control_word;

    public:
        TimerI8253() : control_word(0) {}
        void reset();
        void write_cw(uint8_t w8);
        void write(int addr, uint8_t w8);
        int read(int addr);
        //int Count(int cycles);
        void Count(int cycles, int& c0, int& c1, int& c2);
    };

    class TimerWrapper
    {
    private:
        TimerI8253& timer;
        int sound;
        int average_count;
        int last_sound;
    public:
        TimerWrapper(TimerI8253& _timer) : timer(_timer),
            sound(0), average_count(0), last_sound(0)
        {}

        // step with enable per chan?
        //
        /*int step(int cycles)
        {
            this->last_sound = this->timer.Count(cycles) / cycles;
            return this->last_sound;
        }*/

        int singlestep(int ena_ch0, int ena_ch1, int ena_ch2)
        {
            int ch0, ch1, ch2;
            this->timer.Count(1, ch0, ch1, ch2);
            this->last_sound = ch0 * ena_ch0 + ch1 * ena_ch1 + ch2 * ena_ch2;
            return this->last_sound;
        }
        void reset()
        {
            this->timer.reset();
        };
    };

}