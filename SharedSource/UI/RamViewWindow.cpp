#include "RamViewWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_opengl3_loader.h"

// Vertex shader source code
const char* rvShaderVtx = R"(
    #version 330 core
    precision highp float;
    
    layout (location = 0) in vec3 vtxPos;
    layout (location = 1) in vec2 vtxUV;

    uniform vec4 globalColorBg;
    uniform vec4 globalColorFg;
    
    out vec2 uv0;
    out vec4 globalColorBg0;
    out vec4 globalColorFg0;

    void main()
    {
        uv0 = vtxUV;
        globalColorBg0 = globalColorBg;
        globalColorFg0 = globalColorFg;
        gl_Position = vec4(vtxPos.xyz, 1.0f);
    }
)";

// Fragment shader source code
const char* rvShaderFrag = R"(
    #version 330 core
    precision highp float;

    in vec2 uv0;
    in vec4 globalColorBg0;
    in vec4 globalColorFg0;

    uniform sampler2D texture0;
    uniform ivec2 iresolution;

    layout (location = 0) out vec4 out0;

    #define BYTE_COLOR_MULL 0.6
    #define BACK_COLOR_MULL 0.7

    int GetBit(float _color, int _bitIdx) {
        return (int(_color * 255.0) >> _bitIdx) & 1;
    }

    void main()
    {
        float isAddrBelow32K = 1.0 - step(0.5, uv0.y);
        vec2 uv = vec2( uv0.y * 2.0, uv0.x / 2.0 + isAddrBelow32K * 0.5);
        float byte = texture(texture0, uv).r;

        float isOdd8K = step(0.5, fract(uv0.x / 0.5));
        isOdd8K = mix(isOdd8K, 1.0 - isOdd8K, isAddrBelow32K);
        vec3 bgColor = mix(globalColorBg0.xyz, globalColorBg0.xyz * BACK_COLOR_MULL, isOdd8K);

        int bitIdx = 7 - int(uv0.x * 1023.0) & 7;
        int isBitOn = GetBit(byte, bitIdx);

        int isByteOdd = (int(uv0.x * 511.0)>>3) & 1;
        vec3 byteColor = mix(globalColorFg0.xyz * BYTE_COLOR_MULL, globalColorFg0.xyz, float(isByteOdd));
        vec3 color = mix(bgColor, byteColor, float(isBitOn));

        out0 = vec4(color, globalColorBg0.a);
        //out0 = vec4(byte,byte,byte, globalColorBg0.a);
    }
)";

dev::RamViewWindow::RamViewWindow(Hardware& _hardware,
        const float* const _fontSizeP, const float* const _dpiScaleP, GLUtils& _glUtils)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
    m_hardware(_hardware), m_glUtils(_glUtils)
{
    auto globalColorBg = GLUtils::Vec4(0.2f, 0.2f, 0.2f, 1.0f);
    auto globalColorFg = GLUtils::Vec4(1.0f, 1.0f, 1.0f, 1.0f);

    m_shaderParamData = {
        { "globalColorBg", globalColorBg },
        { "globalColorFg", globalColorFg } };
    m_renderDataIdx = m_glUtils.InitRenderData(rvShaderVtx, rvShaderFrag, FRAME_BUFFER_W, FRAME_BUFFER_H, { "globalColorBg", "globalColorFg" }, RAM_TEXTURES);
}

void dev::RamViewWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Ram View", &open, ImGuiWindowFlags_NoCollapse);

    bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
    UpdateData(isRunning);

	DrawDisplay();

    ScaleView();
	ImGui::End();
}

void dev::RamViewWindow::DrawDisplay()
{

    if (m_renderDataIdx >= 0 && m_glUtils.IsShaderDataReady(m_renderDataIdx))
    {   
        auto& framebufferTextures = m_glUtils.GetFramebufferTextures(m_renderDataIdx);

        ImVec2 imageSize(FRAME_BUFFER_W * m_scale, FRAME_BUFFER_H * m_scale);

        ImGui::SeparatorText("The Main Ram");
        ImGui::Image((void*)(intptr_t)framebufferTextures[0], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 0");
        ImGui::Image((void*)(intptr_t)framebufferTextures[1], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 1");
        ImGui::Image((void*)(intptr_t)framebufferTextures[2], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 2");
        ImGui::Image((void*)(intptr_t)framebufferTextures[3], imageSize);
        
        ImGui::SeparatorText("The Ram-Disk Page 3");
        ImGui::Image((void*)(intptr_t)framebufferTextures[4], imageSize);
    }
}

void dev::RamViewWindow::UpdateData(const bool _isRunning)
{
    // check if the hardware updated its state
    auto res = m_hardware.Request(Hardware::Req::GET_REGS);
    const auto& data = *res;

    uint64_t cc = data["cc"];
    auto ccDiff = cc - m_ccLast;
    m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
    m_ccLast = cc;
    if (ccDiff == 0) return;

    // update
    if (m_renderDataIdx >= 0) 
    {
        auto memP = m_hardware.GetRam()->data();
        m_glUtils.UpdateTextures(m_renderDataIdx, memP, RAM_TEXTURE_W, RAM_TEXTURE_H, 1);
        m_glUtils.Draw(m_renderDataIdx, m_shaderParamData);
    }
}

// check the keys, scale the view
void dev::RamViewWindow::ScaleView()
{
    if (ImGui::IsWindowHovered())
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            float scaleAdjusted = m_scale < 1.0f ? SCALE_INC : m_scale * SCALE_INC;

            if (ImGui::GetIO().MouseWheel > 0.0f)
            {
                m_scale = m_scale > SCALE_MAX ? SCALE_MAX : m_scale + scaleAdjusted;
            }
            else if (ImGui::GetIO().MouseWheel < 0.0f)
            {
                m_scale = m_scale < SCALE_MIN ? SCALE_MIN : m_scale - scaleAdjusted;
            }
        }
    }
}