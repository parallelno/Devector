#include "core/display.h"
#include "utils/utils.h"

#define BORDER_RIGHT	( m_borderLeft + ACTIVE_AREA_W )

dev::Display::Display(Memory& _memory, IO& _io)
	:
	m_memory(_memory), m_io(_io)
{
	// init the full palette
	for (int i = 0; i < FULL_PALETTE_LEN; i++) {
		m_fullPalette[i] = VectorColorToArgb(i);
	}
	m_state.frameBufferP = &m_frameBuffer;

	m_state.BuffUpdate = std::bind(&Display::BuffUpdate, this, std::placeholders::_1);

	Init();
}

void dev::Display::Init()
{
	m_state.update.framebufferIdx = 0;
	m_frameBuffer.fill(0xff000000);
}

void dev::Display::RasterizeActiveArea(const int _rasterizedPixels)
{
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	auto commitTime = m_io.GetOutCommitTimer() > 0 || m_io.GetPaletteCommitTimer() > 0;

	bool scrollTime = rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel < m_borderLeft + RASTERIZED_PXLS_MAX;
	if (commitTime || scrollTime)
	{
		if (m_io.GetDisplayMode() == IO::MODE_256) FillActiveArea256PortHandling(_rasterizedPixels);
		else FillActiveArea512PortHandling(_rasterizedPixels);
	}
	else {
		if (m_io.GetDisplayMode() == IO::MODE_256) FillActiveArea256(_rasterizedPixels);
		else FillActiveArea512(_rasterizedPixels);
	}
}

void dev::Display::RasterizeBorder(const int _rasterizedPixels)
{
	int rasterLine = GetRasterLine();
	auto commitTime = m_io.GetOutCommitTimer() >= 0 || m_io.GetPaletteCommitTimer() >= 0;

	if (commitTime || rasterLine == 0 || rasterLine == 311)
	{
		FillBorderPortHandling(_rasterizedPixels);
	}
	else {
		FillBorder(_rasterizedPixels);
	}
}

// renders 16 pixels (in the 512 mode) from left to right
void dev::Display::Rasterize()
{
	// reset the interrupt request. it can be set during border drawing.
	m_state.update.irq = false;

	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();

	bool isActiveScan = rasterLine >= SCAN_ACTIVE_AREA_TOP && rasterLine < SCAN_ACTIVE_AREA_TOP + ACTIVE_AREA_H;
	bool isActiveArea = isActiveScan &&
					rasterPixel >= m_borderLeft && rasterPixel < BORDER_RIGHT;

	// Rasterize the Active Area
	if (isActiveArea)
	{
		int rasterizedPixels = dev::Min(BORDER_RIGHT - rasterPixel, RASTERIZED_PXLS_MAX);
		RasterizeActiveArea(rasterizedPixels);
		// Rasterize the border if there is a leftover
		if (rasterizedPixels < RASTERIZED_PXLS_MAX)
		{
			rasterizedPixels = RASTERIZED_PXLS_MAX - rasterizedPixels;
			RasterizeBorder(rasterizedPixels);
		}
	}
	// Rasterize the Border
	else {
		int rasterizedPixels = !isActiveScan || rasterPixel >= BORDER_RIGHT ? RASTERIZED_PXLS_MAX :
						dev::Min(m_borderLeft - rasterPixel, RASTERIZED_PXLS_MAX);

		RasterizeBorder(rasterizedPixels);

		// Rasterize the Active Area if there is a leftover
		if (rasterizedPixels < RASTERIZED_PXLS_MAX)
		{
			rasterizedPixels = RASTERIZED_PXLS_MAX - rasterizedPixels;
			RasterizeActiveArea(rasterizedPixels);
		}
	}
}

void dev::Display::FillBorder(const int _rasterizedPixels)
{
	auto borderColor = m_fullPalette[m_io.GetBorderColor()];
	for (int i = 0; i < _rasterizedPixels; i++)
	{
		m_frameBuffer[m_state.update.framebufferIdx++] = borderColor;
	}
}

void dev::Display::FillBorderPortHandling(const int _rasterizedPixels)
{
	for (int i = 0; i < _rasterizedPixels; i++)
	{
		m_io.TryToCommit(m_io.GetBorderColorIdx());
		auto color = m_fullPalette[m_io.GetBorderColor()];

		m_frameBuffer[m_state.update.framebufferIdx++] = color;
		int isNewFrame = m_state.update.framebufferIdx / FRAME_LEN;
		m_state.update.framebufferIdx %= FRAME_LEN;

		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		m_state.update.irq |= m_state.update.framebufferIdx == m_irqCommitPxl;

		if (isNewFrame)
		{
			m_state.update.frameNum++;
			std::unique_lock<std::mutex> mlock(m_backBufferMutex);
			m_backBuffer = m_frameBuffer; // copy a frame to a back buffer
		}
	}
}

void dev::Display::FillActiveArea256(const int _rasterizedPixels)
{
	// scrolling
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	int rasterLineScrolled = (rasterLine - SCAN_ACTIVE_AREA_TOP + (255 - m_state.update.scrollIdx) + ACTIVE_AREA_H) % ACTIVE_AREA_H + SCAN_ACTIVE_AREA_TOP;

	// rasterization
	auto screenBytes = GetScreenBytes(rasterLineScrolled, rasterPixel);
	int bitIdx = 7 - (((m_state.update.framebufferIdx - m_borderLeft) % RASTERIZED_PXLS_MAX) >> 1);

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		auto colorIdx = BytesToColorIdx256(screenBytes, bitIdx);
		auto color = m_fullPalette[m_io.GetColor(colorIdx)];

		m_frameBuffer[m_state.update.framebufferIdx++] = color;

		bitIdx -= i % 2;
		if (bitIdx < 0) {
			bitIdx = 7;
			screenBytes = GetScreenBytes(rasterLineScrolled, GetRasterPixel());
		}
	}
}

void dev::Display::FillActiveArea256PortHandling(const int _rasterizedPixels)
{
	// scrolling
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	int rasterLineScrolled = (rasterLine - SCAN_ACTIVE_AREA_TOP + (255 - m_state.update.scrollIdx) + ACTIVE_AREA_H) % ACTIVE_AREA_H + SCAN_ACTIVE_AREA_TOP;

	// rasterization
	auto screenBytes = GetScreenBytes(rasterLineScrolled, rasterPixel);
	int bitIdx = 7 - (((m_state.update.framebufferIdx - m_borderLeft) % RASTERIZED_PXLS_MAX) >> 1);

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		rasterPixel = GetRasterPixel();

		if (rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel == SCROLL_COMMIT_PXL) {
			m_state.update.scrollIdx = m_io.GetScroll();
		}

		auto colorIdx = BytesToColorIdx256(screenBytes, bitIdx);
		m_io.TryToCommit(colorIdx);
		auto color = m_fullPalette[m_io.GetColor(colorIdx)];

		m_frameBuffer[m_state.update.framebufferIdx++] = color;

		bitIdx -= i % 2;
		if (bitIdx < 0){
			bitIdx = 7;
			screenBytes = GetScreenBytes(rasterLineScrolled, rasterPixel);
		}
	}
}

void dev::Display::FillActiveArea512PortHandling(const int _rasterizedPixels)
{
	// scrolling
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	int rasterLineScrolled = (rasterLine - SCAN_ACTIVE_AREA_TOP + (255 - m_state.update.scrollIdx) + ACTIVE_AREA_H) % ACTIVE_AREA_H + SCAN_ACTIVE_AREA_TOP;

	// rasterization
	auto screenBytes = GetScreenBytes(rasterLineScrolled, rasterPixel); // 4 bytes. One byte per screen buffer
	int pxlIdx = 15 - ((m_state.update.framebufferIdx - m_borderLeft) % RASTERIZED_PXLS_MAX); // 0-15

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		rasterPixel = GetRasterPixel();

		if (rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel == SCROLL_COMMIT_PXL) {
			m_state.update.scrollIdx = m_io.GetScroll();
		}

		auto colorIdx = BytesToColorIdx512(screenBytes, pxlIdx);
		m_io.TryToCommit(colorIdx);
		auto color = m_fullPalette[m_io.GetColor(colorIdx)];

		m_frameBuffer[m_state.update.framebufferIdx++] = color;

		pxlIdx--;
		if (pxlIdx < 0){
			pxlIdx = 15;
			screenBytes = GetScreenBytes(rasterLineScrolled, rasterPixel);
		}
	}
}

void dev::Display::FillActiveArea512(const int _rasterizedPixels)
{
	// scrolling
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	int rasterLineScrolled = (rasterLine - SCAN_ACTIVE_AREA_TOP + (255 - m_state.update.scrollIdx) + ACTIVE_AREA_H) % ACTIVE_AREA_H + SCAN_ACTIVE_AREA_TOP;

	// rasterization
	auto screenBytes = GetScreenBytes(rasterLineScrolled, rasterPixel); // 4 bytes. One byte per screen buffer
	int pxlIdx = 15 - ((m_state.update.framebufferIdx - m_borderLeft) % RASTERIZED_PXLS_MAX); // 0-15

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		auto colorIdx = BytesToColorIdx512(screenBytes, pxlIdx);
		auto color = m_fullPalette[m_io.GetColor(colorIdx)];

		m_frameBuffer[m_state.update.framebufferIdx++] = color;

		pxlIdx--;
		if (pxlIdx < 0){
			pxlIdx = 15;
			screenBytes = GetScreenBytes(rasterLineScrolled, GetRasterPixel());
		}
	}
}

bool dev::Display::IsIRQ() { return m_state.update.irq; }

auto dev::Display::GetFrame(const bool _vsync)
->const FrameBuffer*
{
	std::unique_lock<std::mutex> mlock(m_backBufferMutex);
	m_gpuBuffer = _vsync ? m_backBuffer : m_frameBuffer;

	return &m_gpuBuffer;
}

// Vector color format: uint8_t BBGGGRRR
// Output Color: ABGR (Imgui Image)
auto dev::Display::VectorColorToArgb(const uint8_t _vColor)
-> ColorI
{
	int r = (_vColor & 0x07);
	int g = (_vColor & 0x38) >> 3;
	int b = (_vColor & 0xc0) >> 6;

	uint32_t color =
		0xff000000 |
		(r << (5 + 0)) |
		(g << (5 + 8 )) |
		(b << (6 + 16 ));
	return color;
}

/**
 * Retrieves four screen bytes at the current raster line and pixel.
 * Each byte for each graphic buffer.
 */
uint32_t dev::Display::GetScreenBytes(int _rasterLine, int _rasterPixel)
{
	auto addrHigh = (_rasterPixel - m_borderLeft) / RASTERIZED_PXLS_MAX;
	auto addrLow = ACTIVE_AREA_H - 1 - (_rasterLine - SCAN_ACTIVE_AREA_TOP);
	Addr screenAddrOffset = static_cast<Addr>(addrHigh << 8 | addrLow);

	return m_memory.GetScreenBytes(screenAddrOffset);
}

// 256 screen mode
// extract a 4-bit color index from the four screen bytes.
// _bitIdx is in the range [0..7]
uint32_t dev::Display::BytesToColorIdx256(uint32_t _screenBytes, uint8_t _bitIdx)
{
	return (_screenBytes >> (_bitIdx - 0 + 0))  & 1 |
		   (_screenBytes >> (_bitIdx - 1 + 8))  & 2 |
		   (_screenBytes >> (_bitIdx - 2 + 16)) & 4 |
		   (_screenBytes >> (_bitIdx - 3 + 24)) & 8;
}

// 512 screen mode
// extract a 3-bit color index from the four screen bytes.
// _bitIdx is in the range [0..15]
// In the 512x256 mode, the even pixel colors are stored in screen buffers 3 and 2,
// and the odd ones - in screen buffers 0 and 1
uint32_t dev::Display::BytesToColorIdx512(uint32_t _screenBytes, uint8_t _bitIdx)
{
	bool even = _bitIdx & 1;
	_bitIdx >>= 1;

	if (even) {
		return
			(_screenBytes >> (_bitIdx - 0 + 0)) & 1 |
			(_screenBytes >> (_bitIdx - 1 + 8)) & 2;
	}

	return
		((_screenBytes >> (_bitIdx - 0 + 16)) & 1 |
		(_screenBytes >> (_bitIdx - 1 + 24)) & 2) * 4;
}

// rasterizes the memory into the frame buff
void dev::Display::BuffUpdate(BufferType _bufferType)
{
	switch (_bufferType)
	{
	case dev::Display::BufferType::FRAME_BUFFER:
		FrameBuffUpdate();
		break;

	case dev::Display::BufferType::BACK_BUFFER:
		m_backBuffer = m_frameBuffer;
		break;

	case dev::Display::BufferType::GPU_BUFFER:
		m_gpuBuffer = m_frameBuffer;
		break;

	default:
		break;
	}
}

void dev::Display::FrameBuffUpdate()
{
	int framebufferIdxTemp = m_state.update.framebufferIdx;
	m_state.update.framebufferIdx = 0;

	for(int i=0; i < FRAME_LEN; i += 16)
	{
		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();

		bool isActiveScan = rasterLine >= SCAN_ACTIVE_AREA_TOP && rasterLine < SCAN_ACTIVE_AREA_TOP + ACTIVE_AREA_H;
		bool isActiveArea = isActiveScan &&
			rasterPixel >= m_borderLeft && rasterPixel < BORDER_RIGHT;

		// Rasterize the Active Area
		if (isActiveArea)
		{
			int rasterizedPixels = dev::Min(BORDER_RIGHT - rasterPixel, RASTERIZED_PXLS_MAX);

			if (m_io.GetDisplayMode() == IO::MODE_256) FillActiveArea256(rasterizedPixels);
			else FillActiveArea512(rasterizedPixels);

			// Rasterize the border if there is a leftover
			if (rasterizedPixels < RASTERIZED_PXLS_MAX)
			{
				rasterizedPixels = RASTERIZED_PXLS_MAX - rasterizedPixels;
				FillBorder(rasterizedPixels);
			}
		}
		// Rasterize the Border
		else {
			int rasterizedPixels = !isActiveScan || rasterPixel >= BORDER_RIGHT ? RASTERIZED_PXLS_MAX :
				dev::Min(m_borderLeft - rasterPixel, RASTERIZED_PXLS_MAX);

			FillBorder(rasterizedPixels);

			// Rasterize the Active Area if there is a leftover
			if (rasterizedPixels < RASTERIZED_PXLS_MAX)
			{
				rasterizedPixels = RASTERIZED_PXLS_MAX - rasterizedPixels;
				if (m_io.GetDisplayMode() == IO::MODE_256) FillActiveArea256(rasterizedPixels);
				else FillActiveArea512(rasterizedPixels);
			}
		}
	}

	m_state.update.framebufferIdx = framebufferIdxTemp;
}