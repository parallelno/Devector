#pragma once
#ifndef DEV_DEBUGGER_H
#define DEV_DEBUGGER_H

#include <cstdint>

#include "Memory.h"

namespace dev
{
	class Debugger
	{
	public:
		enum MemAccess
		{
			RUN, READ, WRITE
		};

		void MemStats(uint32_t _addr, MemAccess _memAccess, Memory::AddrSpace _addrSpace = Memory::AddrSpace::RAM);
	};
}
#endif // !DEV_DEBUGGER_H