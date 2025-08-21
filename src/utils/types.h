#pragma once

#include <cstdint>
#include <bit>

namespace dev
{
	using GlobalAddr = uint32_t;
	using Addr = uint16_t;
	using ColorI = uint32_t;
	using Id = int;

	enum class UIItemMouseAction { NONE = 0, HOVERED, LEFT, RIGHT, MIDDLE };
	enum class Condition : uint8_t { ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, INVALID, COUNT };
	static constexpr int CONDITION_BIT_WIDTH = std::bit_width<uint8_t>(static_cast<uint8_t>(Condition::COUNT) - 1);
}