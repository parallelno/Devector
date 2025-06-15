#include "ui/display_window.h"
#include "utils/imgui_utils.h"

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

dev::DisplayWindow::DisplayWindow(Hardware& _hardware,
	const float* const _dpiScaleP, GLUtils& _glUtils, 
	ReqUI& _reqUI, Scripts& _scripts,
	const Hardware::ExecSpeed _execSpeed)
	:
	BaseWindow("Display", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _dpiScaleP),
	m_hardware(_hardware), m_glUtils(_glUtils), m_reqUI(_reqUI), m_scripts(_scripts)
{
	m_isGLInited = Init();
	SetExecutionSpeed(_execSpeed);
}

bool dev::DisplayWindow::Init()
{
	// init shader
	auto vramShaderId = m_glUtils.InitShader(vtxShaderS, fragShaderS);
	if (vramShaderId == INVALID_ID) return false;
	m_vramShaderId = vramShaderId;

	// init texture
	auto vramTexId = m_glUtils.InitTexture(Display::FRAME_W, Display::FRAME_H, GLUtils::Texture::Format::RGBA);
	if (vramTexId == INVALID_ID) return false;
	m_vramTexId = vramTexId;

	// shader params
	int borderLeft = m_hardware.Request(Hardware::Req::GET_DISPLAY_BORDER_LEFT)->at("borderLeft");
	m_bordsLRTB.x = borderLeft * FRAME_PXL_SIZE_W;
	m_bordsLRTB.y = (borderLeft + Display::ACTIVE_AREA_W) * FRAME_PXL_SIZE_W;
	GLUtils::ShaderParams shaderParams = {
		{ "m_activeArea_pxlSize", m_activeArea_pxlSize },
		{ "m_scrollV_crtXY_highlightMul", m_scrollV_crtXY_highlightMul },
		{ "m_bordsLRTB", m_bordsLRTB }
	};

	// init material
	auto vramMatId = m_glUtils.InitMaterial(m_vramShaderId,
			{m_vramTexId}, shaderParams,
			Display::FRAME_W, Display::FRAME_H);
	if (vramMatId == INVALID_ID) return false;
	m_vramMatId = vramMatId;

	// get param ids
	m_matParamId_scrollV_crtXY_highlightMul = m_glUtils.GetMaterialParamId(vramMatId, "m_scrollV_crtXY_highlightMul");
	m_matParamId_activeArea_pxlSize = m_glUtils.GetMaterialParamId(vramMatId, "m_activeArea_pxlSize");
	m_matParamId_bordsLRTB = m_glUtils.GetMaterialParamId(vramMatId, "m_bordsLRTB");


	return true;
}

void dev::DisplayWindow::Update(bool& _visible, const bool _isRunning)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
/*
		// TODO: DEBUG test to tune the display settings
		int paletteCommitTime = m_hardware.Request(Hardware::Req::GET_IO_PALETTE_COMMIT_TIME)->at("paletteCommitTime");
		if (ImGui::InputInt("paletteCommitTime", &paletteCommitTime, 1, 2000) )
		{
			m_hardware.Request(Hardware::Req::SET_IO_PALETTE_COMMIT_TIME, { {"paletteCommitTime", paletteCommitTime}});
		};
		
		int borderLeft = m_hardware.Request(Hardware::Req::GET_DISPLAY_BORDER_LEFT)->at("borderLeft");
		if (ImGui::InputInt("Border Left", &borderLeft, 1, 2000))
		{
			m_hardware.Request(Hardware::Req::SET_DISPLAY_BORDER_LEFT, { {"borderLeft", borderLeft} });

			m_bordsLRTB.x = borderLeft * FRAME_PXL_SIZE_W;
			m_bordsLRTB.y = (borderLeft + Display::ACTIVE_AREA_W) * FRAME_PXL_SIZE_W;
		};

		int irqCommitPxl = m_hardware.Request(Hardware::Req::GET_DISPLAY_IRQ_COMMIT_PXL)->at("irqCommitPxl");
		if (ImGui::InputInt("irqCommitPxl", &irqCommitPxl, 1, 2000))
		{
			m_hardware.Request(Hardware::Req::SET_DISPLAY_IRQ_COMMIT_PXL, { {"irqCommitPxl", irqCommitPxl} });
		};
		// TODO: DEBUG test end
*/
		m_windowFocused = ImGui::IsWindowFocused();
		
		// switch the border type
		if (ImGui::IsKeyPressed(ImGuiKey_B) && ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) {
			m_borderType = static_cast<BorderType>((static_cast<int>(m_borderType) + 1) % static_cast<int>(BorderType::LEN));
			DrawTooltipTimer(m_borderTypeAS[(int)(m_borderType)]);
		}
		// switch the display size
		if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) {
			m_displaySize = static_cast<DisplaySize>((static_cast<int>(m_displaySize) + 1) % static_cast<int>(DisplaySize::LEN));
			DrawTooltipTimer(m_displaySizeAS[(int)(m_displaySize)]);
		}

		UpdateData(_isRunning);
		DrawContextMenu();
		DrawDisplay();
		DrawTooltipTimer();

		ImGui::End();
	}
}

bool dev::DisplayWindow::IsFocused() const
{
	return m_windowFocused;
}

void dev::DisplayWindow::UpdateData(const bool _isRunning)
{
	//ReqHandling();

	uint64_t cc = m_hardware.Request(Hardware::Req::GET_CC)->at("cc");
	auto ccDiff = cc - m_ccLast;
	m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
	m_ccLast = cc;

	m_scrollV_crtXY_highlightMul.w = 1.0f;

	if (!_isRunning)
	{
		if (ccDiff) 
		{
			auto res = m_hardware.Request(Hardware::Req::GET_DISPLAY_DATA);
			const auto& displayData = *res;
			m_rasterPixel = displayData["rasterPixel"];
			m_rasterLine = displayData["rasterLine"];
		}
		if (!m_displayIsHovered)
		{
			m_scrollV_crtXY_highlightMul.y = (float)m_rasterPixel * FRAME_PXL_SIZE_W;
			m_scrollV_crtXY_highlightMul.z = (float)m_rasterLine * FRAME_PXL_SIZE_H;
			m_scrollV_crtXY_highlightMul.w = SCANLINE_HIGHLIGHT_MUL;
		}
	}

	// update
	if (m_isGLInited)
	{
		//uint8_t scrollVert = (m_hardware.Request(Hardware::Req::GET_SCROLL_VERT)->at("scrollVert") + 1; // adding +1 offset because the default is 255
		m_scrollV_crtXY_highlightMul.x = 0;//FRAME_PXL_SIZE_H* scrollVert;

		// update params
		m_glUtils.UpdateMaterialParam(m_vramMatId, m_matParamId_scrollV_crtXY_highlightMul, m_scrollV_crtXY_highlightMul);
		m_glUtils.UpdateMaterialParam(m_vramMatId, m_matParamId_bordsLRTB, m_bordsLRTB);
		m_glUtils.UpdateMaterialParam(m_vramMatId, m_matParamId_activeArea_pxlSize, m_activeArea_pxlSize);

		auto frameP = m_hardware.GetFrame(_isRunning);
		m_glUtils.UpdateTexture(m_vramTexId, (uint8_t*)frameP->data());
		m_glUtils.Draw(m_vramMatId);
	}
}

void dev::DisplayWindow::DrawDisplay()
{
	if (m_isGLInited)
	{
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
			int borderLeft = m_hardware.Request(Hardware::Req::GET_DISPLAY_BORDER_LEFT)->at("borderLeft");

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

		
		auto pos = ImGui::GetCursorPos();

		auto framebufferTex = m_glUtils.GetFramebufferTexture(m_vramMatId);
		ImGui::Image(framebufferTex, displaySize, borderMin, borderMax);
		m_displayIsHovered = ImGui::IsItemHovered();
		
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup(m_contextMenuName);
		}

		// Draw script objects
		DrawScriptsUIItems(pos, displaySize);
				
	}
}

void dev::DisplayWindow::DrawScriptsUIItems(
	const ImVec2& _pos, const ImVec2& _displaySize)
{
	float v6visibleBorderL = 0; // in Vector pixels
	float v6visibleBorderR = 0;
	float v6visibleBorderT = 0;
	float v6visibleBorderB = 0;
	
	switch (m_borderType)
	{
		case dev::DisplayWindow::BorderType::FULL:
			v6visibleBorderL = Display::BORDER_LEFT / 2.0f;
			v6visibleBorderR = Display::BORDER_RIGHT / 2.0f;
			v6visibleBorderT = Display::BORDER_TOP;
			v6visibleBorderB = Display::BORDER_BOTTOM;
			break;

		case dev::DisplayWindow::BorderType::NORMAL:
			v6visibleBorderL = v6visibleBorderR = v6visibleBorderT = v6visibleBorderB = Display::BORDER_VISIBLE;
			break;
	}
	float v6activeAreaW = Display::ACTIVE_AREA_W / 2.0f + v6visibleBorderL + v6visibleBorderR;
	float v6activeAreaH = Display::ACTIVE_AREA_H + v6visibleBorderT + v6visibleBorderB;

	// convert Vector pixels to ImGui pixels
	float v6pixelSizeX = _displaySize.x / v6activeAreaW;
	float v6pixelSizeY = _displaySize.y / v6activeAreaH;

	float posLeftTopX = _pos.x + v6visibleBorderL * v6pixelSizeX;
	float posLeftTopY = _pos.y + v6visibleBorderT * v6pixelSizeY;

	float posRightBottomX = _pos.x + _displaySize.x - v6visibleBorderR * v6pixelSizeX;
	float posRightBottomY = _pos.y + _displaySize.y - v6visibleBorderB * v6pixelSizeY;

	

	for (auto const& [id, uiItem] : m_scripts.GetUIItems())
	{
		float posX = 0;
		float posY = 0;
		float sizeX = 0;
		float sizeY = 0;
		if (uiItem.vectorScreenCoords)
		{	
			posX = uiItem.x * v6pixelSizeX;
			posY = -uiItem.y * v6pixelSizeY;
	
			posX += uiItem.x >= 0 ? posLeftTopX : posRightBottomX;
			posY += uiItem.y < 0 ? posLeftTopY : posRightBottomY;
	
			sizeX = uiItem.width * v6pixelSizeX;
			sizeY = -uiItem.height * v6pixelSizeY;
		}
		else {
			posX = uiItem.x;
			posY = uiItem.y;

			posX += uiItem.x >= 0 ? _pos.x : _pos.x + _displaySize.x;
			posY += uiItem.x >= 0 ? _pos.y : _pos.y + _displaySize.y;

			sizeX = uiItem.width;
			sizeY = uiItem.height;
		}

		ImVec2 pos = { posX, posY };

		switch (uiItem.type)
		{
			case dev::Scripts::UIType::TEXT:
			{
				ImGui::SetCursorPos( pos );
				ImGui::PushStyleColor(ImGuiCol_Text, dev::IM_VEC4(uiItem.color));
				ImGui::Text(uiItem.text.c_str());
				ImGui::PopStyleColor();		
				break;
			}
			case dev::Scripts::UIType::RECT:
			{	
				ImVec2 screenPos = dev::CursorPosToScreenPos(pos);
				ImGui::GetWindowDrawList()->AddRect(
					screenPos,
					{screenPos.x + sizeX, screenPos.y + sizeY},
					dev::IM_U32(uiItem.color));
				break;
			}
			
			case dev::Scripts::UIType::RECT_FILLED:
			{
				ImVec2 screenPos = dev::CursorPosToScreenPos(pos);
				ImGui::GetWindowDrawList()->AddRectFilled(
					screenPos,
					{screenPos.x + sizeX, screenPos.y + sizeY},
					dev::IM_U32(uiItem.color));	
				break;
			}
		}
	}
}


void dev::DisplayWindow::DrawContextMenu()
{
	if (ImGui::BeginPopup(m_contextMenuName))
	{
		if (ImGui::BeginMenu("Display Settings"))
		{
			ImGui::Combo("Border Type", (int*)(&m_borderType), m_borderTypeS);
			ImGui::Combo("Display Size", (int*)(&m_displaySize), m_displaySizeS);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Emulation Settings"))
		{
			if (ImGui::Combo("Emulation Speed", (int*)(&m_execSpeed), m_execSpeedsS)) 
			{
				SetExecutionSpeed(m_execSpeed);
			};
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::Text(
				"Left Ctrl + S - set the display size\n"
				"Left Ctrl + B - set the border\n"
			);
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
}
/*
void dev::DisplayWindow::ReqHandling()
{
	if (m_reqUI.type == ReqUI::Type::NONE) return;

	switch (m_reqUI.type)
	{
	case ReqUI::Type::DISPLAY_FRAME_BUFF_UPDATE: {
		m_reqUI.type = ReqUI::Type::NONE;
		//
	}
	}
}
*/

void dev::DisplayWindow::SetExecutionSpeed(const Hardware::ExecSpeed _execSpeed)
{ 
	m_execSpeed = _execSpeed;
	m_hardware.Request(Hardware::Req::SET_CPU_SPEED, { {"speed", int(m_execSpeed)} }); 
};