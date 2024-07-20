#pragma once

#include <cstdint>
#include "Utils/Types.h"

namespace dev
{
	using ErrCode = int;

	using GlobalAddr = uint32_t;
	using Addr = uint16_t;
	using ColorI = uint32_t;

	// TODO: replace it with GLuint
	using GLuint1 = unsigned int;
	using GLint1 = int;
	using GLenum1 = unsigned int;
	using GLsizei1 = int;

	using Id = int;

	// the request initiated by the user
	struct ReqUI {
		enum class Type : int {
			NONE = 0,
			DISASM_UPDATE, // redraw disasm
			DISASM_UPDATE_ADDR,
			DISASM_NAVIGATE_TO_ADDR,
			DISASM_NAVIAGATE_NEXT,
			DISASM_NAVIAGATE_PREV,
			HEX_HIGHLIGHT_ON,
			HEX_HIGHLIGHT_OFF,
			RELOAD_ROM_FDD,
			PLAYBACK_STEP_REVERSE,
			PLAYBACK_STEP_FORWARD,
		};

		Type type = Type::NONE;
		GlobalAddr globalAddr = 0;
		GlobalAddr len = 0;
	};

	enum class UIItemMouseAction { NONE = 0, HOVERED, LEFT, RIGHT, MIDDLE };
	enum class Condition : uint8_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, INVALID };
}