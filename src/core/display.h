#pragma once

#include <vector>
#include <array>
#include <chrono>
#include <mutex>

#include "utils/types.h"
#include "core/memory.h"
#include "core/io.h"

namespace dev
{
	using namespace std::chrono_literals;

	class Display
	{
	private:
		// phisical frame config:
		// 312 scanlines in a frame:
		//		vsync: 22 lines
		//		vblank (top): 18 lines
		//		vertical resolution: 256 lines
		//      vblank (bottom): 16 lines

		// scanline has 768/384 pxls (MODE_512/MODE_256).
		// A scanline rasterising time takes 192 cpu cycles (3 Mhz tick rate) or
		// 768 quarters of a cpu cycle (12 Mhz tick rate).
		//		hblank (left): 128/64 pxls
		//		horizontal resolution : 512/256 pxls
		//		hblank (right): 128/64 pxls

		// For simplisity of the logic the diplay buffer horizontal resolution
		// is always 768 pxls to fit the 512 mode.
		// It rasters 4 horizontal pxls every cpu cycle no mater the mode.
		// In MODE_256 it dups every 2 horizontal pxls.

	public:

		static constexpr int FRAME_W = 768;	// a frame resolution including borders
		static constexpr int FRAME_H = 312; // a frame resolution including borders
		// the size of a frame buffer
		static constexpr int FRAME_LEN = FRAME_W * FRAME_H;

		// For the realtime emulation it should be called
		//  every 0.019968 seconds by 3000000/59904 Mz timer
		static constexpr auto VSYC_DELAY = 19968us;

		static constexpr auto FRAMES_PER_SECOND = 50;

		static constexpr int SCAN_VSYNC = 24;
		static constexpr int SCAN_VBLANK_TOP = 16;
		static constexpr int SCAN_VBLANK_BOTTOM = 16;
		static constexpr int SCAN_ACTIVE_AREA_TOP = SCAN_VSYNC + SCAN_VBLANK_TOP;
		// horizontal screen resolution in MODE_512
		static constexpr int ACTIVE_AREA_W = 512;
		// vertical screen resolution
		static constexpr int ACTIVE_AREA_H = 256;
		// horizontal screen resolution in MODE_512
		static constexpr int BORDER_LEFT = 128;
		// horizontal screen resolution in MODE_512
		static constexpr int BORDER_RIGHT = BORDER_LEFT;
		static constexpr int BORDER_TOP = SCAN_ACTIVE_AREA_TOP;
		static constexpr int BORDER_BOTTOM = SCAN_VBLANK_BOTTOM;
		// border visible on the screen in pxls in 256 mode
		static constexpr int BORDER_VISIBLE = 16;
		// the amount of rasterized pxls every 4 cpu cycles in MODE_512
		static constexpr int RASTERIZED_PXLS_MAX = 16;
		// this timer in pixels.
		// if the palette is set inside the active area,
		// the fourth and the fifth pixels get corrupted colors
		static constexpr int COLORS_POLUTED_DELAY = 4;

		// interrupt request.
		// time's counted by 12 MHz clock (equals amount of pixels in 512 mode)
		static constexpr int IRQ_COMMIT_PXL = 112;
		static constexpr int SCROLL_COMMIT_PXL = BORDER_LEFT + 3;
		// vertical scrolling, 0xff - no scroll
		static constexpr int SCROLL_DEFAULT = 0xff;

		static constexpr int FULL_PALETTE_LEN = 256;

		using FrameBuffer = std::array <ColorI, FRAME_LEN>;

		enum class BufferType { FRAME_BUFFER, BACK_BUFFER, GPU_BUFFER};
		using BuffUpdateFunc = std::function<void(const BufferType _bufferType)>;

		// contains the state after the last instruction executed
#pragma pack(push, 1)
		struct Update
		{
			uint64_t frameNum = 0;	// counts frames
			bool irq = false;			// interruption request
			int framebufferIdx = 0;		// currently rendered pixel idx to m_frameBuffer
			uint8_t scrollIdx = SCROLL_DEFAULT;	// vertical scrolling
		};
#pragma pack(pop)

		struct State
		{
			Update update;
			FrameBuffer* frameBufferP = nullptr;
			BuffUpdateFunc BuffUpdate = nullptr;
		};

	private:
		Memory& m_memory;
		IO& m_io;

		State m_state;

		FrameBuffer m_frameBuffer;	// rasterizer draws here
		FrameBuffer m_backBuffer;	// a buffer to simulate VSYNC
		FrameBuffer m_gpuBuffer;	// temp buffer for loading on GPU
		std::mutex m_backBufferMutex;

		int m_borderLeft = BORDER_LEFT;
		int m_irqCommitPxl = IRQ_COMMIT_PXL;

		// prebaked look-up vector_color->RGBA palette
		ColorI m_fullPalette[FULL_PALETTE_LEN];

	public:
		Display(Memory& _memory, IO& _io);
		void Init();
		void Rasterize();
		bool IsIRQ();
		auto GetFrame(const bool _vsync) ->const FrameBuffer*;
		inline auto GetFrameNum() const -> uint64_t { return m_state.update.frameNum; };
		inline int GetRasterLine() const { return m_state.update.framebufferIdx / FRAME_W; };
		inline int GetRasterPixel() const { return m_state.update.framebufferIdx % FRAME_W; };
		inline int GetFramebufferIdx() const { return m_state.update.framebufferIdx; };
		static ColorI VectorColorToArgb(const uint8_t _vColor);
		auto GetScrollVert() const -> uint8_t { return m_state.update.scrollIdx; };
		auto GetState() const -> const State& { return m_state; };
		auto GetStateP() -> State* { return &m_state; };
		auto GetBorderLeft() const -> int { return m_borderLeft; };
		void SetBorderLeft(const int _borderLeft) { m_borderLeft = _borderLeft; };
		auto GetIrqCommitPxl() const -> int { return m_irqCommitPxl; };
		void SetIrqCommitPxl(const int _irqCommitPxl) { m_irqCommitPxl = _irqCommitPxl; };

	private:
		uint32_t BytesToColorIdxs();
		uint32_t GetScreenBytes(int _rasterLine, int _rasterPixel);
		uint32_t BytesToColorIdx256(uint32_t _screenBytes, uint8_t _bitIdx);
		uint32_t BytesToColorIdx512(uint32_t _screenBytes, uint8_t _bitIdx);
		void RasterizeActiveArea(const int _rasterizedPixels);
		void FillActiveArea256(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillActiveArea512(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillActiveArea256PortHandling(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillActiveArea512PortHandling(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void RasterizeBorder(const int _rasterizedPixels);
		void FillBorder(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillBorderPortHandling(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void BuffUpdate(BufferType _buffer);
		void FrameBuffUpdate();
	};
}