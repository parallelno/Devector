#include "Keyboard.h"
#include "Utils/Utils.h"
#include "GLFW/glfw3.h"

dev::Keyboard::Keyboard()
{
	memset(m_encodingMatrix, 0, sizeof(m_encodingMatrix));
	InitMapping();
}

// HW thread
auto dev::Keyboard::KeyHandling(int _key, int _action)
-> Operation
{
	int row, column;

	switch (_key)
	{
	case GLFW_KEY_F11:
		if (_action == GLFW_RELEASE) {
			// BLK + VVOD functionality
			return Operation::RESET;
		}
		break;
	case GLFW_KEY_F10: // TODO: F12 causes debug interruption. It is ImGui feature. Fix it.
		if (_action == GLFW_RELEASE) {
			// BLK + SBR functionality
			return Operation::RESTART;
		}
		break;
		// shift keys
	case GLFW_KEY_LEFT_SHIFT: [[fallthrough]];
	case GLFW_KEY_RIGHT_SHIFT:
		m_keySS = _action == GLFW_PRESS;
		break; // shift/ss
	case GLFW_KEY_LEFT_CONTROL:
		m_keyUS = _action == GLFW_PRESS;
		break; // ctrl/us
	case GLFW_KEY_LEFT_SUPER: [[fallthrough]];
	case GLFW_KEY_LEFT_ALT: [[fallthrough]];
	case GLFW_KEY_F6:
		m_keyRus = _action == GLFW_PRESS;
		break; // cmd/rus
		// matrix keys
	default:

		auto it = m_keymap.find(_key);
		if (it != m_keymap.end()) 
		{
			auto rowColumn = it->second;
			row = rowColumn >> 8;
			column = rowColumn & 0xFF;

			if (_action == GLFW_RELEASE) {
				m_encodingMatrix[row] &= ~column;
			}
			else {
				m_encodingMatrix[row] |= column;
			}
		}
		else {
			//dev::Log("unknown keycode: {}\n", _key);
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
	// ──--┼───────────────────────────────
	//   7 │SPC  ^   ]   \   [   Z   Y   X
	//   6 │ W   V   U   T   S   R   Q   P
	// r 5 │ O   N   M   L   K   J   I   H
	// o 4 │ G   F   E   D   C   B   A   @
	// w 3 │ /   .   =   ,   ;   :   9   8
	// s 2 │ 7   6   5   4   3   2   1   0
	//   1 │F5  F4  F3  F2  F1  AR2 STR LDA, // LDA - is a left diagonal arrow
	//   0 │DN  RT  UP  LFT ZAB VK  PS  TAB

	m_keymap = {
		// KeyCode				RowColumnCode = row<<8 | 1<<column
		{ GLFW_KEY_SPACE,		0x780 },
		{ GLFW_KEY_GRAVE_ACCENT,0x701 },
		{ GLFW_KEY_RIGHT_BRACKET,0x720 },
		{ GLFW_KEY_BACKSLASH,	0x710 },
		{ GLFW_KEY_LEFT_BRACKET,0x708 },
		{ GLFW_KEY_Z,			0x704 },
		{ GLFW_KEY_Y,			0x702 },
		{ GLFW_KEY_X,			0x701 },

		{ GLFW_KEY_W,			0x680 },
		{ GLFW_KEY_V,			0x640 },
		{ GLFW_KEY_U,			0x620 },
		{ GLFW_KEY_T,			0x610 },
		{ GLFW_KEY_S,			0x608 },
		{ GLFW_KEY_R,			0x604 },
		{ GLFW_KEY_Q,			0x602 },
		{ GLFW_KEY_P,			0x601 },

		{ GLFW_KEY_O,			0x580 },
		{ GLFW_KEY_N,			0x540 },
		{ GLFW_KEY_M,			0x520 },
		{ GLFW_KEY_L,			0x510 },
		{ GLFW_KEY_K,			0x508 },
		{ GLFW_KEY_J,			0x504 },
		{ GLFW_KEY_I,			0x502 },
		{ GLFW_KEY_H,			0x501 },

		{ GLFW_KEY_G,			0x480 },
		{ GLFW_KEY_F,			0x440 },
		{ GLFW_KEY_E,			0x420 },
		{ GLFW_KEY_D,			0x410 },
		{ GLFW_KEY_C,			0x408 },
		{ GLFW_KEY_B,			0x404 },
		{ GLFW_KEY_A,			0x402 },
		{ GLFW_KEY_MINUS,		0x401 }, // 189:-@

		{ GLFW_KEY_SLASH,		0x380 },
		{ GLFW_KEY_PERIOD,		0x340 },
		{ GLFW_KEY_EQUAL,		0x320 },
		{ GLFW_KEY_COMMA,		0x310 },
		{ GLFW_KEY_SEMICOLON,	0x308 },
		{ GLFW_KEY_APOSTROPHE,	0x304 },
		{ GLFW_KEY_9,			0x302 },
		{ GLFW_KEY_8,			0x301 },

		{ GLFW_KEY_7,			0x280 },
		{ GLFW_KEY_6,			0x240 },
		{ GLFW_KEY_5,			0x220 },
		{ GLFW_KEY_4,			0x210 },
		{ GLFW_KEY_3,			0x208 },
		{ GLFW_KEY_2,			0x204 },
		{ GLFW_KEY_1,			0x202 },
		{ GLFW_KEY_0,			0x201 },

		{ GLFW_KEY_F5,			0x180 },
		{ GLFW_KEY_F4,			0x140 },
		{ GLFW_KEY_F3,			0x120 },
		{ GLFW_KEY_F2,			0x110 },
		{ GLFW_KEY_F1,			0x108 },
		{ GLFW_KEY_ESCAPE,		0x104 },
		{ GLFW_KEY_F8,			0x102 }, // STR
		{ GLFW_KEY_F7,			0x101 }, // ^\ ?

		{ GLFW_KEY_DOWN,		0x080 },
		{ GLFW_KEY_RIGHT,		0x040 },
		{ GLFW_KEY_UP,			0x020 },
		{ GLFW_KEY_LEFT,		0x010 },
		{ GLFW_KEY_BACKSPACE,	0x008 },
		{ GLFW_KEY_ENTER,		0x004 },
		{ GLFW_KEY_RIGHT_ALT,	0x002 },
		{ GLFW_KEY_TAB,			0x001 },
	};
}
