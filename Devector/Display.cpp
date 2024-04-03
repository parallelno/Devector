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

// _isOutCommitMCicle is true when OUT command executes M3 T1
void dev::Display::Rasterize()
{
	bool isHorizBorder = m_rasterPixel < BORDER_LEFT || m_rasterPixel >= BORDER_LEFT + RES_W;
	bool isVertBorder = m_rasterLine < SCAN_ACTIVE_AREA_TOP || m_rasterLine >= SCAN_ACTIVE_AREA_TOP + RES_H;
	bool isBorder = isHorizBorder || isVertBorder;
	
	auto outCommitTime = m_io.GetOutCommitTime();
	auto paletteCommitTime = m_io.GetPaletteCommitTime();

	bool isPortPortHandling = m_rasterLine == 0 || m_rasterLine == 311 || isHorizBorder ||
		outCommitTime >= IO::PORT_COMMIT_TIME || paletteCommitTime >= IO::PORT_COMMIT_TIME;

	if (isBorder) {
		if (isPortPortHandling) FillBorderWithPortHandling();
		else FillBorder();
	}
	else if (isPortPortHandling) {
		if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveAreaMode256WithPortHandling();
		else FillActiveAreaMode512WithPortHandling();
	}
	else if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveAreaMode256();
	else FillActiveAreaMode512();

	

	
	// advance the m_rasterPixel & m_rasterLine
	m_rasterPixel = m_rasterPixel % FRAME_W;
	m_rasterLine = m_rasterPixel == 0 ? (m_rasterLine + 1) % FRAME_H : m_rasterLine;

	m_irq = m_rasterLine == 0 && m_rasterPixel == IO::IRQ_COMMIT_PXL;

	if (m_rasterPixel + m_rasterLine == 0)
	{
		m_frameNum++;
		// copy a frame to a back buffer
		std::unique_lock<std::mutex> mlock(m_backBufferMutex);
		m_backBuffer = m_frameBuffer;
	}
}

void dev::Display::FillBorder()
{
	auto borderColor = m_io.GetBorderColor();
	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = borderColor;
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = borderColor;
	}
}

// fill up pixels in the border. works for 256 & 512 modes
void dev::Display::FillBorderWithPortHandling()
{
	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		auto borderColorIdx = m_io.GetBorderColorIdx();
		m_io.CommitTimersHandling(borderColorIdx);

		auto color = m_io.GetBorderColor();
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
	}
}

// it takes 4 bytes, each from every screen buffer
// then it swizzels the bits like this:
// buffer8000_byte = qwertyui
// bufferA000_byte = opasdfgh
// bufferC000_byte = jklzxcvb
// bufferE000_byte = nm?!@#$%
// out uint32_t = %bhi $vgu #cfy @xdt !zsr ?lae mkpw njoq
// an examble:
//	input:	1111 1111 0111 1111 0000 0000 0000 0000
//	the input's similar to:
//		buffer8000_byte = 0000 0000
//		bufferA000_byte = 0000 0000
//		bufferC000_byte = 0111 1111
//		bufferE000_byte = 1111 1111
//	output: 1100 1100 1100 1100 1100 1100 1100 1000
uint32_t dev::Display::BytesToColorIdxs()
{
	auto addrHigh = (m_rasterPixel - BORDER_LEFT) / RASTERIZED_PXLS * RES_H;
	auto addrLow = RES_H - 1 - (m_rasterLine - SCAN_ACTIVE_AREA_TOP);
	Addr screenSpaceAddrOffset = addrHigh + addrLow;

	auto bytes = m_memory.GetScreenSpaceBytes(screenSpaceAddrOffset);

	uint32_t result = 0;
	for (int i = 0; i < 8; i++)
	{
		result <<= 4;
		result |= (bytes >> (i - 0 + 0 )) & 1;
		result |= (bytes >> (i - 1 + 8 )) & 2;
		result |= (bytes >> (i - 2 + 16)) & 4;
		result |= (bytes >> (i - 3 + 24)) & 8;
	}

	return result;
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

		colorIdx = colorIdxs & 0x0f;
		colorIdxs >>= 4;

		color = m_io.GetColor(colorIdx);
		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = color;
	}
}

void dev::Display::FillActiveAreaMode256WithPortHandling()
{
	auto colorIdxs = BytesToColorIdxs();

	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		auto colorIdx = colorIdxs & 0x0f;
		colorIdxs >>= 4;

		m_io.CommitTimersHandling(colorIdxs);

		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = m_io.GetColor(colorIdx /* & 0x03 */); // TODO: figure out why there's 0x03

		colorIdx = colorIdxs & 0x0f;
		colorIdxs >>= 4;

		m_frameBuffer[m_rasterLine * FRAME_W + m_rasterPixel++] = m_io.GetColor(colorIdx /* & 0x0c */); // TODO: figure out why there's 0x0c

		if (m_rasterLine == SCAN_ACTIVE_AREA_TOP && m_rasterPixel == IO::SCROLL_COMMIT_PXL)
		{
			m_scrollIdx = m_io.GetScroll();
		}
	}
}

void dev::Display::FillActiveAreaMode512WithPortHandling()
{
	// TODO: replace with a proper code
	FillActiveAreaMode256WithPortHandling();
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
auto dev::Display::VectorColorToArgb(const uint8_t _vColor) 
-> ColorI
{
	int r = (_vColor & 0x07);
	int g = (_vColor & 0x38) >> 3;
	int b = (_vColor & 0xc0) >> 6;

	uint32_t color =
		0xff000000 |
		(r << (5 + 16)) |
		(g << (5 + 8 )) |
		(b << (6 + 0 ));
	return color;
}