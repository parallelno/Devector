#pragma once
#ifndef DEV_DISPLAY_H
#define DEV_DISPLAY_H

#include "Memory.h"

namespace dev
{
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
		static constexpr int FRAME_W = 768;
		static constexpr int FRAME_H = 312;
	private:
		static constexpr int VSYNC = 22;
		static constexpr int VBLANK_TOP = 18;
		static constexpr int BORDER_TOP = VSYNC + VBLANK_TOP;
		static constexpr int BORDER_LEFT = 128;
		static constexpr int RES_W = 512;
		static constexpr int RES_H = 256;
		static constexpr int RASTERIZED_PXLS = 16;
		static constexpr int PALETTE_LEN = 16;

		bool m_mode;

		//GCHandle data_handle{ get; private set; }

	public:
		bool T50HZ; // interruption request
		static constexpr int FRAME_CC = FRAME_W * FRAME_H;

		//WriteableBitmap frame;
		uint32_t m_data[FRAME_W * FRAME_H];

		uint32_t m_palette[PALETTE_LEN];
		uint32_t m_fillColor;


		int m_rasterLine;	// currently rasterized scanline idx from the bottom
		int m_rasterPixel;	// currently rasterized scanline pixel

		const Memory& m_memory;

		Display(const Memory& _memory);

		void Init();

		// to draw a pxl
		void Rasterize();
	private:
		void Draw8PxlsActiveArea();
		void Draw8PxlsBorder();
	};
}
#endif // !DEV_DISPLAY_H