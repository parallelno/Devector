#include "TimerI8253.h"

//-------------------------------------------------------------
//
// CounterUnit
//
//-------------------------------------------------------------

#define WRITE_DELAY 2
#define LATCH_DELAY 1
#define READ_DELAY  0

void dev::CounterUnit::Reset()
{
    m_latchValue = -1;
    m_writeState = m_value = m_modeInt = m_loadValue = m_flags = m_delay = 0;
    m_writeLsb = m_writeMsb = m_out = m_latchMode = 0;
}

void dev::CounterUnit::SetMode(int _mode, int _latchMode, bool _flagBcd)
{
    Tick(LATCH_DELAY);
    m_delay = LATCH_DELAY;

    m_flagBcd = _flagBcd;
    if ((_mode & 0x03) == 2) {
        m_modeInt = 2;
    }
    else if ((_mode & 0x03) == 3) {
        m_modeInt = 3;
    }
    else {
        m_modeInt = _mode;
    }

    switch (m_modeInt) {
    case 0:
        m_out = 0;
        m_flagArmed = true;
        m_flagEnabled = false;
        break;
    case 1:
        m_out = 1;
        m_flagArmed = true;
        m_flagEnabled = false;
        break;
    case 2:
        m_out = 1;
        m_flagEnabled = false;
        // armed?
        break;
    default:
        m_out = 1;
        m_flagEnabled = false;
        // armed?
        break;
    }
    m_flagLoad = false;
    m_latchMode = _latchMode;
    m_writeState = 0;
}

void dev::CounterUnit::Latch() 
{
    Tick(LATCH_DELAY);
    m_delay = LATCH_DELAY;
    m_latchValue = m_value;
}

int dev::CounterUnit::Tick(int _cycles)
{
    //int cycles = 1; //incycles;

    if (m_delay) {
        --m_delay;
        _cycles = 0;
    }
    if (!_cycles) return m_out;
    if (!m_flagEnabled && !m_flagLoad) return m_out;

    int result = m_out;

    switch (m_modeInt) {
    case 0: // Interrupt on terminal count
        if (m_flagLoad) {
            m_value = m_loadValue;
            m_flagEnabled = true;
            m_flagArmed = true;
            m_out = 0;
            result = 0;
        }
        if (m_flagEnabled) {
            int previous = m_value;
            m_value -= _cycles;
            if (m_value <= 0) {
                if (m_flagArmed) {
                    if (previous != 0) m_out = 1;
                    result = -m_value + 1;
                    m_flagArmed = false;
                }
                m_value += m_flagBcd ? 10000 : 65536;
            }
        }
        break;
    case 1: // Programmable one-shot
        if (!m_flagEnabled && m_flagLoad) {
            //value = loadvalue; -- quirk!
            m_flagEnabled = true;
        }
        if (m_flagEnabled) {
            m_value -= _cycles;
            if (m_value <= 0) {
                int reload = m_loadValue == 0 ?
                    (m_flagBcd ? 10000 : 0x10000) : (m_loadValue + 1);
                m_value += reload;
                //value += loadvalue + 1;
            }
        }
        break;
    case 2: // Rate generator
        if (!m_flagEnabled && m_flagLoad) {
            m_value = m_loadValue;
            m_flagEnabled = true;
        }
        if (m_flagEnabled) {
            m_value -= _cycles;
            if (m_value <= 0) {
                int reload = m_loadValue == 0 ?
                    (m_flagBcd ? 10000 : 0x10000) : m_loadValue;
                m_value += reload;
                //value += loadvalue;
            }
        }
        // out will go low for one clock pulse but in our machine it should not be 
        // audible
        break;
    case 3: // Square wave generator
        if (!m_flagEnabled && m_flagLoad) {
            m_value = m_loadValue;
            m_flagEnabled = true;
        }
        if (m_flagEnabled) 
        {
            m_value -=
                m_value == m_loadValue && (m_value & 1) == 1 ?
                (m_out == 0 ? 3 : 1) : 2;

            if (m_value <= 0) 
            {
                m_out ^= 1;

                int reload = (m_loadValue == 0) ?
                    (m_flagBcd ? 10000 : 0x10000) : m_loadValue;
                m_value += reload;
                //value = loadvalue;
            }
        }
        result = m_out;
        break;
    case 4: // Software triggered strobe
        break;
    case 5: // Hardware triggered strobe
        break;
    default:
        break;
    }

    m_flagLoad = false;
    return result;
}

void dev::CounterUnit::Write(uint8_t _w8) 
{
    if (m_latchMode == 3) {
        // lsb, msb             
        switch (m_writeState) {
        case 0:
            m_writeLsb = _w8;
            m_writeState = 1;
            break;
        case 1:
            m_writeMsb = _w8;
            m_writeState = 0;
            m_loadValue = ((m_writeMsb << 8) & 0xffff) |
                (m_writeLsb & 0xff);
            m_flagLoad = true;
            break;
        default:
            break;
        }
    }
    else if (m_latchMode == 1) {
        // lsb only
        m_loadValue = _w8;
        m_flagLoad = true;
    }
    else if (m_latchMode == 2) {
        // msb only 
        m_value = _w8 << 8;
        m_value &= 0xffff;
        m_loadValue = m_value;
        m_flagLoad = true;
    }
    if (m_flagLoad) {
        if (m_flagBcd) {
            m_loadValue = FromBcd(m_loadValue);
        }
        // I'm deeply sorry about the following part
        switch (m_modeInt) {
        case 0:
            m_delay = 3; break;
        case 1:
            if (!m_flagEnabled) {
                m_delay = 3;
            }
            break;
        case 2:
            if (!m_flagEnabled) {
                m_delay = 3;
            }
            break;
        case 3:
            if (!m_flagEnabled) {
                m_delay = 3;
            }
            break;
        default:
            m_delay = 4;
            break;
        }
    }
}

int dev::CounterUnit::Read()
{
    int value = 0;
    switch (m_latchMode) {
    case 0:
        // impossibru
        break;
    case 1:
        value = m_latchValue != -1 ? m_latchValue : m_value;
        m_latchValue = -1;
        value = m_flagBcd ? ToBcd(value) : value;
        value &= 0xff;
        break;
    case 2:
        value = m_latchValue != -1 ? m_latchValue : m_value;
        m_latchValue = -1;
        value = m_flagBcd ? ToBcd(value) : value;
        value = (value >> 8) & 0xff;
        break;
    case 3:
        value = m_latchValue != -1 ? m_latchValue : m_value;
        value = m_flagBcd ? ToBcd(value) : value;
        switch (m_writeState) {
        case 0:
            m_writeState = 1;
            value = value & 0xff;
            break;
        case 1:
            m_latchValue = -1;
            m_writeState = 0;
            value = (value >> 8) & 0xff;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return value;
}

uint16_t dev::CounterUnit::ToBcd(uint16_t _x) 
{
    int result = 0;
    for (int i = 0; i < 4; ++i) {
        result |= (_x % 10) << (i * 4);
        _x /= 10;
    }
    return result;
}

uint16_t dev::CounterUnit::FromBcd(uint16_t _x) 
{
    int result = 0;
    for (int i = 0; i < 4; ++i) {
        int digit = (_x & 0xf000) >> 12;
        if (digit > 9) digit = 9;
        result = result * 10 + digit;
        _x <<= 4;
    }
    return result;
}

//-------------------------------------------------------------
//
// TimerI8253
//
//-------------------------------------------------------------

void dev::TimerI8253::Reset()
{
    m_counters[0].Reset();
    m_counters[1].Reset();
    m_counters[2].Reset();
};

void dev::TimerI8253::write_cw(uint8_t _w8)
{
    unsigned counterSet = (_w8 >> 6) & 3;
    int modeSet = (_w8 >> 1) & 3;
    int latchSet = (_w8 >> 4) & 3;
    int bcdSet = (_w8 & 1);

    if ((unsigned)counterSet >= sizeof(m_counters) / sizeof(m_counters[0])) {
        // error
        return;
    }

    CounterUnit& counter = m_counters[counterSet];

    if (latchSet == 0) {
        counter.Latch();
    }
    else {
        counter.SetMode(modeSet, latchSet, bcdSet);
    }
}

void dev::TimerI8253::Write(int _addr, uint8_t _w8)
{
    switch (_addr & 3) {
    case 0x03:
        return write_cw(_w8);
    default:
        return m_counters[_addr & 3].Write(_w8);
    }
}

int dev::TimerI8253::Read(int _addr)
{
    switch (_addr & 3) {
    case 0x03:
        return m_controlWord;
    default:
        return m_counters[_addr & 3].Read();
    }
}

auto dev::TimerI8253::Tick(int _cycles)
-> float
{
    auto ch0 = m_counters[0].Tick(_cycles);
    auto ch1 = m_counters[1].Tick(_cycles);
    auto ch2 = m_counters[2].Tick(_cycles);
    return (ch0 + ch1 + ch2) / 3.0f;
}