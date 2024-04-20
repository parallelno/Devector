#pragma once
#ifndef DEV_TYPES_H
#define DEV_TYPES_H

#include <cstdint>

namespace dev
{
	using GlobalAddr = uint32_t;
	using Addr = uint16_t;
	using ColorI = uint32_t;

	// TODO: replace it with GLuint
	using GLuint1 = unsigned int;
	using GLenum1 = unsigned int;

	using Id = int;
}
#endif // !DEV_TYPES_H