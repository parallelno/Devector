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
		static constexpr int PORT_OUT_BORDER_COLOR = 0x0c;
		static constexpr int PORT_NO_COMMIT = -1; // no-data to process
		static constexpr int PORT_COMMIT_TIME = 0; 
		
		// determines when the OUT command sends data into the port
		// this timing is based on the 12 MHz clock (equivalent to the number of pixels in 512 mode)
		// it's calculated from the start of the third machine cycle (4 cpu cycles each)
		static constexpr int OUT_COMMIT_TIME = 3 * 4;
		// determines when the color sent from the port is stored in the palette memory
		// this timing is based on the 12 MHz clock (equivalent to the number of pixels in 512 mode)
		// it's calculated from the start of the third machine cycle (4 cpu cycles each)
		static constexpr int PALETTE_COMMIT_TIME = 19 * 4;
		static constexpr int SCROLL_COMMIT_PXL = 128; // when the OUTed scroll data is handled. time's counted by 12 MHz clock (equals amount of pixels in 512 mode)
		static constexpr int IRQ_COMMIT_PXL = 176; // when apply the OUTed scroll data is handled. time's counted by 12 MHz clock (equals amount of pixels in 512 mode)
		static constexpr int PALETTE_LEN = 16;

		// TODO: make it working
		std::function<void(bool)> onruslat;

		using VectorColorToArgbFunc = std::function<ColorI(const uint8_t)>;

		static constexpr bool DISPLAY_MODE_256 = false;
		static constexpr bool MODE_512 = true;

	private:
		using Palette = std::array <ColorI, PALETTE_LEN>;
		Palette m_palette;
		uint8_t m_borderColorIdx;
		bool m_displayMode; // 256 or 512 modes
		int m_outCommitTimer; // in pixels (12Mhz clock)
		int m_paletteCommitTimer; // in pixels (12Mhz clock)

		uint8_t CW, m_portA, m_portB, m_portC;
		uint8_t CW2, m_portA2, m_portB2, m_portC2;

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
		void Init();
		auto PortIn(uint8_t _port) -> uint8_t;
		void PortOut(uint8_t _port, uint8_t _value);
		void PortOutCommit();
		void PaletteCommit(const int _index);
		inline auto GetKeyboard() -> Keyboard& { return m_keyboard; };
		inline auto GetColor(const size_t _colorIdx) const -> ColorI { return m_palette[_colorIdx]; };
		inline auto GetBorderColor() const -> ColorI { return m_palette[m_borderColorIdx]; };
		inline auto GetBorderColorIdx() const -> uint8_t { return m_borderColorIdx; };
		inline auto GetScroll() const -> uint8_t { 
			return m_portA; 
		};
		inline auto GetDisplayMode() const -> bool { return m_displayMode; };
		inline auto GetOutCommitTime() const -> int { return m_outCommitTimer; };
		inline auto GetPaletteCommitTime() const -> int { return m_paletteCommitTimer; };
		void CommitTimersHandling(const uint8_t _colorIdx);
	};
}
#endif // !DEV_IO_H