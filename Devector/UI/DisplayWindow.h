#pragma once

#include <vector>

#include "Utils/Types.h"
#include "Utils/Consts.h"
#include "Ui/BaseWindow.h"
#include "Utils/Result.h"
#include "Core/Hardware.h"
#include "Utils/GLUtils.h"

namespace dev
{
	class DisplayWindow : public BaseWindow
	{
		static constexpr float WINDOW_ASPECT = 3.0f / 4.0f;
		static constexpr int DEFAULT_WINDOW_W = 800;
		static constexpr int DEFAULT_WINDOW_H = static_cast<int>(DEFAULT_WINDOW_W * WINDOW_ASPECT);
		static constexpr float SCANLINE_HIGHLIGHT_MUL = 0.3f;
		static constexpr float FRAME_PXL_SIZE_W = 1.0f / Display::FRAME_W;
		static constexpr float FRAME_PXL_SIZE_H = 1.0f / Display::FRAME_H;

		GLuint1 m_frameTextureId = 0;
		Hardware& m_hardware;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		bool m_windowFocused = false;
		int m_rasterPixel = 0;
		int m_rasterLine = 0;
		enum class BorderType : int { NONE = 0, NORMAL, FULL, LEN};
		BorderType m_borderType = BorderType::NORMAL;
		enum class DisplaySize : int { R256_256 = 0, R512_256, R512_512, MAX, LEN };
		DisplaySize m_displaySize = DisplaySize::MAX;
		const char* m_borderTypeS = " None\0 Normal\0 Full\0\0";
		const char* m_displaySizeS = " 256x256\0 512x256\0 512x512\0 Maximize\0\0";
		const char* m_borderTypeAS[3] = { "Border: None", "Border: Normal", "Border: Full" };
		const char* m_displaySizeAS[4] = { "Display Size: 256x256", "Display Size: 512x256", "Display Size: 512x512", "Display Size: Maximize" };
		Hardware::ExecSpeed m_execSpeed = Hardware::ExecSpeed::NORMAL;
		const char* m_execSpeedsS = " 10 PERCENT\0 HALF\0 NORMAL\0 X2\0 MAX\0\0";
		
		GLUtils& m_glUtils;
		GLUtils::Vec4 m_activeArea_pxlSize = { Display::ACTIVE_AREA_W, Display::ACTIVE_AREA_H, FRAME_PXL_SIZE_W, FRAME_PXL_SIZE_H};
		GLUtils::Vec4 m_scrollV_crtXY_highlightMul = { 255.0f * FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f};
		GLUtils::Vec4 m_bordsLRTB = {
						static_cast<float>(Display::BORDER_LEFT * FRAME_PXL_SIZE_W),
						static_cast<float>((Display::BORDER_LEFT + Display::ACTIVE_AREA_W) * FRAME_PXL_SIZE_W),
						static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP * FRAME_PXL_SIZE_H),
						static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP + Display::ACTIVE_AREA_H) * FRAME_PXL_SIZE_H };
		GLuint1 m_vramShaderId = -1;
		GLUtils::MaterialId m_vramMatId;
		GLuint1 m_vramTexId = -1;
		bool m_isGLInited = false;
		bool m_displayIsHovered = false;
		const char* m_contextMenuName = "##displayCMenu";

		void DrawDisplay();
		void DrawContextMenu();
		void CreateTexture(const bool _vsync);
		void UpdateData(const bool _isRunning);
		bool Init();

	public:
		DisplayWindow(Hardware& _hardware, const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils);
		void Update(bool& _visible);
		bool IsFocused() const;
	};

};