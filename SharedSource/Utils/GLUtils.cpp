#include "GLUtils.h"

#include "Result.h"
#include <format>
#include "GL/glew.h"

// vertices of a quad with UV coordinates
GLfloat vertices[] = {
    // Positions          // UV Coords
     -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
     -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,
      1.0f,  1.0f, 0.0f,  1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
};

// it is not initializing the Window and OpenGL 3.3 context
// assumming ImGui and did it already

 dev::GLUtils::GLUtils()
 {
     m_glewInitCode = glewInit();
     if (m_glewInitCode != GLEW_OK) {
         dev::Log("Failed to initialize GLEW");
     }
 }

 auto dev::GLUtils::InitRenderData(const std::string& _vtxShaderS, const std::string& _fragShaderS,
     const int _framebufferW, const int _framebufferH, const ShaderParams& _paramParams, const int _textureCount, const int framebufferTextureFilter)
-> int
 {
     if (m_glewInitCode != GLEW_OK) return -1;

     auto& renderData = m_renderDatas.emplace_back(RenderData{ _textureCount });
     int renderDataIdx = m_renderDatas.size() - 1;

    // Create a OpenGL texture identifiers
    glGenTextures(_textureCount, renderData.textures.data());

    // Create Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
    glGenVertexArrays(1, &vtxArrayObj);
    glGenBuffers(1, &vtxBufferObj);
    glBindVertexArray(vtxArrayObj);
    glBindBuffer(GL_ARRAY_BUFFER, vtxBufferObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Create and bind a framebuffer object (FBO)
    glGenFramebuffers(_textureCount, renderData.framebuffers.data());
    // Create a framebuffer texture to render to
    glGenTextures(_textureCount, renderData.framebufferTextures.data());

    for (int i = 0; i < _textureCount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderData.framebuffers[i]);

        glBindTexture(GL_TEXTURE_2D, renderData.framebufferTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _framebufferW, _framebufferH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, framebufferTextureFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, framebufferTextureFilter);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderData.framebufferTextures[i], 0);

        // Check framebuffer status
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
        {
            dev::Log("Framebuffer is not complete!");
            return -1;
        }
        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    renderData.framebufferW = _framebufferW;
    renderData.framebufferH = _framebufferH;

    // Create shader program
    renderData.shaderProgram = CreateShaderProgram(_vtxShaderS.c_str(), _fragShaderS.c_str());

    // get uniform vars ids
    for (const auto& [name, val] : _paramParams)
    {
        auto paramId = glGetUniformLocation(renderData.shaderProgram, name.c_str());
        if (paramId < 0) continue;

        renderData.params[paramId] = val;
    }

    // assign a texture
    glUseProgram(renderData.shaderProgram);
    glUniform1i(glGetUniformLocation(renderData.shaderProgram, "texture0"), 0);

    if (!IsShaderDataReady(renderDataIdx)) return -1;

    return renderDataIdx;
}

dev::GLUtils::~GLUtils()
{
    for (const auto& renderData : m_renderDatas)
    {
        // Clean up
        glDeleteFramebuffers(renderData.textureCount, renderData.framebuffers.data());
        glDeleteTextures(renderData.textureCount, renderData.textures.data());
        glDeleteTextures(renderData.textureCount, renderData.framebufferTextures.data());

        // Delete shader program
        glDeleteProgram(renderData.shaderProgram);
    }

    glDeleteVertexArrays(1, &vtxArrayObj);
    glDeleteBuffers(1, &vtxBufferObj);
}

int dev::GLUtils::Draw(const int _renderDataIdx) const
{
    if (m_glewInitCode != GLEW_OK ||
        !IsShaderDataReady(_renderDataIdx)) return -1;

    auto& renderData = m_renderDatas.at(_renderDataIdx);

    for (int i = 0; i < renderData.textureCount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, renderData.framebuffers[i]);
        glViewport(0, 0, renderData.framebufferW, renderData.framebufferH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render the quad
        glUseProgram(renderData.shaderProgram);

        // send the color
        for (const auto& [paramId, paramValue] : renderData.params)
        {
            glUniform4f(paramId, paramValue->x, paramValue->y, paramValue->z, paramValue->w);
        }

        // assign a texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderData.textures[i]);

        glBindVertexArray(vtxArrayObj);
        glDrawArrays(GL_QUADS, 0, 4);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    return 0;
}

void dev::GLUtils::UpdateTextures(const int _renderDataIdx, const uint8_t* _memP, const int _width, const int _height, const int _colorDepth, const int textureFilter)
{
    if (_renderDataIdx >= m_renderDatas.size()) return;
    auto& renderData = m_renderDatas.at(_renderDataIdx);

    int imageSize = _width * _height * _colorDepth;

    for (int i = 0; i < renderData.textureCount; i++)
    {
        glBindTexture(GL_TEXTURE_2D, renderData.textures[i]);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        int glTexFormat;
        if (_colorDepth == 1) {
            glTexFormat = GL_RED;
        }
        else if (_colorDepth == 4) {
            glTexFormat = GL_RGBA;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, glTexFormat, _width, _height, 0, glTexFormat, GL_UNSIGNED_BYTE, _memP + i * imageSize);
    }
}

auto dev::GLUtils::GetFramebufferTextures(const int _renderDataIdx) const
-> const std::vector<GLuint>&
{
    return m_renderDatas.at(_renderDataIdx).framebufferTextures;
}

GLuint dev::GLUtils::GLCheckError(GLuint _obj, const std::string& _txt)
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

bool dev::GLUtils::IsShaderDataReady(const int _renderDataIdx) const
{
    auto& renderData = m_renderDatas.at(_renderDataIdx);

    bool ready = vtxArrayObj && vtxBufferObj && (renderData.shaderProgram >= 0);
    auto textureCount = renderData.framebuffers.size();

    for (int i = 0; i < textureCount; i++)
    {
        ready = ready && renderData.framebuffers[i] >= 0 &&
            renderData.framebufferTextures[i] >= 0 &&
            renderData.textures[i] >= 0;
    }
    return ready;
}