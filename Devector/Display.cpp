#include "Display.h"
#include "Utils/Utils.h"

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

// renders 16 pixels (in the 512 mode) from left to right
void dev::Display::Rasterize()
{
	// reset the interrupt request. it can be set during border drawing.
	m_irq = false;

	int rasterLine = GetRasterLine();
	int rasterPixel = GetRasterPixel();
	int rasterizedPixels = RASTERIZED_PXLS_MAX;
	
	bool isActiveScan = rasterLine >= SCAN_ACTIVE_AREA_TOP && rasterLine < SCAN_ACTIVE_AREA_TOP + ACTIVE_AREA_H;
	bool isActiveArea = isActiveScan &&
					rasterPixel >= BORDER_LEFT && rasterPixel < BORDER_RIGHT;

	auto commitTime = m_io.GetOutCommitTimer() >= 0 || m_io.GetPaletteCommitTimer() >= 0;

	if (isActiveArea)
	{
		rasterizedPixels = dev::Min(BORDER_RIGHT - rasterPixel, RASTERIZED_PXLS_MAX);
		if (commitTime) 
		{
			if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveArea256PortHandling(rasterizedPixels);
			else FillActiveArea512PortHandling(rasterizedPixels);
		}
		else {
			if (m_io.GetDisplayMode() == IO::DISPLAY_MODE_256) FillActiveArea256(rasterizedPixels);
			else FillActiveArea512(rasterizedPixels);
		}
	} 
	else {
		rasterizedPixels = !isActiveScan || rasterPixel >= BORDER_RIGHT ? RASTERIZED_PXLS_MAX :
						dev::Min(BORDER_LEFT - rasterPixel, RASTERIZED_PXLS_MAX);
		if (commitTime || rasterLine == 0 || rasterLine == 311)
		{
			FillBorderPortHandling(rasterizedPixels);
		}
		else {
			FillBorder(rasterizedPixels);
		}
	}
}

void dev::Display::FillBorder(const int _rasterizedPixels)
{
	auto borderColor = m_io.GetBorderColor();
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
		auto color = m_io.GetBorderColor();

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
		auto colorIdx = BytesToColorIdx(screenBytes, bitIdx);
		auto color = m_io.GetColor(colorIdx);

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
		auto colorIdx = BytesToColorIdx(screenBytes, bitIdx);
		m_io.TryToCommit(colorIdx);
		auto color = m_io.GetColor(colorIdx);

		m_frameBuffer[m_framebufferIdx++] = color;

		int rasterLine = GetRasterLine();
		int rasterPixel = GetRasterPixel();
		if (rasterLine == SCAN_ACTIVE_AREA_TOP && rasterPixel == BORDER_LEFT)
		{
			m_scrollIdx = m_io.GetScroll();
		}
		
		bitIdx--;
		if (bitIdx < 0){
			bitIdx = 7;
			screenBytes = GetScreenBytes();
		}		
	}
}

void dev::Display::FillActiveArea512PortHandling(const int _rasterizedPixels)
{
	// TODO: replace with a proper code
	FillActiveArea256PortHandling(_rasterizedPixels);
}

void dev::Display::FillActiveArea512(const int _rasterizedPixels)
{
	// TODO: replace with a proper code
	FillActiveArea256();
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
/*
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

	auto addrHigh = (rasterPixel - BORDER_LEFT) / RASTERIZED_PXLS;
	auto addrLow = ACTIVE_AREA_H - 1 - (rasterLine - SCAN_ACTIVE_AREA_TOP);
	Addr screenAddrOffset = addrHigh<<8 | addrLow;

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

uint32_t dev::Display::BytesToColorIdx(uint32_t _screenBytes, uint8_t _bitIdx)
{
	return (_screenBytes >> (_bitIdx - 0 + 0))  & 1 |
		   (_screenBytes >> (_bitIdx - 1 + 8))  & 2 |
		   (_screenBytes >> (_bitIdx - 2 + 16)) & 4 |
		   (_screenBytes >> (_bitIdx - 3 + 24)) & 8;
}