#include "TimerI8253.h"

//-------------------------------------------------------------
//
// CounterUnit
//
//-------------------------------------------------------------

#define WRITE_DELAY 2
#define LATCH_DELAY 1
#define READ_DELAY  0

void dev::CounterUnit::reset()
{
    latch_value = -1;
    write_state = value = mode_int = loadvalue = flags = delay = 0;
    write_lsb = write_msb = out = latch_mode = 0;
}

void dev::CounterUnit::SetMode(int new_mode, int new_latch_mode, int new_bcd_mode)
{
    this->Count(LATCH_DELAY);
    this->delay = LATCH_DELAY;

    this->bcd = new_bcd_mode;
    if ((new_mode & 0x03) == 2) {
        this->mode_int = 2;
    }
    else if ((new_mode & 0x03) == 3) {
        this->mode_int = 3;
    }
    else {
        this->mode_int = new_mode;
    }

    switch (this->mode_int) {
    case 0:
        this->out = 0;
        this->armed = true;
        this->enabled = false;
        break;
    case 1:
        this->out = 1;
        this->armed = true;
        this->enabled = false;
        break;
    case 2:
        this->out = 1;
        this->enabled = false;
        // armed?
        break;
    default:
        this->out = 1;
        this->enabled = false;
        // armed?
        break;
    }
    this->load = false;
    this->latch_mode = new_latch_mode;
    this->write_state = 0;
}

void dev::CounterUnit::Latch(uint8_t w8) 
{
    this->Count(LATCH_DELAY);
    this->delay = LATCH_DELAY;
    this->latch_value = this->value;
}

int dev::CounterUnit::Count(int incycles)
{
    int cycles = 1; //incycles;
    if (this->delay) {
        --this->delay;
        cycles = 0;
    }
    if (!cycles) return this->out;
    if (!this->enabled && !this->load) return this->out;
    int result = this->out;

    switch (this->mode_int) {
    case 0: // Interrupt on terminal count
        if (this->load) {
            this->value = this->loadvalue;
            this->enabled = true;
            this->armed = true;
            this->out = 0;
            result = 0;
        }
        if (this->enabled) {
            int previous = this->value;
            this->value -= cycles;
            if (this->value <= 0) {
                if (this->armed) {
                    if (previous != 0) this->out = 1;
                    result = -this->value + 1;
                    this->armed = false;
                }
                this->value += this->bcd ? 10000 : 65536;
            }
        }
        break;
    case 1: // Programmable one-shot
        if (!this->enabled && this->load) {
            //this->value = this->loadvalue; -- quirk!
            this->enabled = true;
        }
        if (this->enabled) {
            this->value -= cycles;
            if (this->value <= 0) {
                int reload = this->loadvalue == 0 ?
                    (this->bcd ? 10000 : 0x10000) : (this->loadvalue + 1);
                this->value += reload;
                //this->value += this->loadvalue + 1;
            }
        }
        break;
    case 2: // Rate generator
        if (!this->enabled && this->load) {
            this->value = this->loadvalue;
            this->enabled = true;
        }
        if (this->enabled) {
            this->value -= cycles;
            if (this->value <= 0) {
                int reload = this->loadvalue == 0 ?
                    (this->bcd ? 10000 : 0x10000) : this->loadvalue;
                this->value += reload;
                //this->value += this->loadvalue;
            }
        }
        // out will go low for one clock pulse but in our machine it should not be 
        // audible
        break;
    case 3: // Square wave generator
        if (!this->enabled && this->load) {
            this->value = this->loadvalue;
            this->enabled = true;
        }
        if (this->enabled) {
            this->value -=
                ((this->value == this->loadvalue) && ((this->value & 1) == 1)) ?
                this->out == 0 ? 3 : 1 : 2;
            if (this->value <= 0) {
                this->out ^= 1;

                int reload = (this->loadvalue == 0) ?
                    (this->bcd ? 10000 : 0x10000) : this->loadvalue;
                this->value += reload;
                //this->value = this->loadvalue;
            }
        }
        result = this->out;
        break;
    case 4: // Software triggered strobe
        break;
    case 5: // Hardware triggered strobe
        break;
    default:
        break;
    }

    this->load = false;
    return result;
}

void dev::CounterUnit::write_value(uint8_t w8) 
{
    if (this->latch_mode == 3) {
        // lsb, msb             
        switch (this->write_state) {
        case 0:
            this->write_lsb = w8;
            this->write_state = 1;
            break;
        case 1:
            this->write_msb = w8;
            this->write_state = 0;
            this->loadvalue = ((this->write_msb << 8) & 0xffff) |
                (this->write_lsb & 0xff);
            this->load = true;
            break;
        default:
            break;
        }
    }
    else if (this->latch_mode == 1) {
        // lsb only
        this->loadvalue = w8;
        this->load = true;
    }
    else if (this->latch_mode == 2) {
        // msb only 
        this->value = w8 << 8;
        this->value &= 0xffff;
        this->loadvalue = this->value;
        this->load = true;
    }
    if (this->load) {
        if (this->bcd) {
            this->loadvalue = frombcd(this->loadvalue);
        }
        // I'm deeply sorry about the following part
        switch (this->mode_int) {
        case 0:
            this->delay = 3; break;
        case 1:
            if (!this->enabled) {
                this->delay = 3;
            }
            break;
        case 2:
            if (!this->enabled) {
                this->delay = 3;
            }
            break;
        case 3:
            if (!this->enabled) {
                this->delay = 3;
            }
            break;
        default:
            this->delay = 4;
            break;
        }
    }
}

int dev::CounterUnit::read_value()
{
    int value = 0;
    switch (this->latch_mode) {
    case 0:
        // impossibru
        break;
    case 1:
        value = this->latch_value != -1 ? this->latch_value : this->value;
        this->latch_value = -1;
        value = this->bcd ? tobcd(value) : value;
        value &= 0xff;
        break;
    case 2:
        value = this->latch_value != -1 ? this->latch_value : this->value;
        this->latch_value = -1;
        value = this->bcd ? tobcd(value) : value;
        value = (value >> 8) & 0xff;
        break;
    case 3:
        value = this->latch_value != -1 ? this->latch_value : this->value;
        value = this->bcd ? tobcd(value) : value;
        switch (this->write_state) {
        case 0:
            this->write_state = 1;
            value = value & 0xff;
            break;
        case 1:
            this->latch_value = -1;
            this->write_state = 0;
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

uint16_t dev::CounterUnit::tobcd(uint16_t x) 
{
    int result = 0;
    for (int i = 0; i < 4; ++i) {
        result |= (x % 10) << (i * 4);
        x /= 10;
    }
    return result;
}

uint16_t dev::CounterUnit::frombcd(uint16_t x) 
{
    int result = 0;
    for (int i = 0; i < 4; ++i) {
        int digit = (x & 0xf000) >> 12;
        if (digit > 9) digit = 9;
        result = result * 10 + digit;
        x <<= 4;
    }
    return result;
}

//-------------------------------------------------------------
//
// TimerI8253
//
//-------------------------------------------------------------

void dev::TimerI8253::reset()
{
    counters[0].reset();
    counters[1].reset();
    counters[2].reset();
};

void dev::TimerI8253::write_cw(uint8_t w8)
{
    unsigned counter_set = (w8 >> 6) & 3;
    int mode_set = (w8 >> 1) & 3;
    int latch_set = (w8 >> 4) & 3;
    int bcd_set = (w8 & 1);

    if ((unsigned)counter_set >= sizeof(counters) / sizeof(counters[0])) {
        // error
        return;
    }

    CounterUnit& ctr = this->counters[counter_set];
    if (latch_set == 0) {
        ctr.Latch(latch_set);
    }
    else {
        ctr.SetMode(mode_set, latch_set, bcd_set);
    }
}

void dev::TimerI8253::write(int addr, uint8_t w8)
{
    switch (addr & 3) {
    case 0x03:
        return this->write_cw(w8);
    default:
        return this->counters[addr & 3].write_value(w8);
    }
}

int dev::TimerI8253::read(int addr)
{
    switch (addr & 3) {
    case 0x03:
        return this->control_word;
    default:
        return this->counters[addr & 3].read_value();
    }
}

/*int dev::TimerI8253::Count(int cycles)
{
    return this->counters[0].Count(cycles) +
        this->counters[1].Count(cycles) +
        this->counters[2].Count(cycles);
}*/

void dev::TimerI8253::Count(int cycles, int& c0, int& c1, int& c2)
{
    c0 = this->counters[0].Count(cycles);
    c1 = this->counters[1].Count(cycles);
    c2 = this->counters[2].Count(cycles);
}