#pragma once
#ifndef DEV_IO_H
#define DEV_IO_H

#include <cstdint>
#include <array>

#include "Utils/JsonUtils.h"
#include "Types.h"
#include "Keyboard.h"
#include "Memory.h"

namespace dev
{
	class IO
	{
	public:
		static constexpr int PALETTE_LEN = 16;

		std::function<void(int)> onborderchange;
		std::function<void(bool)> onmodechange;
		std::function<void(bool)> onruslat;

	private:
		using Palette = std::array <ColorI, PALETTE_LEN>;
		Palette m_palette;

		uint8_t CW, m_portA, m_portB, m_portC, PIA1_last;
		uint8_t CW2, PA2, PB2, PC2;

		int outport;
		int outbyte;
		int palettebyte;

		uint8_t joy_0e, joy_0f;

		Keyboard& m_keyboard;
		Memory& m_memory;

	public:
		IO(Keyboard& _keyboard, Memory& _memory);
		auto PortIn(uint8_t _port) -> uint8_t;
		void PortOut(uint8_t _port, uint8_t _value);
		void PortOutHandling(uint8_t _port, uint8_t _value);
		auto GetKeyboard() -> Keyboard&;
	};
}
#endif // !DEV_IO_H