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

#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_UNSIGNED_INT                   0x1405
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_R8 0x8229
#define GL_R32UI 0x8236
#define GL_RED 0x1903

	constexpr static ErrCode NO_ERRORS = 0;
	constexpr static ErrCode ERROR_UNSPECIFIED = 1;
	constexpr static ErrCode ERROR_NO_FILES = 2;

	static constexpr int DELAYED_SELECTION_TIME = 6;
}

#endif // !DEV_CONSTS_H