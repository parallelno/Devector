#pragma once

#include <cstdint>
#include <array>

#include "utils/json_utils.h"
#include "utils/types.h"
#include "core/keyboard.h"
#include "core/memory.h"
#include "core/timer_i8253.h"
#include "core/sound_ay8910.h"
#include "core/fdc_wd1793.h"

namespace dev
{
	class IO
	{
		// determines when the OUT command sends data into the port
		// this timing is based on the 12 MHz clock (equivalent to the number of pixels in 512 mode)
		// it's calculated from the start of the third machine cycle (4 cpu cycles each)
		static constexpr int OUT_COMMIT_TIME = 1;
		// determines when the color sent from the port is stored in the palette memory
		// this timing is based on the 12 MHz clock (equivalent to the number of pixels in 512 mode)
		// it's calculated from the start of the third machine cycle (4 cpu cycles each)
		static constexpr int PALETTE_COMMIT_TIME = 5;
		static constexpr int DISPLAY_MODE_COMMIT_TIME = 2 * 4;

	public:
		static constexpr uint8_t PORT_OUT_BORDER_COLOR0 = 0x0C;
		static constexpr uint8_t PORT_OUT_BORDER_COLOR1 = 0x0D;
		static constexpr uint8_t PORT_OUT_BORDER_COLOR2 = 0x0E;
		static constexpr uint8_t PORT_OUT_BORDER_COLOR3 = 0x0F;
		static constexpr uint8_t PORT_OUT_DISPLAY_MODE = 0x02;
		static constexpr int PALETTE_LEN = 16;
		static constexpr bool MODE_256 = false;
		static constexpr bool MODE_512 = true;

#pragma pack(push, 1)
		union Palette
		{
			struct {
				uint64_t low	: 64;
				uint64_t hi		: 64;
			};
			uint8_t bytes[PALETTE_LEN];
		};
#pragma pack(pop)

#pragma pack(push, 1)
		union Ports
		{
			struct {
				uint8_t CW		: 8;
				uint8_t portA	: 8;
				uint8_t portB	: 8;
				uint8_t portC	: 8;
				uint8_t CW2		: 8;
				uint8_t portA2	: 8;
				uint8_t portB2	: 8;
				uint8_t portC2	: 8;
			};
			uint64_t data;
		};
#pragma pack(pop)

#pragma pack(push, 1)
		struct State
		{
			Palette palette;
			Ports ports;
			uint32_t ruslatHistory = 0;
			uint8_t outport		: 8;
			uint8_t outbyte		: 8;


			int8_t outCommitTimer		: 8; // in pixels (12Mhz clock)
			int8_t paletteCommitTimer	: 8; // in pixels (12Mhz clock)
			int8_t displayModeTimer		: 8; // in pixels (12Mhz clock)

			uint8_t joy0 : 8;
			uint8_t joy1 : 8;

			uint8_t hwColor		: 8; // a tmp store for a output color before it commited to the HW. Vector06C color format : uint8_t BBGGGRRR
			uint8_t reqDisplayMode : 2; // a tmp store for a display mode before it commited to HW
			uint8_t brdColorIdx : 4; // border color idx
			bool displayMode	: 1; // false - 256 mode, true - 512 mode
			uint8_t ruslat		: 1;
		};
#pragma pack(pop)

		// debug only info
		union PortsData {
			struct {
				uint64_t data0;
				uint64_t data1;
				uint64_t data2;
				uint64_t data3;
				uint64_t data4;
				uint64_t data5;
				uint64_t data6;
				uint64_t data7;
			};
			uint8_t data[256];
			PortsData(
				uint64_t _data0, uint64_t _data1, uint64_t _data2, uint64_t _data3,
				uint64_t _data4, uint64_t _data5, uint64_t _data6, uint64_t _data7)
				:
				data0(_data0), data1(_data1), data2(_data2), data3(_data3),
				data4(_data4), data5(_data5), data6(_data6), data7(_data7) {}
			PortsData() :
				data0(0), data1(0), data2(0), data3(0),
				data4(0), data5(0), data6(0), data7(0) {};
		};

	private:
		State m_state;
		
		PortsData m_portsInData;
		PortsData m_portsOutData;

		Keyboard& m_keyboard;
		Memory& m_memory;
		TimerI8253& m_timer;
		SoundAY8910& m_ay;
		Fdc1793& m_fdc;

		int m_outCommitTime = OUT_COMMIT_TIME;
		int m_paletteCommitTime = PALETTE_COMMIT_TIME;
		int m_displayModeTime = DISPLAY_MODE_COMMIT_TIME;

		void PortOutHandling(uint8_t _port, uint8_t _value);
		auto PortInHandling(uint8_t _port) -> uint8_t;

	public:
		IO(Keyboard& _keyboard, Memory& _memory, TimerI8253& _timer, SoundAY8910& _ay, Fdc1793& _fdc);
		void Init();
		auto PortIn(uint8_t _port) -> uint8_t;
		void PortOut(uint8_t _port, uint8_t _value);
		void PortOutCommit();
		inline auto GetKeyboard() -> Keyboard& { return m_keyboard; };
		inline auto GetColor(const uint8_t _colorIdx) const -> uint8_t { return m_state.palette.bytes[_colorIdx]; };
		inline void SetColor(const uint8_t _idx) { m_state.palette.bytes[_idx] = m_state.hwColor; };
		inline auto GetBorderColor() const -> uint8_t { return m_state.palette.bytes[m_state.brdColorIdx]; };
		inline auto GetBorderColorIdx() const -> uint8_t { return m_state.brdColorIdx; };
		inline auto GetScroll() const -> uint8_t { return m_state.ports.portA; };
		inline auto GetRusLat() const -> uint32_t { return m_state.ruslat; }
		inline auto GetRusLatHistory() const -> uint32_t { return m_state.ruslatHistory; }
		inline auto GetDisplayMode() const -> bool { return m_state.displayMode; };
		inline auto GetOutCommitTimer() const -> int { return m_state.outCommitTimer; };
		inline auto GetPaletteCommitTimer() const -> int { return m_state.paletteCommitTimer; };
		inline auto GetPaletteCommitTime() const -> int { return m_paletteCommitTime; };
		inline void SetPaletteCommitTime(const int _paletteCommitTime) { m_paletteCommitTime = _paletteCommitTime; };

		auto GetState() const -> const State& { return m_state; };
		auto GetStateP() -> State* { return &m_state; };
		auto GetPalette() const -> const Palette* { return &m_state.palette; }
		auto GetPorts() const -> const Ports* { return &m_state.ports; }
		auto GetPortsInData() const -> const PortsData* { return &m_portsInData; }
		auto GetPortsOutData() const -> const PortsData* { return &m_portsOutData; }
		auto GetBeeper() const -> uint8_t { return m_state.ports.portC & 1; } // it also out to the tape
		void TryToCommit(const uint8_t _colorIdx);
	};
}