// the hardware logic is mostly taken from:
// https://github.com/parallelno/v06x/blob/master/src/board.cpp
// https://github.com/parallelno/v06x/blob/master/src/vio.h

#include "io.h"

#define CW			m_state.ports.CW
#define PORT_A		m_state.ports.portA
#define PORT_B		m_state.ports.portB
#define PORT_C		m_state.ports.portC
#define CW2			m_state.ports.CW2
#define PORT_A2		m_state.ports.portA2
#define PORT_B2		m_state.ports.portB2
#define PORT_C2		m_state.ports.portC2

#define OUT_PORT	m_state.outport
#define OUT_BYTE	m_state.outbyte
#define HW_COLOR	m_state.hwColor
#define REQ_DISPLAY_MODE	m_state.reqDisplayMode
#define JOY_0		m_state.joy0
#define JOY_1		m_state.joy1
#define RUS_LAT		m_state.ruslat

#define BRD_COLOR_IDX	m_state.brdColorIdx
#define DISPLAY_MODE	m_state.displayMode

#define OUT_COMMIT_TIMER			m_state.outCommitTimer
#define PALLETE_COMMIT_TIMER		m_state.paletteCommitTimer
#define DISPLAY_MODE_COMMIT_TIMER	m_state.displayModeTimer

#define PALLETE_BYTES	m_state.palette.bytes
#define PALLETE_LOW		m_state.palette.low
#define PALLETE_HI		m_state.palette.hi

dev::IO::IO(Keyboard& _keyboard, Memory& _memory, TimerI8253& _timer,
	SoundAY8910& _ay, Fdc1793& _fdc)
	:
	m_keyboard(_keyboard), m_memory(_memory), m_timer(_timer),
	m_ay(_ay), m_fdc(_fdc)
{
	Init();
}

void dev::IO::Init()
{
	CW = 0x08;
	CW2 = 0;
	PORT_A = PORT_B = PORT_C = PORT_A2 = PORT_B2 = PORT_C2 = JOY_0 = JOY_1 = 0xFF;
	OUT_PORT = OUT_BYTE = HW_COLOR = BRD_COLOR_IDX = PALLETE_LOW = PALLETE_HI = RUS_LAT = 0;

	DISPLAY_MODE = MODE_256;

	OUT_COMMIT_TIMER = PALLETE_COMMIT_TIMER = DISPLAY_MODE_COMMIT_TIMER = 0;
	m_state.ruslatHistory = 0;
}

// I8080 IN NN
// handling the data receiving from ports
auto dev::IO::PortInHandling(uint8_t _port)
-> uint8_t
{
	int result = 0xFF;

	switch (_port) {
	case 0x00:
		result = 0xFF;
		break;
	case 0x01:
	{
		/* PortC.low input ? */
		auto portCLow = (CW & 0x01) ? 0x0b : PORT_C & 0x0f;
		/* PortC.high input ? */
		auto portCHigh = (CW & 0x08) ?
				/*(tape_player.sample() << 4) |*/
				(m_keyboard.m_keySS  ? 0 : 1 << 5) |
				(m_keyboard.m_keyUS  ? 0 : 1 << 6) |
				(m_keyboard.m_keyRus ? 0 : 1 << 7) : PORT_C & 0xf0;
		result = portCLow | portCHigh;
	}
		break;

	case 0x02:
		if ((CW & 0x02) != 0) {
			result = m_keyboard.Read(PORT_A); // input
		}
		else {
			result = PORT_B;       // output
		}
		break;
	case 0x03:
		if ((CW & 0x10) == 0) {
			result = PORT_A;       // output
		}
		else {
			result = 0xFF;          // input
		}
		break;

		// Parallel Port
	case 0x04:
		result = CW2;
		break;
	case 0x05:
		result = PORT_C2;
		break;
	case 0x06:
		result = PORT_B2;
		break;
	case 0x07:
		result = PORT_A2;
		break;

		// Timer
	case 0x08: [[fallthrough]];
	case 0x09: [[fallthrough]];
	case 0x0a: [[fallthrough]];
	case 0x0b:
		return m_timer.Read(~(_port & 3));

		// Joystick "C"
	case 0x0e:
		return JOY_0;
	case 0x0f:
		return JOY_1;

		// AY
	case 0x14: [[fallthrough]];
	case 0x15:
		result = m_ay.Read(_port & 1);
		break;

		// FFD
	case 0x18:
		result = m_fdc.Read(Fdc1793::Port::DATA);
		break;
	case 0x19:
		result = m_fdc.Read(Fdc1793::Port::SECTOR);
		break;
	case 0x1a:
		result = m_fdc.Read(Fdc1793::Port::TRACK);
		break;
	case 0x1b:
		result = m_fdc.Read(Fdc1793::Port::STATUS);
		break;
	case 0x1c:
		result = m_fdc.Read(Fdc1793::Port::READY);
		break;
	default:
		break;
	}

	return result;
}

// I8080 OUT NN
// called at the commit time. it's data sent by the cpu instruction OUT
void dev::IO::PortOutHandling(uint8_t _port, uint8_t _value)
{
	switch (_port) {
		// PortInputA 
	case 0x00:
		RUS_LAT = (PORT_C >> 3) & 1;
		if ((_value & 0x80) == 0) {
			// port C BSR: 
			//   bit 0: 1 = set, 0 = reset
			//   bit 1-3: bit number
			int bit = (_value >> 1) & 7;
			if ((_value & 1) == 1) {
				PORT_C |= 1 << bit;
			}
			else {
				PORT_C &= ~(1 << bit);
			}
		}
		else {
			CW = _value;
			PortOutHandling(1, 0);
			PortOutHandling(2, 0);
			PortOutHandling(3, 0);
		}
		break;
	case 0x01:
		RUS_LAT = (PORT_C >> 3) & 1;
		m_state.ruslatHistory = (m_state.ruslatHistory<<1) + RUS_LAT;
		PORT_C = _value;
		break;
	case 0x02:
		PORT_B = _value;
		BRD_COLOR_IDX = PORT_B & 0x0f;
		REQ_DISPLAY_MODE = (PORT_B & 0x10) != 0;
		break;
		// Vertical Scrolling
	case 0x03:
		PORT_A = _value;
		break;
		// Parallel Port
	case 0x04:
		CW2 = _value;
		break;
	case 0x05:
		PORT_C2 = _value;
		break;
	case 0x06:
		PORT_B2 = _value;
		break;
	case 0x07:
		PORT_A2 = _value;
		break;

		// Timer
	case 0x08: [[fallthrough]];
	case 0x09: [[fallthrough]];
	case 0x0a: [[fallthrough]];
	case 0x0b:
		m_timer.Write(~_port & 3, _value);
		break;

		// Color pallete
	case PORT_OUT_BORDER_COLOR0: [[fallthrough]];
	case PORT_OUT_BORDER_COLOR1: [[fallthrough]];
	case PORT_OUT_BORDER_COLOR2: [[fallthrough]];
	case PORT_OUT_BORDER_COLOR3:
		HW_COLOR = _value;
		break;

		// Ram Disk 0
	case 0x10:
		m_memory.SetRamDiskMode(0, _value);
		break;
	case 0x11:
		m_memory.SetRamDiskMode(1, _value);
		break;
		// AY
	case 0x14: [[fallthrough]];
	case 0x15:
		m_ay.Write(_port & 1, _value);
		break;

		// FDD
	case 0x18:
		m_fdc.Write(Fdc1793::Port::DATA, _value);
		break;
	case 0x19:
		m_fdc.Write(Fdc1793::Port::SECTOR, _value);
		break;
	case 0x1a:
		m_fdc.Write(Fdc1793::Port::TRACK, _value);
		break;
	case 0x1b:
		m_fdc.Write(Fdc1793::Port::COMMAND, _value);
		break;
	case 0x1c:
		m_fdc.Write(Fdc1793::Port::SYSTEM, _value);
		break;
	case 0x20:

		// Ram Disk
		m_memory.SetRamDiskMode(2, _value);
		break;
	case 0x21:
		m_memory.SetRamDiskMode(3, _value);
		break;
	case 0x40:
		m_memory.SetRamDiskMode(4, _value);
		break;
	case 0x41:
		m_memory.SetRamDiskMode(5, _value);
		break;
	case 0x80:
		m_memory.SetRamDiskMode(6, _value);
		break;
	case 0x81:
		m_memory.SetRamDiskMode(7, _value);
		break;
		// Sends data to the emulator
	case 0xED:
		_port = _port; // TODO: do something meaningful
		break;
	default:
		break;
	}
}

// cpu IN instruction callback
auto dev::IO::PortIn(uint8_t _port)
->uint8_t
{
	auto out = PortInHandling(_port);
	// store it for debbuging
	m_portsInData.data[_port] = out;
	return out;
}

// cpu OUT instruction callback
void dev::IO::PortOut(uint8_t _port, uint8_t _value)
{
	// store it for debbuging
	m_portsOutData.data[_port] = _value;

	// store port/val data to handle when ports are available (commit time)
	OUT_PORT = _port;
	OUT_BYTE = _value;

	// set the commit time for port output
	OUT_COMMIT_TIMER = m_outCommitTime;

	// set the palette commit time
	switch (_port) {
	case PORT_OUT_BORDER_COLOR0: [[fallthrough]];
	case PORT_OUT_BORDER_COLOR1: [[fallthrough]];
	case PORT_OUT_BORDER_COLOR2: [[fallthrough]];
	case PORT_OUT_BORDER_COLOR3:
		PALLETE_COMMIT_TIMER = m_paletteCommitTime;
		break;
	case PORT_OUT_DISPLAY_MODE:
		DISPLAY_MODE_COMMIT_TIMER = m_displayModeTime;
		break;
	}
}

void dev::IO::TryToCommit(const uint8_t _colorIdx)
{
	if (OUT_COMMIT_TIMER > 0){
		if (--OUT_COMMIT_TIMER == 0)
		{
			PortOutCommit();
		}
	}

	if (PALLETE_COMMIT_TIMER > 0) {
		if (--PALLETE_COMMIT_TIMER == 0)
		{
			SetColor(_colorIdx);
		}
	}

	if (DISPLAY_MODE_COMMIT_TIMER > 0) {
		if (--DISPLAY_MODE_COMMIT_TIMER == 0)
		{
			DISPLAY_MODE = REQ_DISPLAY_MODE;
		}
	}
}

void dev::IO::PortOutCommit()
{
	PortOutHandling(OUT_PORT, OUT_BYTE);
}