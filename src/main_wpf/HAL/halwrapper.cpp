#include "halwrapper.h"
#include <msclr\marshal_cppstd.h>

#include <glad/glad.h>

dev::HAL::HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
	const bool _ramDiskClearAfterRestart)
{
    std::wstring pathBootData = msclr::interop::marshal_as<std::wstring>(_pathBootData);
    std::wstring pathRamDiskData = msclr::interop::marshal_as<std::wstring>(_pathRamDiskData);

	m_hardwareP = new Hardware(pathBootData, pathRamDiskData, _ramDiskClearAfterRestart);
	m_debuggerP = new Debugger(*m_hardwareP);
}

void dev::HAL::Init()
{
	m_hardwareP->Request(Hardware::Req::RUN);
}

dev::HAL::~HAL()
{
    this->!HAL(); // Ensure finalizer is called
}

dev::HAL::!HAL()
{

    if (m_debuggerP)
    {
        delete m_debuggerP;
        m_debuggerP = nullptr;
    }

    if (m_hardwareP)
    {
        delete m_hardwareP;
        m_hardwareP = nullptr;
    }
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

//////////////////////////////////////////////////////////////////////////////////////
GLfloat vertices1[] = {
    // First triangle
    -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,  // bottom-left
    1.0f, -1.0f, 0.0f,   1.0f, 1.0f,  // bottom-right
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  // top-left

    // Second triangle
    -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  // top-left
    1.0f, -1.0f, 0.0f,   1.0f, 1.0f,  // bottom-right
    1.0f,  1.0f, 0.0f,   1.0f, 0.0f   // top-right
};


const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 position;
    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
    }
)";

// Create a fragment shader
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.0f, 0.5f, 1.0f, 1.0f); // orange color
    }
)";


void dev::HAL::RenderTextureOnHWND(HWND hWnd)
{
    // Assuming hWnd is the handle to the window
    HDC hdc = GetDC(hWnd);
    if (hdc == NULL) {
        MessageBox(hWnd, L"Failed to get device context", L"Error", MB_OK);
        return;
    }



    // Set the pixel format to a format compatible with OpenGL
    int pixelFormat;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0
    };
    pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0) {
        MessageBox(hWnd, L"Failed to choose pixel format", L"Error", MB_OK);
        ReleaseDC(hWnd, hdc);
        return;
    }
    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        MessageBox(hWnd, L"Failed to set pixel format", L"Error", MB_OK);
        ReleaseDC(hWnd, hdc);
        return;
    }




    // Create an OpenGL context
    HGLRC hglrc = wglCreateContext(hdc);
    if (hglrc == NULL) {
        MessageBox(hWnd, L"Failed to create OpenGL context", L"Error", MB_OK);
        ReleaseDC(hWnd, hdc);
        return;
    }

    // Make the context current
    if (!wglMakeCurrent(hdc, hglrc)) {
        MessageBox(hWnd, L"Failed to make OpenGL context current", L"Error", MB_OK);
        wglDeleteContext(hglrc);
        ReleaseDC(hWnd, hdc);
        return;
    }



    if (!gladLoadGL()) {
        MessageBox(hWnd, L"Failed to initialize GLAD", L"Error", MB_OK);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hWnd, hdc);
        return;
    }


    ////////////////////////////////////
    // Compile the shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Create a program and link the shaders
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Create Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
    GLuint vtxArrayObj = 0;
    GLuint vtxBufferObj = 0;

    glGenVertexArrays(1, &vtxArrayObj);
    glGenBuffers(1, &vtxBufferObj);
    glBindVertexArray(vtxArrayObj);
    glBindBuffer(GL_ARRAY_BUFFER, vtxBufferObj);
    // Upload vertex data to the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);
    // Specify layout of vertex data (position)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Specify layout of texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // Unbind the buffer and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ////////////////////////////////////////////////////////////////

    // Set up the OpenGL viewport
    int width = 80; // Replace with your window width
    int height = 480; // Replace with your window height
    glViewport(0, 0, width, height);

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        MessageBox(hWnd, L"OpenGL error occurred", L"Error", MB_OK);
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hWnd, hdc);
        return;
    }

    // Set the clear color to green
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // RGBA

    // Clear the screen with the green color
    glClear(GL_COLOR_BUFFER_BIT);

    // Check for OpenGL errors
    error = glGetError();
    if (error != GL_NO_ERROR) {
        MessageBox(hWnd, L"OpenGL error occurred", L"Error", MB_OK);
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hWnd, hdc);
        return;
    }

    // Render the quad
    glUseProgram(program);
    glBindVertexArray(vtxBufferObj);
    glDrawArrays(GL_TRIANGLES, 0, 6);  // 6 vertices for two triangles
    glBindVertexArray(0);
    glUseProgram(0);


    // Swap the buffers to display the green color
    if (!SwapBuffers(hdc)) {
        MessageBox(hWnd, L"Failed to swap buffers", L"Error", MB_OK);
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(hWnd, hdc);
        return;
    }

    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(program);

    glDeleteVertexArrays(1, &vtxArrayObj);
    glDeleteBuffers(1, &vtxBufferObj);

    wglMakeCurrent(hdc, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hWnd, hdc);
}

