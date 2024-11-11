#include "halwrapper.h"
#include "utils\str_utils.h"
#include <msclr\marshal_cppstd.h>

#include <glad/glad.h>

dev::HAL::HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
	const bool _ramDiskClearAfterRestart)
{
    std::wstring pathBootData = msclr::interop::marshal_as<std::wstring>(_pathBootData);
    std::wstring pathRamDiskData = msclr::interop::marshal_as<std::wstring>(_pathRamDiskData);

	m_hardwareP = new Hardware(pathBootData, pathRamDiskData, _ramDiskClearAfterRestart);
	m_debuggerP = new Debugger(*m_hardwareP);
    m_winGlUtilsP = new WinGlUtils();
}

void dev::HAL::Init(System::IntPtr _hwnd, GLsizei _viewportW, GLsizei _viewportH)
{
    m_activeArea_pxlSizeP = new GLUtils::Vec4({ Display::ACTIVE_AREA_W, Display::ACTIVE_AREA_H, FRAME_PXL_SIZE_W, FRAME_PXL_SIZE_H });
    m_scrollV_crtXY_highlightMulP = new GLUtils::Vec4({ 255.0f * FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f });
    m_bordsLRTBP = new GLUtils::Vec4({
                        static_cast<float>(0), // inited in the constructor
                        static_cast<float>(0), // inited in the constructor
                        static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP * FRAME_PXL_SIZE_H),
                        static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP + Display::ACTIVE_AREA_H) * FRAME_PXL_SIZE_H });

    m_hardwareP->Request(Hardware::Req::RUN);
    m_hwnd_temp = static_cast<HWND>(_hwnd.ToPointer());
    auto res = m_winGlUtilsP->CreateGfxContext(m_hwnd_temp, _viewportW, _viewportH);

    m_isGLInited = DisplayWindowInit();
}

dev::HAL::~HAL()
{
    this->!HAL(); // Ensure finalizer is called
}

dev::HAL::!HAL()
{
    delete m_debuggerP; m_debuggerP = nullptr;
    delete m_hardwareP; m_hardwareP = nullptr;
    delete m_winGlUtilsP; m_winGlUtilsP = nullptr;

    delete m_activeArea_pxlSizeP; m_activeArea_pxlSizeP = nullptr;
    delete m_scrollV_crtXY_highlightMulP; m_scrollV_crtXY_highlightMulP = nullptr;
    delete m_bordsLRTBP; m_bordsLRTBP = nullptr;
}

uint64_t dev::HAL::GetCC()
{
    auto res = m_hardwareP->Request(Hardware::Req::GET_CC);
    const auto& data = *res;

    return data["cc"];
}

void dev::HAL::Run()
{
    m_hardwareP->Request(Hardware::Req::RUN);
}


// Vertex shader source code
const char* vtxShaderS = R"#(
	#version 330 core
	precision highp float;

	layout (location = 0) in vec3 pos;
	layout (location = 1) in vec2 uv;

	out vec2 uv0;

	void main()
	{
		uv0 = vec2(uv.x, 1.0f - uv.y);
		gl_Position = vec4(pos.xyz, 1.0f);
	}
)#";

// Fragment shader source code
const char* fragShaderS = R"#(
	#version 330 core
	precision highp float;
	precision highp int;

	in vec2 uv0;

	uniform sampler2D texture0;
	uniform vec4 m_activeArea_pxlSize;
	uniform vec4 m_bordsLRTB;
	uniform vec4 m_scrollV_crtXY_highlightMul;

	layout (location = 0) out vec4 out0;

	void main()
	{
		vec2 uv = uv0;
		float bordL = m_bordsLRTB.x;
		float bordR = m_bordsLRTB.y;
		float bordT = m_bordsLRTB.z;
		float bordB = m_bordsLRTB.w;
		float highlightMul = m_scrollV_crtXY_highlightMul.w;
		vec2 crt = m_scrollV_crtXY_highlightMul.yz;
		vec2 pxlSize = m_activeArea_pxlSize.zw;

		// vertical scrolling
		if (uv.x >= bordL &&
			uv.x < bordR &&
			uv.y >= bordT &&
			uv.y < bordB)
		{
			uv.y -= m_scrollV_crtXY_highlightMul.x;
			// wrap V
			uv.y += uv.y < bordT ? m_activeArea_pxlSize.y * pxlSize.y : 0.0f;
		}

		vec3 color = texture(texture0, uv).rgb;

		// crt scanline highlight
		if (highlightMul < 1.0f)
		{
			if (uv.y >= crt.y &&
				uv.y < crt.y + pxlSize.y &&
				uv.x < crt.x + pxlSize.x )
			{
				// highlight the rasterized pixels of the current crt line
				color.xyz = vec3(0.3f, 0.3f, 0.3f) + color.xyz * 2.0f;
			}
			else 
			if ((uv.y >= crt.y && 
				uv.y < crt.y + pxlSize.y &&
				uv.x >= crt.x + pxlSize.x ) || uv.y > crt.y + pxlSize.y)
			{
				// renders not rasterized pixels yet
				color.xyz *= m_scrollV_crtXY_highlightMul.w;
			}
		}

		out0 = vec4(color, 1.0f);
	}
)#";

bool dev::HAL::DisplayWindowInit()
{
    int borderLeft = m_hardwareP->Request(Hardware::Req::GET_DISPLAY_BORDER_LEFT)->at("borderLeft");
    m_bordsLRTBP->x = borderLeft * FRAME_PXL_SIZE_W;
    m_bordsLRTBP->y = (borderLeft + Display::ACTIVE_AREA_W) * FRAME_PXL_SIZE_W;

    auto vramShaderRes = m_winGlUtilsP->InitShader(m_hwnd_temp, vtxShaderS, fragShaderS);
    if (!vramShaderRes) return false;
    m_vramShaderId = *vramShaderRes;

    auto vramTexRes = m_winGlUtilsP->InitTexture(m_hwnd_temp, Display::FRAME_W, Display::FRAME_H, GLUtils::Texture::Format::RGBA);
    if (!vramTexRes) return false;
    m_vramTexId = *vramTexRes;

    GLUtils::ShaderParams shaderParams = {
        { "m_activeArea_pxlSize", &(*m_activeArea_pxlSizeP) },
        { "m_scrollV_crtXY_highlightMul", &(*m_scrollV_crtXY_highlightMulP) },
        { "m_bordsLRTB", &(*m_bordsLRTBP) }
    };
    
    auto vramMatRes = m_winGlUtilsP->InitMaterial(
        m_hwnd_temp, m_vramShaderId, { vramTexRes }, shaderParams,
        Display::FRAME_W, Display::FRAME_H);

    if (!vramMatRes) return false;
    m_vramMatId = *vramMatRes;

    return true;
}

void dev::HAL::UpdateData(const bool _isRunning,
    const GLsizei _viewportW, const GLsizei _viewportH)
{
    uint64_t cc = m_hardwareP->Request(Hardware::Req::GET_CC)->at("cc");
    auto ccDiff = cc - m_ccLast;
    m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
    m_ccLast = cc;

    m_scrollV_crtXY_highlightMulP->w = 1.0f;

    if (!_isRunning)
    {
        if (ccDiff)
        {
            auto res = m_hardwareP->Request(Hardware::Req::GET_DISPLAY_DATA);
            const auto& displayData = *res;
            m_rasterPixel = displayData["rasterPixel"];
            m_rasterLine = displayData["rasterLine"];
        }
        if (!m_displayIsHovered)
        {
            m_scrollV_crtXY_highlightMulP->y = (float)m_rasterPixel * FRAME_PXL_SIZE_W;
            m_scrollV_crtXY_highlightMulP->z = (float)m_rasterLine * FRAME_PXL_SIZE_H;
            m_scrollV_crtXY_highlightMulP->w = SCANLINE_HIGHLIGHT_MUL;
        }
    }

    // update
    if (m_isGLInited)
    {
        //uint8_t scrollVert = (m_hardware.Request(Hardware::Req::GET_SCROLL_VERT)->at("scrollVert") + 1; // adding +1 offset because the default is 255
        m_scrollV_crtXY_highlightMulP->x = 0;//FRAME_PXL_SIZE_H* scrollVert;

        auto frameP = m_hardwareP->GetFrame(_isRunning);
        m_winGlUtilsP->UpdateTexture(m_hwnd_temp, m_vramTexId, (uint8_t*)frameP->data());
        
        m_winGlUtilsP->DrawOnWindow(m_hwnd_temp, m_vramMatId,
            _viewportW, _viewportW,
            _viewportW, _viewportW,
            m_vramTexId);
    }
}

void dev::HAL::DrawDisplay(const GLsizei _viewportW, const GLsizei _viewportH)
{
    if (m_isGLInited)
    {/*
        float border = 0;
        ImVec2 borderMin;
        ImVec2 borderMax;
        switch (m_borderType)
        {
        case dev::DisplayWindow::BorderType::FULL:
            borderMin = { 0.0f, 0.0f };
            borderMax = { 1.0f, 1.0f };
            break;

        case dev::DisplayWindow::BorderType::NORMAL:
            border = Display::BORDER_VISIBLE;
            [[fallthrough]];

        case dev::DisplayWindow::BorderType::NONE:
        {
            int borderLeft = m_hardwareP->Request(Hardware::Req::GET_DISPLAY_BORDER_LEFT)->at("borderLeft");

            borderMin = {
                (borderLeft - border * 2) * FRAME_PXL_SIZE_W,
                (Display::SCAN_ACTIVE_AREA_TOP - border) * FRAME_PXL_SIZE_H };
            borderMax = {
                borderMin.x + (Display::ACTIVE_AREA_W + border * 4) * FRAME_PXL_SIZE_W,
                borderMin.y + (Display::ACTIVE_AREA_H + border * 2) * FRAME_PXL_SIZE_H };
            break;
        }
        }

        ImVec2 displaySize;
        switch (m_displaySize)
        {
        case dev::DisplayWindow::DisplaySize::R256_256:
            displaySize.x = 256.0f;
            displaySize.y = 256.0f;
            break;
        case dev::DisplayWindow::DisplaySize::R512_256:
            displaySize.x = 512.0f;
            displaySize.y = 256.0f;
            break;
        case dev::DisplayWindow::DisplaySize::R512_512:
            displaySize.x = 512.0f;
            displaySize.y = 512.0f;
            break;
        case dev::DisplayWindow::DisplaySize::MAX:
        {
            ImGuiStyle& style = ImGui::GetStyle();
            auto wMax = ImGui::GetWindowWidth() - style.FramePadding.x * 4;
            auto hMax = ImGui::GetWindowHeight() - style.FramePadding.y * 14;

            displaySize.x = wMax;
            displaySize.y = displaySize.x * WINDOW_ASPECT;
            if (displaySize.y > hMax)
            {
                displaySize.y = hMax;
                displaySize.x = displaySize.y / WINDOW_ASPECT;
            }
            break;
        }
        }

        auto framebufferTex = m_glUtils.GetFramebufferTexture(m_vramMatId);
        ImGui::Image((void*)(intptr_t)framebufferTex, displaySize, borderMin, borderMax);
        //m_displayIsHovered = ImGui::IsItemHovered();
        */
        
        /*m_winGlUtilsP->DrawOnWindow(m_hwnd_temp, m_vramMatId, 
            _viewportW, _viewportH,
            _viewportW, _viewportH);*/

        /*if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(m_contextMenuName);
        }*/
    }
}