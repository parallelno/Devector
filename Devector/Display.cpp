#include "Display.h"

dev::Display::Display(Memory& _memory, IO& _io)
	:
	m_memory(_memory), m_io(_io), m_scrollIdx(0xff)
{
	Init();
}

void dev::Display::Init()
{
	m_rasterLine = 0;
	m_rasterPixel = 0;
	m_frameNum = 0;

	m_frameBuffer.fill(0xffff0000);
}

// called every 4cc. draws 16 pixels in 512 mode and 8 pixels in 256 mode
void dev::Display::Rasterize()
{
	// reset the interrupt request. it can be set during border drawing.
	m_irq = false;

	bool isHorizBorder = m_rasterPixel < BORDER_LEFT || m_rasterPixel >= BORDER_LEFT + ACTIVE_AREA_W;
	bool isVertBorder = m_rasterLine < SCAN_ACTIVE_AREA_TOP || m_rasterLine >= SCAN_ACTIVE_AREA_TOP + ACTIVE_AREA_H;
	bool isBorder = isHorizBorder || isVertBorder;
	
	auto outCommitTimer = m_io.GetOutCommitTimer();
	auto paletteCommitTimer = m_io.GetPaletteCommitTimer();

	bool isPortPortHandling = m_rasterLine == 0 || m_rasterLine == 311 || isHorizBorder ||
		outCommitTimer >= 0 || paletteCommitTimer >= 0;

	if (isBorder) 
	{
		if (isPortPortHandling) 
			FillBorderWithPortHandling();
		else FillBorder();
	}
	else 
	if (isPortPortHandling)
	{
		if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveAreaMode256WithPortHandling(isBorder);
		else FillActiveAreaMode512WithPortHandling(isBorder);
	}
	else if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveAreaMode256();
	else FillActiveAreaMode512();
}

// fill up pixels in the border. works for 256 & 512 modes
void dev::Display::FillBorderWithPortHandling()
{
	int isNewFrame = 0;
	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		auto borderColorIdx = m_io.GetBorderColorIdx();
		m_io.CommitTimersHandling(borderColorIdx);

		auto color = m_io.GetBorderColor();
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
		
		m_rasterLine += m_rasterPixel / FRAME_W;
		isNewFrame = m_rasterLine / FRAME_H;
		m_rasterPixel %= FRAME_W;
		m_rasterLine %= FRAME_H;
		m_irq |= m_rasterLine == 0 && m_rasterPixel >= IO::IRQ_COMMIT_PXL;

		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;

		m_rasterLine += m_rasterPixel / FRAME_W;
		isNewFrame = m_rasterLine / FRAME_H;
		m_rasterPixel %= FRAME_W;
		m_rasterLine %= FRAME_H;
		m_irq |= m_rasterLine == 0 && m_rasterPixel >= IO::IRQ_COMMIT_PXL;

		if (isNewFrame)
		{
			m_frameNum++;
			// copy a frame to a back buffer
			std::unique_lock<std::mutex> mlock(m_backBufferMutex);
			m_backBuffer = m_frameBuffer;
		}
	}
}

void dev::Display::FillActiveAreaMode256WithPortHandling(const bool _isBorder)
{
	auto colorIdxs = BytesToColorIdxs();
	
	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		auto colorIdx = colorIdxs & 0x0f;
		colorIdxs >>= 4;

		m_io.CommitTimersHandling(colorIdxs);

		if (m_io.GetPaletteCommitTimer() >= 0 && !_isBorder) {
			m_colorsPolutedTimer = COLORS_POLUTED_DELAY;
		}

		auto color = m_io.GetColor(colorIdx);
		if (m_colorsPolutedTimer == 0)
		{
			color = 0xfff0f0f0;
			m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
			m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
		}
		else {
			m_colorsPolutedTimer -= m_colorsPolutedTimer < 0 ? 0 : 1;

			m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
			m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
		}

		if (m_rasterLine == SCAN_ACTIVE_AREA_TOP && m_rasterPixel == BORDER_LEFT)
		{
			m_scrollIdx = m_io.GetScroll();
		}
	}
}

void dev::Display::FillBorder()
{
	auto borderColor = m_io.GetBorderColor();
	for (int i = 0; i < RASTERIZED_PXLS; i++)
	{
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = borderColor;
	}
}

void dev::Display::FillActiveAreaMode256()
{
	auto colorIdxs = BytesToColorIdxs();
	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		auto colorIdx = colorIdxs & 0x0f;
		colorIdxs >>= 4;

		auto color = m_io.GetColor(colorIdx);
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
	}
}

void dev::Display::FillActiveAreaMode512WithPortHandling(const bool _isBorder)
{
	// TODO: replace with a proper code
	FillActiveAreaMode256WithPortHandling(_isBorder);
}

void dev::Display::FillActiveAreaMode512()
{
	// TODO: replace with a proper code
	FillActiveAreaMode256();
}

bool dev::Display::IsIRQ() { return m_irq; }

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

// it takes 4 bytes, each from every screen buffer
// then it swizzels the bits like this:
// bufferE000_byte = qwertyui
// bufferC000_byte = opasdfgh
// bufferA000_byte = jklzxcvb
// buffer8000_byte = nm?!@#$%
// out uint32_t = %bhi $vgu #cfy @xdt !zsr ?lae mkpw njoq
// an examble:
//	input:	1111 1111 0111 1111 0000 0000 0000 0000
//	the input's similar to:
//		bufferE000_byte = 0000 0000
//		bufferC000_byte = 0000 0000
//		bufferA000_byte = 0111 1111
//		buffer8000_byte = 1111 1111
//	output: 1100 1100 1100 1100 1100 1100 1100 1000
uint32_t dev::Display::BytesToColorIdxs()
{
	auto addrHigh = (m_rasterPixel - BORDER_LEFT) / RASTERIZED_PXLS * ACTIVE_AREA_H;
	auto addrLow = ACTIVE_AREA_H - 1 - (m_rasterLine - SCAN_ACTIVE_AREA_TOP);
	Addr screenSpaceAddrOffset = addrHigh + addrLow;

	auto bytes = m_memory.GetScreenSpaceBytes(screenSpaceAddrOffset);

	uint32_t result = 0;
	for (int i = 0; i < 8; i++)
	{
		result <<= 4;
		result |= (bytes >> (i - 0 + 0)) & 1;
		result |= (bytes >> (i - 1 + 8)) & 2;
		result |= (bytes >> (i - 2 + 16)) & 4;
		result |= (bytes >> (i - 3 + 24)) & 8;
	}

	return result;
}