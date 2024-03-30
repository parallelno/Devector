#pragma once
#ifndef DEV_DISPLAY_H
#define DEV_DISPLAY_H

#include <vector>
#include <array>
#include <chrono>
#include <mutex>

#include "Types.h"
#include "Memory.h"

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

		static constexpr bool MODE_256 = false;
		static constexpr bool MODE_512 = true;
	public:
		static constexpr int FRAME_W = 768;					// a frame resolution including borders
		static constexpr int FRAME_H = 312;					// a frame resolution including borders
		static constexpr int FRAME_LEN = FRAME_W * FRAME_H;	// the size of a frame buffer

		static constexpr auto VSYC_DELAY = 19968us; // For the realtime emulation it should be called every 0.019968 sec by 3000000/59904 Mz timer

		using FrameBuffer = std::array <ColorI, FRAME_LEN>;

	private:
		static constexpr int SCAN_VSYNC = 22;
		static constexpr int SCAN_VBLANK_TOP = 18;
		static constexpr int SCAN_BORDER_TOP = SCAN_VSYNC + SCAN_VBLANK_TOP;
		static constexpr int BORDER_LEFT = 128;				// left border in pxls
		static constexpr int BORDER_RIGHT = BORDER_LEFT;	// right border in pxls
		static constexpr int RES_W = 512;			// horizontal screen resolution in MODE_512
		static constexpr int RES_H = 256;			// vertical screen resolution
		static constexpr int RASTERIZED_PXLS = 16;	// the amount of rasterized pxls every 4 cpu cycles in MODE_512

		bool m_mode;
		bool m_t50Hz; // interruption request

		FrameBuffer m_frameBuffer;	// rasterizer draws here
		FrameBuffer m_backBuffer;	// a buffer to simulate VSYNC
		FrameBuffer m_gpuBuffer;	// temp buffer for output to GPU
		std::mutex m_backBufferMutex;

		ColorI m_fillColor;

		int m_rasterLine;	// currently rasterized scanline idx from the bottom
		int m_rasterPixel;	// currently rasterized scanline pixel

		uint64_t m_frameNum;

		Memory& m_memory;

	public:
		Display(Memory& _memory);
		void Init();
		void Rasterize();	// to draw a pxl
		bool IsInt50Hz();
		auto GetFrame(const bool _vsync) ->const FrameBuffer*;
		auto GetFrameNum() const -> uint64_t { return m_frameNum; };
		int GetRasterLine() const { return m_rasterLine; };
		int GetRasterPixel() const { return m_rasterPixel; };
	private:
		void Draw8PxlsActiveArea();
		void Draw8PxlsBorder();
	};
}
#endif // !DEV_DISPLAY_H