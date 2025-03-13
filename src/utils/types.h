#pragma once

#include <cstdint>
#include <bit>

namespace dev
{
	using GlobalAddr = uint32_t;
	using Addr = uint16_t;
	using ColorI = uint32_t;
	using Id = int;

	// The User initiated requests in the UI thread
	// for window-to-window communication
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
			RELOAD_ROM_FDD_REC,
			DISPLAY_FRAME_BUFF_UPDATE,
			LOAD_RECENT_FDD_IMG,

			LABEL_EDIT_WINDOW_ADD,
			LABEL_EDIT_WINDOW_EDIT,
			CONST_EDIT_WINDOW_ADD,
			CONST_EDIT_WINDOW_EDIT,
			COMMENT_EDIT_WINDOW_ADD,
			COMMENT_EDIT_WINDOW_EDIT,
			MEMORY_EDIT_EDIT_WINDOW_ADD,
			MEMORY_EDIT_EDIT_WINDOW_EDIT,
			CODE_PERFS_EDIT_WINDOW_ADD,
			CODE_PERFS_EDIT_WINDOW_EDIT,
		};

		Type type = Type::NONE;
		GlobalAddr globalAddr = 0;
		uint16_t len = 0;
	};

	enum class UIItemMouseAction { NONE = 0, HOVERED, LEFT, RIGHT, MIDDLE };
	enum class Condition : uint8_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, INVALID, COUNT };
	static constexpr int CONDITION_BIT_WIDTH = std::bit_width<uint8_t>(static_cast<uint8_t>(Condition::COUNT) - 1);
}