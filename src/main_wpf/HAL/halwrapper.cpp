#include "halwrapper.h"
#include "utils\str_utils.h"
#include "utils\tqueue.h"
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

bool dev::HAL::CreateGfxContext(System::IntPtr _hWnd, GLsizei _viewportW, GLsizei _viewportH)
{
	auto hWnd = static_cast<HWND>(_hWnd.ToPointer());
	auto glContextStatus = m_winGlUtilsP->CreateGfxContext(hWnd, _viewportW, _viewportH);
	m_glInited = glContextStatus == WinGlUtils::Status::INITED;

	return m_glInited;
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
}

auto dev::HAL::InitShader(System::IntPtr _hWnd, 
		System::String^ _vtxShaderS, System::String^ _fragShaderS) 
-> Id
{
	auto hWnd = static_cast<HWND>(_hWnd.ToPointer());
	std::string vtxShaderS = msclr::interop::marshal_as<std::string>(_vtxShaderS);
	std::string fragShaderS = msclr::interop::marshal_as<std::string>(_fragShaderS);

	return m_winGlUtilsP->InitShader(hWnd, vtxShaderS.c_str(), fragShaderS.c_str());
}

auto dev::HAL::InitMaterial(System::IntPtr _hWnd, const Id _shaderId, 
		const GLUtils::TextureIds& _textureIds, const GLUtils::ShaderParams& _shaderParams, 
		const int _framebufferW, const int _framebufferH) 
-> Id
{
	auto hWnd = static_cast<HWND>(_hWnd.ToPointer());
	return m_winGlUtilsP->InitMaterial(hWnd, _shaderId,
		_textureIds, _shaderParams,
		_framebufferW, _framebufferH, false);
}

auto dev::HAL::InitTexture(System::IntPtr _hWnd, GLsizei _w, GLsizei _h) -> Id
{
	auto hWnd = static_cast<HWND>(_hWnd.ToPointer());
	return m_winGlUtilsP->InitTexture(hWnd, _w, _h);
}

auto dev::HAL::Request(const Req _req, System::String^ _dataS)
-> System::String^
{
	auto dataS = msclr::interop::marshal_as<std::string>(_dataS);
	auto dataJ = dataS.empty() ? nlohmann::json() : nlohmann::json::parse(dataS);

	auto req = m_hardwareP->Request(static_cast<Hardware::Req>(_req), dataJ);
	if (!req) return System::String::Empty;

	return msclr::interop::marshal_as<System::String^>(req->dump());
}

auto dev::HAL::GetCC()
-> uint64_t
{
	auto res = m_hardwareP->Request(Hardware::Req::GET_CC);
	const auto& data = *res;

	return data["cc"];
}

void dev::HAL::Run()
{
	m_hardwareP->Request(Hardware::Req::RUN);
}

/*
// Vertex shader source code
const char* vtxShaderS = R"#(
	#version 330 core
	precision highp float;

	layout (location = 0) in vec3 pos;
	layout (location = 1) in vec2 uv;

	out vec2 uv0;

	void main()
	{
		uv0 = uv;
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

*/

//bool dev::HAL::DisplayWindowInit(System::IntPtr _hWnd, const GLsizei _viewportW, const GLsizei _viewportH)
//{
//
//
//	m_vramShaderId = m_winGlUtilsP->InitShader(m_hWnd_temp, vtxShaderS, fragShaderS);
//	if (m_vramShaderId == dev::INVALID_ID ) return false;
//
//	m_vramTexId = m_winGlUtilsP->InitTexture(m_hWnd_temp, Display::FRAME_W, Display::FRAME_H, GLUtils::Texture::Format::RGBA);
//	if (m_vramTexId == INVALID_ID) return false;
//
//
//	int borderLeft = m_hardwareP->Request(Hardware::Req::GET_DISPLAY_BORDER_LEFT)->at("borderLeft");
//
//	GLUtils::ShaderParams shaderParams = {
//		{ "m_activeArea_pxlSize", { Display::ACTIVE_AREA_W, Display::ACTIVE_AREA_H, FRAME_PXL_SIZE_W, FRAME_PXL_SIZE_H }},
//		{ "m_scrollV_crtXY_highlightMul", { 255.0f * FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f } },
//		{ "m_bordsLRTB", { 
//					borderLeft * FRAME_PXL_SIZE_W, 
//					(borderLeft + Display::ACTIVE_AREA_W) * FRAME_PXL_SIZE_W,
//					static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP * FRAME_PXL_SIZE_H),
//					static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP + Display::ACTIVE_AREA_H)* FRAME_PXL_SIZE_H  } }
//	};
//	
//	m_vramMatId = m_winGlUtilsP->InitMaterial(
//		m_hWnd_temp, m_vramShaderId, { m_vramTexId }, shaderParams,
//		Display::FRAME_W, Display::FRAME_H, false);
//
//	if (m_vramMatId == INVALID_ID) return false;
//
//	return true;
//}


//void dev::HAL::UpdateData(const bool _isRunning,
//	const GLsizei _viewportW, const GLsizei _viewportH)
//{
//	uint64_t cc = m_hardwareP->Request(Hardware::Req::GET_CC)->at("cc");
//	auto ccDiff = cc - m_ccLast;
//	m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
//	m_ccLast = cc;
//
//	//m_scrollV_crtXY_highlightMulP->w = 1.0f;
//
//	if (!_isRunning)
//	{
//		if (ccDiff)
//		{
//			auto res = m_hardwareP->Request(Hardware::Req::GET_DISPLAY_DATA);
//			const auto& displayData = *res;
//			m_rasterPixel = displayData["rasterPixel"];
//			m_rasterLine = displayData["rasterLine"];
//		}
//		if (!m_displayIsHovered)
//		{
//			//m_scrollV_crtXY_highlightMulP->y = (float)m_rasterPixel * FRAME_PXL_SIZE_W;
//			//m_scrollV_crtXY_highlightMulP->z = (float)m_rasterLine * FRAME_PXL_SIZE_H;
//			//m_scrollV_crtXY_highlightMulP->w = SCANLINE_HIGHLIGHT_MUL;
//		}
//	}
//
//	// update
//	if (m_isGLInited)
//	{
//		//uint8_t scrollVert = (m_hardware.Request(Hardware::Req::GET_SCROLL_VERT)->at("scrollVert") + 1; // adding +1 offset because the default is 255
//		//m_scrollV_crtXY_highlightMulP->x = 0;//FRAME_PXL_SIZE_H * scrollVert;
//
//		auto frameP = m_hardwareP->GetFrame(_isRunning);
//		m_winGlUtilsP->UpdateTexture(m_hWnd_temp, m_vramTexId, (uint8_t*)frameP->data());        
//	}
//}

//void dev::HAL::DrawDisplay(const GLsizei _viewportW, const GLsizei _viewportH)
//{
//	if (m_isGLInited)
//	{
/*
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
		
//m_winGlUtilsP->Draw(m_hWnd_temp, m_vramMatId,
//	_viewportW, _viewportH);

		/*if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup(m_contextMenuName);
		}*/
//	}
//}