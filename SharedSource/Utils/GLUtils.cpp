#include "GLUtils.h"

#include <format>
#include "GL/glew.h"

// vertices of a quad with UV coordinates
GLfloat vertices[] = {
    // Positions          // UV Coords
    /*
     -1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
     -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
      1.0f,  1.0f, 0.0f,  0.0f, 0.0f,
      1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
      */
     -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
     -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,
      1.0f,  1.0f, 0.0f,  1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
};


// Vertex shader source code
const char* vertexShaderSource = R"(
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
const char* fragmentShaderSource = R"(
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

dev::GLUtils::GLUtils(Hardware& _hardware)
    :
    m_hardware(_hardware), m_frameSizeW(FRAME_BUFFER_W), m_frameSizeH(FRAME_BUFFER_H)
{
    Init();
}

void dev::GLUtils::Update()
{
    CreateRamTextures();
    DrawDisplay();
}

void dev::GLUtils::DrawDisplay()
{

    if (IsShaderDataReady())
        for (int i = 0; i < RAM_TEXTURES; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_shaderData.framebuffers[i]);
            glViewport(0, 0, m_frameSizeW, m_frameSizeH);
            glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render the quad
            glUseProgram(m_shaderData.shaderProgram);

            // send the color
            glUniform4f(m_shaderData.globalColorBgId, 0.2f, 0.2f, 0.2f, 1.0f);
            glUniform4f(m_shaderData.globalColorFgId, 1.0f, 1.0f, 1.0f, 1.0f);

            // assign a texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_shaderData.ramTextures[i]);

            glBindVertexArray(m_shaderData.vtxArrayObj);
            glDrawArrays(GL_QUADS, 0, 4);
            glBindVertexArray(0);
        
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
}

void dev::GLUtils::CreateRamTextures()
{
    auto memP = (m_hardware.GetRam()->data());
    int imageSize = RAM_TEXTURE_W * RAM_TEXTURE_H;

    for (int i = 0; i < RAM_TEXTURES; i++)
    {
        glBindTexture(GL_TEXTURE_2D, m_shaderData.ramTextures[i]);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, RAM_TEXTURE_W, RAM_TEXTURE_H, 0, GL_RED, GL_UNSIGNED_BYTE, memP + i * imageSize);
    }
}


// it is not initializing the Window and OpenGL 3.3 context
// assumming ImGui and did it already
GLenum dev::GLUtils::Init()
{
    auto glewInitCode = glewInit();
    if (glewInitCode != GLEW_OK) {
        dev::Log("Failed to initialize GLEW");
        return glewInitCode;
    }

    // texture init
    // Create a OpenGL texture identifiers
    glGenTextures(RAM_TEXTURES, m_shaderData.ramTextures);
    CreateRamTextures();

    // Create Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
    glGenVertexArrays(1, &m_shaderData.vtxArrayObj);
    glGenBuffers(1, &m_shaderData.vtxBufferObj);
    glBindVertexArray(m_shaderData.vtxArrayObj);
    glBindBuffer(GL_ARRAY_BUFFER, m_shaderData.vtxBufferObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Create and bind a framebuffer object (FBO)
    glGenFramebuffers(RAM_TEXTURES, m_shaderData.framebuffers);
    // Create a texture to render to
    glGenTextures(RAM_TEXTURES, m_shaderData.framebufferTextures);

    for (int i = 0; i < RAM_TEXTURES; i++) 
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_shaderData.framebuffers[i]);

        glBindTexture(GL_TEXTURE_2D, m_shaderData.framebufferTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_frameSizeW, m_frameSizeH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_shaderData.framebufferTextures[i], 0);

        // Check framebuffer status
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            dev::Log("Framebuffer is not complete!");
        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Create shader program
    m_shaderData.shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    // get uniform vars ids
    m_shaderData.globalColorBgId = glGetUniformLocation(m_shaderData.shaderProgram, "globalColorBg");
    m_shaderData.globalColorFgId = glGetUniformLocation(m_shaderData.shaderProgram, "globalColorFg");
    
    // assign a texture
    glUseProgram(m_shaderData.shaderProgram);
    glUniform1i(glGetUniformLocation(m_shaderData.shaderProgram, "texture0"), 0);

    return IsShaderDataReady() ? glewInitCode : -1;
}

dev::GLUtils::~GLUtils()
{
    // Clean up
    glDeleteFramebuffers(RAM_TEXTURES, m_shaderData.framebuffers);
    glDeleteTextures(RAM_TEXTURES, m_shaderData.ramTextures);
    glDeleteTextures(RAM_TEXTURES, m_shaderData.framebufferTextures);
    glDeleteVertexArrays(1, &m_shaderData.vtxArrayObj);
    glDeleteBuffers(1, &m_shaderData.vtxBufferObj);

    // Delete shader program
    glDeleteProgram(m_shaderData.shaderProgram);
}

GLuint dev::GLUtils::GLCheckError(GLuint1 _obj, const std::string& _txt)
{
    // Check for compilation errors
    GLint success;
    glGetShaderiv(_obj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(_obj, 512, NULL, infoLog);
        dev::Log("{}:\n {}", _txt, std::string(infoLog));
        return 0;
    }
    return _obj;
}

GLuint dev::GLUtils::CompileShader(GLenum _shaderType, const char* _source) 
{
    GLuint shader = glCreateShader(_shaderType);
    glShaderSource(shader, 1, &_source, NULL);
    glCompileShader(shader);

    return GLCheckError(shader, std::format("Shader compilation failed:\n {}", _source));
}

GLuint dev::GLUtils::CreateShaderProgram(const char* _vertexShaderSource, const char* _fragmentShaderSource)
{
    // Compile vertex and fragment shaders
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, _vertexShaderSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, _fragmentShaderSource);

    // Create shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    shaderProgram = GLCheckError(shaderProgram, "Shader program linking failed:\n");

    // Delete shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

auto dev::GLUtils::GetShaderData() 
-> const ShaderData*
{
    return &m_shaderData;
}

auto dev::GLUtils::IsShaderDataReady()
-> const bool
{
    return //m_shaderData.framebuffer && m_shaderData.framebufferTexture &&
        m_shaderData.shaderProgram && //m_shaderData.ramMainTexture && 
        m_shaderData.vtxArrayObj && m_shaderData.vtxBufferObj;
}