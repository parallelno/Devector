#include "RamViewWindow.h"

#include "imgui.h"
#include "imgui_impl_opengl3_loader.h"

dev::RamViewWindow::RamViewWindow(Hardware& _hardware,
        const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP),
    m_hardware(_hardware), m_shaderData()
{
    UpdateData(false);
    Init();
}

void dev::RamViewWindow::Update()
{
	BaseWindow::Update();

	static bool open = true;
	ImGui::Begin("Ram View", &open, ImGuiWindowFlags_NoCollapse);

    bool isRunning = m_hardware.Request(Hardware::Req::IS_RUNNING)->at("isRunning");
    UpdateData(isRunning);

	DrawDisplay();

	ImGui::End();
}

void dev::RamViewWindow::DrawDisplay()
{

    if (m_shaderData.frameTextureId && m_shaderData.shaderProgram)
    {       
        // Draw ImGui image using a custom shader
        ImVec2 imageSize(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H);
        ImGui::Image((void*)(intptr_t)m_shaderData.frameTextureId, imageSize);
        
        ImVec2 offset{0, 0};
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        p0.x += offset.x;
        p0.y += offset.y;
        ImVec2 p1 = ImVec2(p0.x + imageSize.x, p0.y + imageSize.y);


        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddCallback([](const ImDrawList* _draw_list, const ImDrawCmd* _curr_cmd)
            {
                auto shaderDataP = (ShaderData*)(_curr_cmd->UserCallbackData);
                auto shaderProgram = shaderDataP->shaderProgram;
                auto textureId = shaderDataP->frameTextureId;

                ImDrawData* draw_data = ImGui::GetDrawData();

                float L = draw_data->DisplayPos.x;
                float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
                float T = draw_data->DisplayPos.y;
                float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

                const float ortho_projection[4][4] =
                {
                   { 2.0f / (R - L),    0.0f,               0.0f,   0.0f },
                   { 0.0f,              2.0f / (T - B),     0.0f,   0.0f },
                   { 0.0f,              0.0f,               -1.0f,  0.0f },
                   { (R + L) / (L - R), (T + B) / (B - T),  0.0f,   1.0f },
                };

                glUseProgram(shaderProgram); // If I remove this line, it works
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "ProjMtx"), 1, GL_FALSE, &ortho_projection[0][0]);
                

                //glBindTexture(GL_TEXTURE_2D, textureId);
                // Load texture data
                //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
                // Bind the texture unit to a texture sampler uniform in your shader
                glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), textureId);
                // Activate texture unit 0
                glActiveTexture(GL_TEXTURE0);
                // Bind the texture
                glBindTexture(GL_TEXTURE_2D, textureId);

            }, &m_shaderData);

        draw_list->AddRectFilled(p0, p1, 0xFFFF00FF);
        draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }
}

#define GL_RED                            0x1903

// creates a textre
void dev::RamViewWindow::CreateTexture(const bool _vsync)
{
    auto ramP = m_hardware.GetRam8K(0);

    // Create a OpenGL texture identifier
    if (!m_shaderData.frameTextureId)
    {
        glGenTextures(1, &m_shaderData.frameTextureId);
    }
    glBindTexture(GL_TEXTURE_2D, m_shaderData.frameTextureId);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Display::FRAME_W, Display::FRAME_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, ram.data());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 256, 32, 0, GL_RED, GL_UNSIGNED_BYTE, ramP->data());
}

void dev::RamViewWindow::UpdateData(const bool _isRunning)
{
    if (!_isRunning) return;

    auto res = m_hardware.Request(Hardware::Req::GET_REGS);
    const auto& data = *res;

    uint64_t cc = data["cc"];
    auto ccDiff = cc - m_ccLast;
    m_ccLastRun = ccDiff == 0 ? m_ccLastRun : ccDiff;
    m_ccLast = cc;
    if (ccDiff == 0) return;

    // update
    CreateTexture(true);
}

// Vertex shader source code
const char* vertexShaderSource = R"(
    #version 330 core
    precision mediump float;
    layout (location = 0) in vec2 Position;
    layout (location = 1) in vec2 UV;
    layout (location = 2) in vec4 Color;

    uniform mat4 ProjMtx;
    out vec2 Frag_UV;
    out vec4 Frag_Color;
    out vec2 Frag_Pos;

    void main()
    {
        Frag_UV = UV;
        Frag_Color = Color;
        gl_Position = ProjMtx * vec4(Position.xy,0,1);

        Frag_Pos = gl_Position.xy * 1.0;
    }
)";

// Fragment shader source code
const char* fragmentShaderSource = R"(
    #version 330 core
    precision mediump float;
    layout (location = 0) out vec4 Out_Color;

    uniform sampler2D texture1;
    in vec2 Frag_UV;
    in vec4 Frag_Color;
    in vec2 Frag_Pos;

    void main()
    {
        //Out_Color = Frag_Color;
        float x = texture(texture1, Frag_Pos).r;
        Out_Color = vec4(Frag_Pos.x, Frag_Pos.y, 0.0, 1.0);
    }
)";
/*
// Vertex shader source code
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

// Fragment shader source code
const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D texture1;
    void main()
    {
        FragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);//texture(texture1, TexCoord);
    }
)";
*/

// Function to compile shader
GLuint compileShader(GLenum shaderType, const char* source) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }

    return shader;
}

// Function to create shader program
GLuint createShaderProgram() {
    // Compile vertex and fragment shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // Create shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader program linking failed:\n" << infoLog << std::endl;
        return 0;
    }

    // Delete shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void dev::RamViewWindow::Init()
{
    CreateTexture(true);

    // Create shader program
    m_shaderData.shaderProgram = createShaderProgram();
}