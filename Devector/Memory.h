#pragma once
#ifndef DEV_MEMORY_H
#define DEV_MEMORY_H

#include <cstdint>

namespace dev
{
	class Memory
	{
	public: 
		
		enum AddrSpace
		{
			RAM, STACK, GLOBAL
		};

		auto GetByte(uint32_t _addr, Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM) -> uint8_t;
		void SetByte(uint32_t _addr, uint8_t _value, Memory::AddrSpace _addr_space = Memory::AddrSpace::RAM);
	};
}
#endif // !DEV_MEMORY_H