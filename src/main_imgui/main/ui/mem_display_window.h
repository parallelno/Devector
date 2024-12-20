#pragma once

#include <vector>

#include "utils/types.h"
#include "utils/consts.h"
#include "ui/base_window.h"
#include "utils/result.h"
#include "core/hardware.h"
#include "core/debugger.h"
#include "utils/gl_utils.h"

namespace dev
{
	class MemDisplayWindow : public BaseWindow
	{
		static constexpr int FRAME_BUFFER_W = 1024;
		static constexpr int FRAME_BUFFER_H = 512;

		static constexpr int DEFAULT_WINDOW_W = 1024;
		static constexpr int DEFAULT_WINDOW_H = 512;

		static constexpr float SCALE_MAX = 5.0f;
		static constexpr float SCALE_MIN = 0.3f;
		static constexpr float SCALE_INC = 0.2f;

		static constexpr int RAM_TEXTURES = Memory::GLOBAL_MEMORY_LEN / Memory::MEM_64K;
		static constexpr int RAM_TEXTURE_W = 256;
		static constexpr int RAM_TEXTURE_H = Memory::MEMORY_MAIN_LEN / 256;

		Hardware& m_hardware;
		Debugger& m_debugger;
		ReqUI& m_reqUI;

		int64_t m_ccLast = -1; // to force the first stats update
		float m_scale = 1.0f;

		GLUtils& m_glUtils;
		GLUtils::Vec4 m_globalColorBg = { 0.2f, 0.2f, 0.2f, 1.0f };
		GLUtils::Vec4 m_globalColorFg = { 1.0f, 1.0f, 1.0f, 1.0f };		
		GLUtils::Vec4 m_highlightRead = { 0.078f, 0.078f, 1.0f, 0.8f };
		GLUtils::Vec4 m_highlightWrite = { 1.0f, 0.078f, 0.078f, 0.8f };
		GLUtils::Vec4 m_highlightIdxMax = {Debugger::LAST_RW_MAX, 0.0f, 0.0f, 0.0f};
		dev::Id m_paramId_globalColorBg = -1;
		dev::Id m_paramId_globalColorFg = -1;
		dev::Id m_paramId_highlightRead = -1;
		dev::Id m_paramId_highlightWrite = -1;
		dev::Id m_paramId_highlightIdxMax = -1;

		dev::Id m_memViewShaderId = -1;
		dev::Id m_highlightShaderId = -1;
		std::array<dev::Id, RAM_TEXTURES> m_memViewMatIds;
		std::array<dev::Id, RAM_TEXTURES> m_memViewTexIds;
		std::array<dev::Id, RAM_TEXTURES> m_lastRWTexIds;
		Debugger::MemLastRW* m_lastRWIdxsP;
		bool m_isGLInited = false;

		void DrawDisplay();
		void UpdateData(const bool _isRunning);
		void ScaleView();
		bool Init();

	public:
		MemDisplayWindow(Hardware& _hardware, Debugger& _debugger, 
			const float* const _dpiScaleP, GLUtils& _glUtils, ReqUI& _reqUI);
		void Update(bool& _visible, const bool _isRunning);
	};

};