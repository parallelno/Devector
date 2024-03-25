#pragma once
#ifndef DEV_IO_H
#define DEV_IO_H

#include <cstdint>
#include <array>

#include "Utils/JsonUtils.h"
#include "Types.h"
#include "Keyboard.h"

namespace dev
{
	class IO
	{
	public:
		static constexpr int PALETTE_LEN = 16;
	private:
		using Palette = std::array <ColorI, PALETTE_LEN>;
		Palette m_palette;

		uint8_t CW, PA, PB, PC, PIA1_last;
		uint8_t CW2, PA2, PB2, PC2;

		int outport;
		int outbyte;
		int palettebyte;

		uint8_t joy_0e, joy_0f;

		Keyboard& m_keyboard;

	public:
		IO(Keyboard& _keyboard);
		auto PortIn(uint8_t _port) -> uint8_t;
		void PortOut(uint8_t _port, uint8_t _value);
		auto GetKeyboard() -> Keyboard&;
	};
}
#endif // !DEV_IO_H