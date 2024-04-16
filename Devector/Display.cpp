#include "Display.h"

dev::Display::Display(Memory& _memory, IO& _io)
	:
	m_memory(_memory), m_io(_io), m_scrollIdx(0xff)
{
	Init();
}

void dev::Display::Init()
{
	m_framebufferIdx = 0;
	m_frameBuffer.fill(0xff000000);
}

void dev::Display::Rasterize()
{
	// reset the interrupt request. it can be set during border drawing.
	m_irq = false;

	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();

	bool isHorizBorder = rasterPixel < BORDER_LEFT || rasterPixel >= BORDER_LEFT + ACTIVE_AREA_W;
	bool isVertBorder = rasterLine < SCAN_ACTIVE_AREA_TOP || rasterLine >= SCAN_ACTIVE_AREA_TOP + ACTIVE_AREA_H;
	bool isBorder = isHorizBorder || isVertBorder;

	auto outCommitTimer = m_io.GetOutCommitTimer();
	auto paletteCommitTimer = m_io.GetPaletteCommitTimer();

	bool isPortHandling = rasterLine == 0 || rasterLine == 311 || isHorizBorder ||
		outCommitTimer >= 0 || paletteCommitTimer >= 0;

	if (isBorder)
	{
		if (isPortHandling)
			FillBorderWithPortHandling();
		else FillBorder();
	}
	else
		if (isPortHandling)
		{
			if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveAreaMode256WithPortHandling(isBorder);
			else FillActiveAreaMode512WithPortHandling(isBorder);
		}
		else if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveAreaMode256();
		else FillActiveAreaMode512();
}

// fill up pixels in the border. works for 256 & 512 modes
void dev::Display::FillBorderWithPortHandling(const int _rasterizedPixels)
{
	int isNewFrame = 0;
	for (int i = 0; i < _rasterizedPixels; i++)
	{
		m_io.TryToCommit(m_io.GetBorderColorIdx());
		auto color = m_io.GetBorderColor();

		m_frameBuffer[m_framebufferIdx++] = color;
		isNewFrame = m_framebufferIdx / FRAME_LEN;
		m_framebufferIdx %= FRAME_LEN;

		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		m_irq |= rasterLine == 0 && rasterPixel >= dev::IRQ_COMMIT_PXL;

		if (isNewFrame)
		{
			m_frameNum++;
			// copy a frame to a back buffer
			std::unique_lock<std::mutex> mlock(m_backBufferMutex);
			m_backBuffer = m_frameBuffer;
		}
	}
}

void dev::Display::FillActiveAreaMode256WithPortHandling(const int _rasterizedPixels)
{
	auto screenBytes = GetScreenBytes();

	for (int i = 0; i < _rasterizedPixels; i++)
	{
		auto colorIdx = BytesToColorIdx(screenBytes, (i % RASTERIZED_PXLS) >> 1);

		m_io.TryToCommit(colorIdx);

		auto color = m_io.GetColor(colorIdx);

		m_frameBuffer[m_framebufferIdx++] = color;
		m_framebufferIdx %= FRAME_LEN;

		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		if (rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel == BORDER_LEFT)
		{
			m_scrollIdx = m_io.GetScroll();
		}
	}
}

void dev::Display::FillBorder(const int _rasterizedPixels)
{
	auto borderColor = m_io.GetBorderColor();
	for (int i = 0; i < _rasterizedPixels; i++)
	{
		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		m_frameBuffer[m_framebufferIdx++] = borderColor;
	}
}

void dev::Display::FillActiveAreaMode256(const int _rasterizedPixels)
{
	auto colorIdxs = BytesToColorIdxs();
	for (int i = 0; i < _rasterizedPixels; i += 2)
	{
		auto colorIdx = colorIdxs & 0x0f;
		colorIdxs >>= 4;

		auto color = m_io.GetColor(colorIdx);
		m_frameBuffer[m_framebufferIdx++] = color;
		m_frameBuffer[m_framebufferIdx++] = color;
		m_framebufferIdx %= FRAME_LEN;
	}
}

void dev::Display::FillActiveAreaMode512WithPortHandling(const int _rasterizedPixels)
{
	// TODO: replace with a proper code
	FillActiveAreaMode256WithPortHandling(_rasterizedPixels);
}

void dev::Display::FillActiveAreaMode512(const int _rasterizedPixels)
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
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();

	auto addrHigh = (rasterPixel - BORDER_LEFT) / RASTERIZED_PXLS * ACTIVE_AREA_H;
	auto addrLow = ACTIVE_AREA_H - 1 - (rasterLine - SCAN_ACTIVE_AREA_TOP);
	Addr screenAddrOffset = addrHigh + addrLow;

	auto bytes = m_memory.GetScreenBytes(screenAddrOffset);

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

uint32_t dev::Display::GetScreenBytes()
{
	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();

	auto addrHigh = (rasterPixel - BORDER_LEFT) / RASTERIZED_PXLS;
	auto addrLow = ACTIVE_AREA_H - 1 - (rasterLine - SCAN_ACTIVE_AREA_TOP);
	Addr screenAddrOffset = static_cast<Addr>(addrHigh << 8 | addrLow);

	return m_memory.GetScreenBytes(screenAddrOffset);
}

uint32_t dev::Display::BytesToColorIdx(uint32_t _screenBytes, uint8_t _bitIdx)
{
	_bitIdx = 7 - _bitIdx % 8;
	return (_screenBytes >> (_bitIdx - 0 + 0)) & 1  |
		   (_screenBytes >> (_bitIdx - 1 + 8)) & 2  |
		   (_screenBytes >> (_bitIdx - 2 + 16)) & 4 |
		   (_screenBytes >> (_bitIdx - 3 + 24)) & 8;
}