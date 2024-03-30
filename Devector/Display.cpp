#include "Display.h"

dev::Display::Display(Memory& _memory)
	:
	m_memory(_memory)
{
	Init();
}

void dev::Display::Init()
{
	m_mode = MODE_256;
	m_rasterLine = 0;
	m_rasterPixel = 0;
	m_frameNum = 0;

	m_frameBuffer.fill(0xffff0000);
}

void dev::Display::Rasterize()
{
	if (m_rasterLine < SCAN_BORDER_TOP || m_rasterLine >= SCAN_BORDER_TOP + RES_H ||
		m_rasterPixel < BORDER_LEFT || m_rasterPixel >= BORDER_LEFT + RES_W)
	{
		Draw8PxlsBorder();
	}
	else
	{
		Draw8PxlsActiveArea();
	}
	
	// advance the m_rasterPixel & m_rasterLine
	m_rasterPixel = (m_rasterPixel + RASTERIZED_PXLS) % FRAME_W;
	m_rasterLine = m_rasterPixel == 0 ? (m_rasterLine + 1) % FRAME_H : m_rasterLine;

	m_t50Hz = (m_rasterPixel + m_rasterLine) == 0;

	if (m_t50Hz)
	{
		m_frameNum++;
		// copy a frame to a back buffer
		std::unique_lock<std::mutex> mlock(m_backBufferMutex);
		m_backBuffer = m_frameBuffer;
	}
}

void dev::Display::Draw8PxlsActiveArea()
{
	auto pos_addr = (m_rasterPixel - BORDER_LEFT) / RASTERIZED_PXLS * RES_H + RES_H - 1 - (m_rasterLine - SCAN_BORDER_TOP);

	auto addr8 = (Addr)(0x8000 + pos_addr);
	auto addrA = (Addr)(0xA000 + pos_addr);
	auto addrC = (Addr)(0xC000 + pos_addr);
	auto addrE = (Addr)(0xE000 + pos_addr);

	auto color_byte8 = m_memory.GetByte(addr8, Memory::AddrSpace::GLOBAL);
	auto color_byteA = m_memory.GetByte(addrA, Memory::AddrSpace::GLOBAL);
	auto color_byteC = m_memory.GetByte(addrC, Memory::AddrSpace::GLOBAL);
	auto color_byteE = m_memory.GetByte(addrE, Memory::AddrSpace::GLOBAL);

	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		int color_bit8 = (color_byte8 >> (7 - i / 2)) & 1;
		int color_bitA = (color_byteA >> (7 - i / 2)) & 1;
		int color_bitC = (color_byteC >> (7 - i / 2)) & 1;
		int color_bitE = (color_byteE >> (7 - i / 2)) & 1;

		int palette_idx = color_bit8 | color_bitA << 1 | color_bitC << 2 | color_bitE << 3;

		m_fillColor = (color_bit8 | color_bitA | color_bitC | color_bitE) == 0 ? 0xff000000 : 0xffffffff; // palette[palette_idx];

		m_frameBuffer[m_rasterPixel + m_rasterLine * FRAME_W + i] = m_fillColor;
		m_frameBuffer[m_rasterPixel + m_rasterLine * FRAME_W + i + 1] = m_fillColor;
	}
}

void dev::Display::Draw8PxlsBorder()
{
	for (int i = 0; i < RASTERIZED_PXLS; i += 2)
	{
		m_frameBuffer[m_rasterPixel + m_rasterLine * FRAME_W + i] = 0xff0000ff;
		m_frameBuffer[m_rasterPixel + m_rasterLine * FRAME_W + i + 1] = 0xff0000ff;
	}
}

bool dev::Display::IsInt50Hz() { return m_t50Hz; }

auto dev::Display::GetFrame(const bool _vsync)
->const FrameBuffer*
{
	std::unique_lock<std::mutex> mlock(m_backBufferMutex);
	m_gpuBuffer = _vsync ? m_backBuffer : m_frameBuffer;

	return &m_gpuBuffer;
}