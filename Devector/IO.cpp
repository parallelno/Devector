#include "IO.h"

dev::IO::IO(Keyboard& _keyboard)
    :
    m_keyboard(_keyboard),
    CW(0x08), PA(0xff), PB(0xff), PC(0xff), CW2(0), PA2(0xff), PB2(0xff), PC2(0xff),
    outport(-1), outbyte(-1), palettebyte(- 1),
    joy_0e(0xff), joy_0f(0xff)

{
    m_palette.fill(0xFF000000);
}

auto dev::IO::PortIn(uint8_t _port)
-> uint8_t
{
    int result = 0xff;

    switch (_port) {
    case 0x00:
        result = 0xff;
        break;
    case 0x01:
    {
        /* PC.low input ? */
        auto pclow = (CW & 0x01) ? 0x0b : (PC & 0x0f);
        /* PC.high input ? */
        auto pcupp = (CW & 0x08) ?
            (/*(this->tape_player.sample() << 4) |*/
                (m_keyboard.ss ? 0 : (1 << 5)) |
                (m_keyboard.us ? 0 : (1 << 6)) |
                (m_keyboard.rus ? 0 : (1 << 7))) : (PC & 0xf0);
        result = pclow | pcupp;
    }
        break;

    case 0x02:
        if ((this->CW & 0x02) != 0) {
            result = m_keyboard.read(~this->PA); // input
        }
        else {
            result = this->PB;       // output
        }
        break;
    case 0x03:
        if ((this->CW & 0x10) == 0) {
            result = this->PA;       // output
        }
        else {
            result = 0xff;          // input
        }
        break;

    case 0x04:
        result = this->CW2;
        break;
    case 0x05:
        result = this->PC2;
        break;
    case 0x06:
        result = this->PB2;
        break;
    case 0x07:
        result = this->PA2;
        break;

        // Timer
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
        //return this->timer.read(~(port & 3));

        // Joystick "C"
    case 0x0e:
        return this->joy_0e;
    case 0x0f:
        return this->joy_0f;

    case 0x14:
    case 0x15:
        //result = this->ay.read(port & 1);
        break;

    case 0x18: // fdc data
        //result = this->fdc.read(3);
        break;
    case 0x19: // fdc sector
        //result = this->fdc.read(2);
        break;
    case 0x1a: // fdc track
        //result = this->fdc.read(1);
        break;
    case 0x1b: // fdc status
        //result = this->fdc.read(0);
        break;
    case 0x1c: // fdc control - readonly
        //result = this->fdc.read(4); // ask Svofski why it is disabled
        break;
    default:
        break;
    }

    return result;
}

void dev::IO::PortOut(uint8_t _port, uint8_t _value)
{
    int a = 0;
}

auto dev::IO::GetKeyboard()
-> Keyboard&
{
    return m_keyboard;
}
