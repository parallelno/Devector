#pragma once
#ifndef DEV_IO_H
#define DEV_IO_H

#include <cstdint>

namespace dev
{
	class IO
	{
	public:
		auto PortIn(uint8_t _port) -> uint8_t;
		void PortOut(uint8_t _port, uint8_t _value);
	};
}
#endif // !DEV_IO_H