#pragma once

#include "core/hardware.h"
#include "utils/imgui_utils.h"
#include "ui/base_window.h"
#include "utils/glu_utils.h"

namespace dev
{
	class KeyboardWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 600;
		static constexpr int DEFAULT_WINDOW_H = 250;

		static constexpr float KEYBOARD_IMG_ASPECT =  682.0f / 1280.0f;

		Hardware& m_hardware;
		ReqUI& m_reqUI;
		std::string m_pathImgKeyboard;
		
		bool m_isGLInited = false;
		GLUtils& m_glUtils;
		GLUtils::Vec4 m_pressedKeyImgIdx_scaleXY = { 0, 1.0f, 1.0f, 0};
		GLuint m_vramShaderId = -1;
		GLUtils::MaterialId m_vramMatId;
		GLuint m_vramTexId = -1;	
		int m_imgKeyboardH;
		int m_imgKeyboardW;
		int m_imgKeyboardCh;
		bool m_displayIsHovered = false;
		const char* m_contextMenuName = "##keyboardCMenu";
		bool m_windowFocused = false;

		bool Init();

	public:
		KeyboardWindow(Hardware& _hardware, const float* const _dpiScaleP, 
			GLUtils& _glUtils, ReqUI& _reqUI, const std::string& _pathImgKeyboard);
		void Update(bool& _visible);
		void Draw(const bool _isRunning);
		bool IsFocused() const;
	};
};