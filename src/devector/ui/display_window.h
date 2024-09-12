#pragma once

#include <vector>
#include <atomic>
#include <thread>

#include "utils/types.h"
#include "utils/consts.h"
#include "ui/base_window.h"
#include "utils/result.h"
#include "core/hardware.h"
#include "utils/glu_utils.h"

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

		GLuint m_frameTextureId = 0;
		Hardware& m_hardware;
		int64_t m_ccLast = -1; // to force the first stats update
		int64_t m_ccLastRun = 0;
		std::atomic_bool m_windowFocused = false;
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
		const char* m_execSpeedsS = " 20%\0 50%\0 100%\0 200%\0 MAX\0\0";
		
		GLUtils& m_glUtils;
		GLUtils::Vec4 m_activeArea_pxlSize = { Display::ACTIVE_AREA_W, Display::ACTIVE_AREA_H, FRAME_PXL_SIZE_W, FRAME_PXL_SIZE_H};
		GLUtils::Vec4 m_scrollV_crtXY_highlightMul = { 255.0f * FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f};
		GLUtils::Vec4 m_bordsLRTB = {
						static_cast<float>(0), // inited in the constructor
						static_cast<float>(0), // inited in the constructor
						static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP * FRAME_PXL_SIZE_H),
						static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP + Display::ACTIVE_AREA_H) * FRAME_PXL_SIZE_H };
		GLuint m_vramShaderId = -1;
		GLUtils::MaterialId m_vramMatId;
		GLuint m_vramTexId = -1;
		bool m_isGLInited = false;
		bool m_displayIsHovered = false;
		const char* m_contextMenuName = "##displayCMenu";
		ReqUI& m_reqUI;

		void DrawDisplay();
		void DrawContextMenu();
		void CreateTexture(const bool _vsync);
		void UpdateData(const bool _isRunning);
		bool Init();
		//void ReqHandling();

	public:
		DisplayWindow(Hardware& _hardware, const float* const _fontSizeP, 
			const float* const _dpiScaleP, GLUtils& _glUtils, ReqUI& _reqUI);
		void Update(bool& _visible);
		bool IsFocused() const;
	};

};