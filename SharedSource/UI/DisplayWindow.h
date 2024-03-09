#pragma once
#ifndef DEV_DISPLAY_WINDOW_H
#define DEV_DISPLAY_WINDOW_H

#include "imgui_impl_opengl3_loader.h"
#include <vector>
#include "../Devector/Types.h"
#include "Utils/Globals.h"
#include "Utils/ImGuiUtils.h"
#include "Ui/BaseWindow.h"
#include "Utils/Result.h"
#include "../Devector/Display.h"

namespace dev
{
	class DisplayWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 768;
		static constexpr int DEFAULT_WINDOW_H = 312;

		GLuint m_frameTextureId = 0;

		Display& m_display;
		void DrawDisplay();

		void CreateFrameBuffer(const ColorI _image[], const int _width, const int _height);
		void UpdateFrameBuffer();

	public:
		DisplayWindow(Display& _display, const float* const _fontSizeP, const float* const _dpiScaleP);
		void Update();
	};

};

#endif // !DEV_DISPLAY_WINDOW_H