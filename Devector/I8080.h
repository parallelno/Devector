#pragma once
#ifndef DEV_I8080_H
#define DEV_I8080_H

#include <functional>

#include "Memory.h"
#include "Debugger.h"

namespace dev 
{
	class I8080
	{
	public:

		using MemoryReadFunc = std::function <uint8_t (uint32_t _addr, Memory::AddrSpace _addrSpace)>;
		using MemoryWriteFunc = std::function <void (uint32_t _addr, uint8_t _value, Memory::AddrSpace _addrSpace)>;
		using InputFunc = std::function <uint8_t (uint8_t _port)>;
		using OutputFunc = std::function <void (uint8_t _port, uint8_t _value)>;
		using DebugMemStatsFunc = std::function<void(uint32_t _addr, Debugger::MemAccess _memAccess, Memory::AddrSpace _addrSpace)>;

		I8080() = delete;
		I8080(
			MemoryReadFunc _memoryRead,
			MemoryWriteFunc _memoryWrite,
			InputFunc _input,
			OutputFunc _output,
			DebugMemStatsFunc _debugMemStats);
	};
}
#endif // !DEV_I8080_H