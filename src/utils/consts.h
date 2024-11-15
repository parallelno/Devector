#pragma once

#include "utils/types.h"

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
		"=ANY", "=", "<", ">",
		"<=", ">=", "!=" };


	enum class ErrCode : int {
		NO_ERRORS = 0,
		UNSPECIFIED = -1,
		NO_FILES,
		FAILED_SDL_INIT,
		FAILED_CREATION_WINDOW,
		FAILED_SDL_GET_DISPLAY_BOUNDS,
		FAILED_OPENGL_INIT,
		UNRECOGNIZED_CPU_INSTR,
		INVALID_ID,
	};

	static constexpr int DELAYED_SELECTION_TIME = 6;

	static constexpr Id INVALID_ID = -1;

}