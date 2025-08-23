#pragma once

#include <cstdint>

/*
* Signals
* A helper type for the Scheduler class.
* Contains the list of signals for the UI thread to windows, and the windows to
* window communication.
*/

namespace dev
{
	enum Signals : uint64_t {
		NONE 				= 0,
		HW_RESET 			= 1 << 0,
		HW_RUNNING 			= 1 << 1, // cpu is ticking
		START				= 1 << 2, // hardware starts after break
		BREAKPOINTS			= 1 << 3,
		WATCHPOINTS			= 1 << 4,
		UI_DRAW				= 1 << 5,
		BREAK				= 1 << 6,
		FRAME				= 1 << 7, // new frame started
		DISASM_UPDATE		= 1 << 8,
		HEX_HIGHLIGHT_ON	= 1 << 9,
		HEX_HIGHLIGHT_OFF	= 1 << 10,
		RELOAD				= 1 << 11, // reload the current rom, fdd or rec
		LOAD_RECENT_FDD_IMG	= 1 << 12,

		LABEL_EDIT_WINDOW_ADD		= 1 << 13,
		LABEL_EDIT_WINDOW_EDIT		= 1 << 14,
		CONST_EDIT_WINDOW_ADD		= 1 << 15,
		CONST_EDIT_WINDOW_EDIT		= 1 << 16,
		COMMENT_EDIT_WINDOW_ADD		= 1 << 17,
		COMMENT_EDIT_WINDOW_EDIT	= 1 << 18,
		MEMORY_EDIT_WINDOW_ADD		= 1 << 19,
		MEMORY_EDIT_WINDOW_EDIT		= 1 << 20,
		CODE_PERF_EDIT_WINDOW_ADD	= 1 << 21,
		CODE_PERF_EDIT_WINDOW_EDIT	= 1 << 22,
		SCRIPT_EDIT_WINDOW_ADD		= 1 << 23,
		SCRIPT_EDIT_WINDOW_EDIT		= 1 << 24,

		TRACE_LOG_POPUP_OPEN		= 1 << 25,
		BREAKPOINTS_POPUP_ADD		= 1 << 26,
		BREAKPOINTS_POPUP_EDIT		= 1 << 27,
		WATCHPOINTS_POPUP_ADD		= 1 << 28,
		WATCHPOINTS_POPUP_EDIT		= 1 << 29,
	};

	// Conversion operator (as free function)
	inline uint32_t operator+(Signals sig) noexcept {
		return static_cast<uint32_t>(sig);
	}

	// Bitwise OR operator
	inline Signals operator|(Signals lhs, Signals rhs) noexcept {
		return static_cast<Signals>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
	}

	// Additional useful operators for bitwise operations
	inline Signals operator&(Signals lhs, Signals rhs) noexcept {
		return static_cast<Signals>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
	}

	inline Signals operator^(Signals lhs, Signals rhs) noexcept {
		return static_cast<Signals>(static_cast<uint32_t>(lhs) ^ static_cast<uint32_t>(rhs));
	}

	inline Signals operator~(Signals sig) noexcept {
		return static_cast<Signals>(~static_cast<uint32_t>(sig));
	}

	// Compound assignment operators
	inline Signals& operator|=(Signals& lhs, Signals rhs) noexcept {
		lhs = lhs | rhs;
		return lhs;
	}

	inline Signals& operator&=(Signals& lhs, Signals rhs) noexcept {
		lhs = lhs & rhs;
		return lhs;
	}

	inline Signals& operator^=(Signals& lhs, Signals rhs) noexcept {
		lhs = lhs ^ rhs;
		return lhs;
	}
}