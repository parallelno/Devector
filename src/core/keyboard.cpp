#include "core/keyboard.h"
#include "utils/utils.h"

dev::Keyboard::Keyboard()
{
	memset(m_encodingMatrix, 0, sizeof(m_encodingMatrix));
	InitMapping();
}

// Hardware thread
auto dev::Keyboard::KeyHandling(int _scancode, int _action)
-> Operation
{
	int row, column;

	switch (_scancode)
	{
		// BLK + VVOD functionality
	case SDL_SCANCODE_F11:
		if (_action == SDL_EVENT_KEY_UP) {
			return Operation::RESET;
		}
		break;

		// BLK + SBR functionality		
	case SDL_SCANCODE_F12:
		if (_action == SDL_EVENT_KEY_UP) {
			return Operation::RESTART;
		}
		break;
		
		// SS (shift) key
	case SDL_SCANCODE_LSHIFT: [[fallthrough]];
	case SDL_SCANCODE_RSHIFT:
		m_keySS = _action == SDL_EVENT_KEY_DOWN;
		break;
		
		// US (ctrl) key
	case SDL_SCANCODE_LCTRL:
		m_keyUS = _action == SDL_EVENT_KEY_DOWN;
		break;

		// RUS/LAT (cmd) key
	case SDL_SCANCODE_LGUI: [[fallthrough]];
	case SDL_SCANCODE_LALT: [[fallthrough]];
	case SDL_SCANCODE_F6:
		m_keyRus = _action == SDL_EVENT_KEY_DOWN;
		break;
		
		// Matrix keys
	default:

		auto it = m_keymap.find(_scancode);
		if (it != m_keymap.end()) 
		{
			auto rowColumn = it->second;
			row = rowColumn >> 8;
			column = rowColumn & 0xFF;

			if (_action == SDL_EVENT_KEY_UP) {
				m_encodingMatrix[row] &= ~column;
			}
			else {
				m_encodingMatrix[row] |= column;
			}
		}
		else {
			//dev::Log("unknown keycode: {}\n", key);
		}
		break;
	}

	return Operation::NONE;
};

auto dev::Keyboard::Read(int _rows)
-> uint8_t
{
	uint8_t result = 0;
	for (auto row = 0; row < 8; ++row)
	{
		auto rowBit = 1 << row;
		result |= (_rows & rowBit) == 0 ? m_encodingMatrix[row] : 0;
	}
	return ~result;
}

void dev::Keyboard::InitMapping()
{
	// Keyboard encoding matrix:
	//              columns
	//     │ 7   6   5   4   3   2   1   0
	// ────┼───────────────────────────────
	//   7 │SPC  ^   ]   \   [   Z   Y   X
	//   6 │ W   V   U   T   S   R   Q   P
	// r 5 │ O   N   M   L   K   J   I   H
	// o 4 │ G   F   E   D   C   B   A   @
	// w 3 │ /   .   =   ,   ;   :   9   8
	// s 2 │ 7   6   5   4   3   2   1   0
	//   1 │F5  F4  F3  F2  F1  AR2 STR LDA,
	//   0 │DN  RT  UP  LFT ZB  VK  PS  TAB
	//
	// LDA - left diagonal arrow

	m_keymap = {
		// KeyCode				RowColumnCode = row<<8 | 1<<column
		{ SDL_SCANCODE_SPACE,		0x780 },
		{ SDL_SCANCODE_GRAVE,		0x701 },
		{ SDL_SCANCODE_RIGHTBRACKET,0x720 },
		{ SDL_SCANCODE_BACKSLASH,	0x710 },
		{ SDL_SCANCODE_LEFTBRACKET,	0x708 },
		{ SDL_SCANCODE_Z,			0x704 },
		{ SDL_SCANCODE_Y,			0x702 },
		{ SDL_SCANCODE_X,			0x701 },

		{ SDL_SCANCODE_W,			0x680 },
		{ SDL_SCANCODE_V,			0x640 },
		{ SDL_SCANCODE_U,			0x620 },
		{ SDL_SCANCODE_T,			0x610 },
		{ SDL_SCANCODE_S,			0x608 },
		{ SDL_SCANCODE_R,			0x604 },
		{ SDL_SCANCODE_Q,			0x602 },
		{ SDL_SCANCODE_P,			0x601 },

		{ SDL_SCANCODE_O,			0x580 },
		{ SDL_SCANCODE_N,			0x540 },
		{ SDL_SCANCODE_M,			0x520 },
		{ SDL_SCANCODE_L,			0x510 },
		{ SDL_SCANCODE_K,			0x508 },
		{ SDL_SCANCODE_J,			0x504 },
		{ SDL_SCANCODE_I,			0x502 },
		{ SDL_SCANCODE_H,			0x501 },

		{ SDL_SCANCODE_G,			0x480 },
		{ SDL_SCANCODE_F,			0x440 },
		{ SDL_SCANCODE_E,			0x420 },
		{ SDL_SCANCODE_D,			0x410 },
		{ SDL_SCANCODE_C,			0x408 },
		{ SDL_SCANCODE_B,			0x404 },
		{ SDL_SCANCODE_A,			0x402 },
		{ SDL_SCANCODE_MINUS,		0x401 }, // '@'

		{ SDL_SCANCODE_SLASH,		0x380 },
		{ SDL_SCANCODE_PERIOD,		0x340 },
		{ SDL_SCANCODE_EQUALS,		0x320 },
		{ SDL_SCANCODE_COMMA,		0x310 },
		{ SDL_SCANCODE_SEMICOLON,	0x308 },
		{ SDL_SCANCODE_APOSTROPHE,	0x304 },
		{ SDL_SCANCODE_9,			0x302 },
		{ SDL_SCANCODE_8,			0x301 },

		{ SDL_SCANCODE_7,			0x280 },
		{ SDL_SCANCODE_6,			0x240 },
		{ SDL_SCANCODE_5,			0x220 },
		{ SDL_SCANCODE_4,			0x210 },
		{ SDL_SCANCODE_3,			0x208 },
		{ SDL_SCANCODE_2,			0x204 },
		{ SDL_SCANCODE_1,			0x202 },
		{ SDL_SCANCODE_0,			0x201 },

		{ SDL_SCANCODE_F5,			0x180 },
		{ SDL_SCANCODE_F4,			0x140 },
		{ SDL_SCANCODE_F3,			0x120 },
		{ SDL_SCANCODE_F2,			0x110 },
		{ SDL_SCANCODE_F1,			0x108 },
		{ SDL_SCANCODE_ESCAPE,		0x104 }, // AR2
		{ SDL_SCANCODE_F8,			0x102 }, // STR
		{ SDL_SCANCODE_F7,			0x101 }, // LDA - left diagonal arrow

		{ SDL_SCANCODE_DOWN,		0x080 },
		{ SDL_SCANCODE_RIGHT,		0x040 },
		{ SDL_SCANCODE_UP,			0x020 },
		{ SDL_SCANCODE_LEFT,		0x010 },
		{ SDL_SCANCODE_BACKSPACE,	0x008 }, // ZB
		{ SDL_SCANCODE_RETURN,		0x004 }, // VK
		{ SDL_SCANCODE_RALT,		0x002 }, // PS
		{ SDL_SCANCODE_TAB,			0x001 }, // TAB
	};
}
