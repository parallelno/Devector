#pragma once
#ifndef DEV_CONSTS_H
#define DEV_CONSTS_H

#include "Utils/Types.h"

namespace dev
{
	constexpr static int MAIN_WINDOW_W = 800;
	constexpr static int MAIN_WINDOW_H = 600;
	constexpr static int MAIN_WINDOW_X = 0;
	constexpr static int MAIN_WINDOW_Y = 0;

	constexpr static int GL_NEAREST1 = 0x2600;
	constexpr static int GL_LINEAR1 = 0x2601;

	constexpr static ErrCode NO_ERRORS = 0;
	constexpr static ErrCode ERROR_UNSPECIFIED = 1;
	constexpr static ErrCode ERROR_NO_FILES = 2;
}

#endif // !DEV_CONSTS_H