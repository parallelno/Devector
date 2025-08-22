#pragma once

#include <vector>
#include <atomic>
#include <thread>

#include "utils/types.h"
#include "utils/consts.h"
#include "ui/base_window.h"
#include "utils/result.h"
#include "core/hardware.h"
#include "core/scripts.h"
#include "utils/gl_utils.h"
#include "imgui_app.h"
#include "scheduler.h"

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
		const char* m_execSpeedsS = " 1%\0 20%\0 50%\0 100%\0 200%\0 MAX\0\0";

		GLUtils& m_glUtils;
		GLUtils::Vec4 m_activeArea_pxlSize = { Display::ACTIVE_AREA_W, Display::ACTIVE_AREA_H, FRAME_PXL_SIZE_W, FRAME_PXL_SIZE_H};
		GLUtils::Vec4 m_scrollV_crtXY_highlightMul = { 255.0f * FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f};
		GLUtils::Vec4 m_bordsLRTB = {
						static_cast<float>(0), // inited in the constructor
						static_cast<float>(0), // inited in the constructor
						static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP * FRAME_PXL_SIZE_H),
						static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP + Display::ACTIVE_AREA_H) * FRAME_PXL_SIZE_H };

		dev::Id m_matParamId_scrollV_crtXY_highlightMul = -1;
		dev::Id m_matParamId_activeArea_pxlSize = -1;
		dev::Id m_matParamId_bordsLRTB = -1;

		dev::Id m_vramShaderId = -1;
		dev::Id m_vramTexId = -1;
		dev::Id m_vramMatId	= -1;
		bool m_isGLInited = false;
		bool m_displayIsHovered = false;
		const char* m_contextMenuName = "##displayCMenu";
		Scripts& m_scripts;

		bool Init(const std::string& m_display_vtxShader,
				const std::string& m_display_fragShader);

		void CallbackUpdateData(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data);

		void Draw(
			const dev::Signals _signals,
			dev::Scheduler::SignalData _data) override;
		void DrawDisplay();
		void DrawContextMenu();
		void CreateTexture(const bool _vsync);


		void DrawScriptsUIItems(const ImVec2& _pos, const ImVec2& _displaySize);

	public:
		DisplayWindow(Hardware& _hardware,
			dev::Scheduler& _scheduler,
			bool* _visibleP,
			const float* const _dpiScaleP,
			GLUtils& _glUtils,
			Scripts& _scripts,
			const Hardware::ExecSpeed _execSpeed,
			const std::string& _vtxShaderS,
			const std::string& _fragShaderS);

		bool IsFocused() const;
		auto GetExecutionSpeed() const { return m_execSpeed; };
		void SetExecutionSpeed(const Hardware::ExecSpeed _execSpeed);
	};

};