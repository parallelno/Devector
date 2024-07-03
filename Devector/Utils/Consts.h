#pragma once

#include "Utils/Types.h"

namespace dev
{
	static constexpr int MAIN_WINDOW_W = 820;
	static constexpr int MAIN_WINDOW_H = 670;
	static constexpr int MAIN_WINDOW_X = 0;
	static constexpr int MAIN_WINDOW_Y = 32;
	static constexpr int DEFAULT_FONT_SIZE = 18;
	static constexpr int DEFAULT_FONT_ITALIC_SIZE = 18;
	static const char* DEFAULT_FONT_PATH = "fonts//medium.ttf";
	static const char* DEFAULT_FONT_ITALIC_PATH = "fonts//medium_italic.ttf";

	static const char* ConditionsS[] = {
		"ANY", "EQU", "LESS", "GREATER",
		"LESS_EQU", "GREATER_EQU", "NOT_EQU" };

#define GL_NEAREST			0x2600
#define GL_LINEAR			0x2601
#define GL_UNSIGNED_INT		0x1405
#define GL_UNSIGNED_BYTE	0x1401
#define GL_RGB				0x1907
#define GL_RGBA				0x1908
#define GL_R8				0x8229
#define GL_R32UI			0x8236
#define GL_RED				0x1903

	static constexpr ErrCode NO_ERRORS = 0;
	static constexpr ErrCode ERROR_UNSPECIFIED = 1;
	static constexpr ErrCode ERROR_NO_FILES = 2;

	static constexpr int DELAYED_SELECTION_TIME = 6;
}