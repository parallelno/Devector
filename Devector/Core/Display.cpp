#include "Display.h"
#include "Utils/Utils.h"

dev::Display::Display(Memory& _memory, IO& _io)
	:
	m_memory(_memory), m_io(_io), m_scrollIdx(0xff)
{
	// init the full palette
	for (int i = 0; i < 256; i++) {
		m_fullPallete[i] = VectorColorToArgb(i);
	}

	Init();
}

void dev::Display::Init()
{
	m_framebufferIdx = 0;
	m_frameBuffer.fill(0xff000000);
}

void dev::Display::RasterizeActiveArea(const int _rasterizedPixels)
{
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	auto commitTime = m_io.GetOutCommitTimer() >= 0 || m_io.GetPaletteCommitTimer() >= 0;

	bool scrollTime = rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel < BORDER_LEFT + RASTERIZED_PXLS_MAX;
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
	m_irq = false;

	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	
	bool isActiveScan = rasterLine >= SCAN_ACTIVE_AREA_TOP && rasterLine < SCAN_ACTIVE_AREA_TOP + ACTIVE_AREA_H;
	bool isActiveArea = isActiveScan &&
					rasterPixel >= BORDER_LEFT && rasterPixel < BORDER_RIGHT;

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
	// Border
	else {
		int rasterizedPixels = !isActiveScan || rasterPixel >= BORDER_RIGHT ? RASTERIZED_PXLS_MAX :
						dev::Min(BORDER_LEFT - rasterPixel, RASTERIZED_PXLS_MAX);

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
	auto borderColor = m_fullPallete[m_io.GetBorderColor()];
	for (int i = 0; i < _rasterizedPixels; i++)
	{
		m_frameBuffer[m_framebufferIdx++] = borderColor;
	}
}

void dev::Display::FillBorderPortHandling(const int _rasterizedPixels)
{
	for (int i = 0; i < _rasterizedPixels; i++)
	{
		m_io.TryToCommit(m_io.GetBorderColorIdx());
		auto color = m_fullPallete[m_io.GetBorderColor()];

		m_frameBuffer[m_framebufferIdx++] = color;
		int isNewFrame = m_framebufferIdx / FRAME_LEN;
		m_framebufferIdx %= FRAME_LEN;

		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		m_irq |= rasterLine == 0 && rasterPixel == dev::IRQ_COMMIT_PXL;

		if (isNewFrame)
		{
			m_frameNum++;
			std::unique_lock<std::mutex> mlock(m_backBufferMutex);
			m_backBuffer = m_frameBuffer; // copy a frame to a back buffer
		}
	}
}

void dev::Display::FillActiveArea256(const int _rasterizedPixels)
{
	auto screenBytes = GetScreenBytes();
	int bitIdx = 7 - (((m_framebufferIdx - BORDER_LEFT) % RASTERIZED_PXLS_MAX) >> 1);

	for (int i = 0; i < _rasterizedPixels; i += 2)
	{
		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();

		auto colorIdx = BytesToColorIdx256(screenBytes, bitIdx);
		auto color = m_fullPallete[m_io.GetColor(colorIdx)];

		m_frameBuffer[m_framebufferIdx++] = color;
		m_frameBuffer[m_framebufferIdx++] = color;
		
		bitIdx--;
		if (bitIdx < 0){
			bitIdx = 7;
			screenBytes = GetScreenBytes();
		}
	}
}

void dev::Display::FillActiveArea256PortHandling(const int _rasterizedPixels)
{
	auto screenBytes = GetScreenBytes();
	int bitIdx = 7 - (((m_framebufferIdx - BORDER_LEFT) % RASTERIZED_PXLS_MAX) >> 1);

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		if (rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel == BORDER_LEFT)
		{
			m_scrollIdx = m_io.GetScroll();
		}

		auto colorIdx = BytesToColorIdx256(screenBytes, bitIdx);
		m_io.TryToCommit(colorIdx);
		auto color = m_fullPallete[m_io.GetColor(colorIdx)];

		m_frameBuffer[m_framebufferIdx++] = color;

		bitIdx -= i % 2;
		if (bitIdx < 0){
			bitIdx = 7;
			screenBytes = GetScreenBytes();
		}		
	}
}

void dev::Display::FillActiveArea512PortHandling(const int _rasterizedPixels)
{
	auto screenBytes = GetScreenBytes(); // 4 bytes. One byte per screen buffer
	int pxlIdx = 15 - ((m_framebufferIdx - BORDER_LEFT) % RASTERIZED_PXLS_MAX); // 0-15

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		if (rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel == BORDER_LEFT)
		{
			m_scrollIdx = m_io.GetScroll();
		}

		auto colorIdx = BytesToColorIdx512(screenBytes, pxlIdx);
		m_io.TryToCommit(colorIdx);
		auto color = m_fullPallete[m_io.GetColor(colorIdx)];

		m_frameBuffer[m_framebufferIdx++] = color;

		pxlIdx--;
		if (pxlIdx < 0){
			pxlIdx = 15;
			screenBytes = GetScreenBytes();
		}		
	}
}

void dev::Display::FillActiveArea512(const int _rasterizedPixels)
{
	auto screenBytes = GetScreenBytes(); // 4 bytes. One byte per screen buffer
	int pxlIdx = 15 - ((m_framebufferIdx - BORDER_LEFT) % RASTERIZED_PXLS_MAX); // 0-15

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();

		auto colorIdx = BytesToColorIdx512(screenBytes, pxlIdx);
		auto color = m_fullPallete[m_io.GetColor(colorIdx)];
		m_frameBuffer[m_framebufferIdx++] = color;
		
		pxlIdx--;
		if (pxlIdx < 0){
			pxlIdx = 15;
			screenBytes = GetScreenBytes();
		}
	}
}

bool dev::Display::IsIRQ() { return m_irq; }

auto dev::Display::GetFrame(const bool _vsync)
->const FrameBuffer*
{
	std::unique_lock<std::mutex> mlock(m_backBufferMutex);
	m_gpuBuffer = _vsync ? m_backBuffer : m_frameBuffer;

	return &m_gpuBuffer;
}

int dev::Display::GetRasterLine() const { return m_framebufferIdx / FRAME_W; };
int dev::Display::GetRasterPixel() const { return m_framebufferIdx % FRAME_W; };

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
uint32_t dev::Display::GetScreenBytes()
{
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();

	auto addrHigh = (rasterPixel - BORDER_LEFT) / RASTERIZED_PXLS_MAX;
	auto addrLow = ACTIVE_AREA_H - 1 - (rasterLine - SCAN_ACTIVE_AREA_TOP);
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
// extract a 2-bit color index from the four screen bytes.
// _bitIdx is in the range [0..15]
// In the 512x256 mode, the even pixel colors are stored in screen buffers 3 and 2,
// and the odd ones - in screen buffers 0 and 1
uint32_t dev::Display::BytesToColorIdx512(uint32_t _screenBytes, uint8_t _bitIdx)
{
	bool even = _bitIdx & 1;
	_bitIdx >>= 1;

	auto colorIdx0 = 
			(_screenBytes >> (_bitIdx - 0 + 0)) & 1 |
			(_screenBytes >> (_bitIdx - 1 + 8)) & 2;

	auto colorIdx1 = 
			(_screenBytes >> (_bitIdx - 1 + 16)) & 2 |
			(_screenBytes >> (_bitIdx - 0 + 24)) & 1;

	return even ? colorIdx0 : colorIdx1;
}