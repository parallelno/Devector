#pragma once

#include <vector>
#include <array>
#include <chrono>
#include <mutex>

#include "Utils/Types.h"
#include "Core/Memory.h"
#include "Core/IO.h"

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

		// scanline has 768/384 pxls (MODE_512/MODE_256). A scanline rasterising time takes 192 cpu cycles (3 Mhz tick rate) or 768 quarters of a cpu cycle (12 Mhz tick rate).
		//		hblank (left): 128/64 pxls
		//		horizontal resolution : 512/256 pxls
		//		hblank (right): 128/64 pxls

		// For simplisity of the logic the diplay buffer horizontal resolution
		// is always 768 pxls to fit the 512 mode.
		// It rasters 4 horizontal pxls every cpu cycle no mater the mode.
		// In MODE_256 it dups every 2 horizontal pxls.

	public:

		static constexpr int FRAME_W = 768;					// a frame resolution including borders
		static constexpr int FRAME_H = 312;					// a frame resolution including borders
		static constexpr int FRAME_LEN = FRAME_W * FRAME_H;	// the size of a frame buffer
		
		static constexpr auto VSYC_DELAY = 19968us; // For the realtime emulation it should be called every 0.019968 sec by 3000000/59904 Mz timer

		static constexpr int SCAN_VSYNC = 22;
		static constexpr int SCAN_VBLANK_TOP = 18;
		static constexpr int SCAN_VBLANK_BOTTOM = 16;
		static constexpr int SCAN_ACTIVE_AREA_TOP = SCAN_VSYNC + SCAN_VBLANK_TOP;
		static constexpr int ACTIVE_AREA_W = 512;			// horizontal screen resolution in MODE_512
		static constexpr int ACTIVE_AREA_H = 256;			// vertical screen resolution
		static constexpr int BORDER_LEFT = 137;				// left border in pxls		
		static constexpr int BORDER_RIGHT = BORDER_LEFT + ACTIVE_AREA_W;
		static constexpr int BORDER_VISIBLE = 16; // border visible on the screen in pxls in 256 mode
		static constexpr int RASTERIZED_PXLS_MAX = 16;	// the amount of rasterized pxls every 4 cpu cycles in MODE_512
		static constexpr int COLORS_POLUTED_DELAY = 4; // this timer in pixels. if the palette is set inside the active area, the fourth and the fifth pixels get corrupted colors

		using FrameBuffer = std::array <ColorI, FRAME_LEN>;

	private:
		bool m_irq; // interruption request
		uint8_t m_scrollIdx; // vertical scrolling, 255 - no scroll

		FrameBuffer m_frameBuffer;	// rasterizer draws here
		FrameBuffer m_backBuffer;	// a buffer to simulate VSYNC
		FrameBuffer m_gpuBuffer;	// temp buffer for output to GPU
		std::mutex m_backBufferMutex;
		int m_framebufferIdx;	// currently rendered pixel idx to m_frameBuffer

		uint64_t m_frameNum; // counts frames

		Memory& m_memory;
		IO& m_io;

	public:
		Display(Memory& _memory, IO& _io);
		void Init();
		void Rasterize();
		bool IsIRQ();
		auto GetFrame(const bool _vsync) ->const FrameBuffer*;
		inline auto GetFrameNum() const -> uint64_t { return m_frameNum; };
		inline int GetRasterLine() const;
		inline int GetRasterPixel() const;
		static ColorI VectorColorToArgb(const uint8_t _vColor);
		auto GetScrollVert() const-> uint8_t { return m_scrollIdx; };
	private:
		uint32_t BytesToColorIdxs();
		uint32_t GetScreenBytes();
		uint32_t BytesToColorIdx(uint32_t _screenBytes, uint8_t _bitIdx);
		void RasterizeActiveArea(const int _rasterizedPixels);
		void FillActiveArea256(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillActiveArea512(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillActiveArea256PortHandling(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillActiveArea512PortHandling(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void RasterizeBorder(const int _rasterizedPixels);
		void FillBorder(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
		void FillBorderPortHandling(const int _rasterizedPixels = RASTERIZED_PXLS_MAX);
	};
}