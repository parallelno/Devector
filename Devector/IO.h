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
		static constexpr int PORT_NO_COMMIT = -1; // no-data to process
		
		// determines when the OUT command sends data into the port
		// this timing is based on the 12 MHz clock (equivalent to the number of pixels in 512 mode)
		// it's calculated from the start of the third machine cycle (4 cpu cycles each)
		static constexpr int OUT_COMMIT_TIME = 3 * 4;
		// determines when the color sent from the port is stored in the palette memory
		// this timing is based on the 12 MHz clock (equivalent to the number of pixels in 512 mode)
		// it's calculated from the start of the third machine cycle (4 cpu cycles each)
		static constexpr int PALETTE_COMMIT_TIME = 19 * 4;
		static constexpr int SCROLL_COMMIT_PXL = 150; // when the OUTed scroll data is handled. time's counted by 12 MHz clock (equals amount of pixels in 512 mode)
		static constexpr int IRQ_COMMIT_PXL = 176; // when apply the OUTed scroll data is handled. time's counted by 12 MHz clock (equals amount of pixels in 512 mode)
		static constexpr int PALETTE_LEN = 16;

		std::function<void(int)> onborderchange;
		std::function<void(bool)> onmodechange;
		std::function<void(bool)> onruslat;

		using VectorColorToArgbFunc = std::function<ColorI(const uint8_t)>;

	private:
		using Palette = std::array <ColorI, PALETTE_LEN>;
		Palette m_palette;
		uint8_t m_borderColorIdx;

		uint8_t CW, m_portA, m_portB, m_portC, PIA1_last;
		uint8_t CW2, PA2, PB2, PC2;

		int outport;
		int outbyte;
		int palettebyte;

		uint8_t joy_0e, joy_0f;

		Keyboard& m_keyboard;
		Memory& m_memory;

		VectorColorToArgbFunc VectorColorToArgb;

		void PortOutHandling(uint8_t _port, uint8_t _value);

	public:
		IO(Keyboard& _keyboard, Memory& _memory, VectorColorToArgbFunc _vecToArgbFunc);
		auto PortIn(uint8_t _port) -> uint8_t;
		void PortOut(uint8_t _port, uint8_t _value);
		void PortOutCommit();
		void PaletteCommit(const int _index);
		auto GetKeyboard() -> Keyboard&;
		auto GetColor(const size_t _colorIdx) const -> ColorI;
		auto GetBorderColor() const -> ColorI;
		auto GetBorderColorIdx() const -> uint8_t;
		auto GetScroll() const -> uint8_t;
	};
}
#endif // !DEV_IO_H