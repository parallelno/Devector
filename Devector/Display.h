#pragma once
#ifndef DEV_DISPLAY_H
#define DEV_DISPLAY_H

#include "Memory.h"

namespace dev
{
	class Display
	{
		const Memory& m_memory;
	public:
		Display(const Memory& _memory);
	};
}
#endif // !DEV_DISPLAY_H