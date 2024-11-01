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
    m_gl_utils = new GLUtils();
}

void dev::HAL::Init(System::IntPtr _hwnd)
{
    m_activeArea_pxlSizeP = new GLUtils::Vec4({ Display::ACTIVE_AREA_W, Display::ACTIVE_AREA_H, FRAME_PXL_SIZE_W, FRAME_PXL_SIZE_H });
    m_scrollV_crtXY_highlightMulP = new GLUtils::Vec4({ 255.0f * FRAME_PXL_SIZE_H, 0.0f, 0.0f, 1.0f });
    m_bordsLRTBP = new GLUtils::Vec4({
                        static_cast<float>(0), // inited in the constructor
                        static_cast<float>(0), // inited in the constructor
                        static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP * FRAME_PXL_SIZE_H),
                        static_cast<float>(Display::SCAN_ACTIVE_AREA_TOP + Display::ACTIVE_AREA_H) * FRAME_PXL_SIZE_H });

    m_hardwareP->Request(Hardware::Req::RUN);
    HWND hWnd = static_cast<HWND>(_hwnd.ToPointer());
    m_gl_utils->InitGL(hWnd);
}

dev::HAL::~HAL()
{
    this->!HAL(); // Ensure finalizer is called
}

dev::HAL::!HAL()
{
    delete m_debuggerP; m_debuggerP = nullptr;
    delete m_hardwareP; m_hardwareP = nullptr;
    delete m_gl_utils; m_gl_utils = nullptr;

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

void dev::HAL::RenderTexture(System::IntPtr _hwnd, GLsizei _viewportW, GLsizei _viewportH)
{
    HWND hWnd = static_cast<HWND>(_hwnd.ToPointer());
    RenderTextureOnHWND(hWnd, _viewportW, _viewportH);
}

void dev::HAL::Render(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH)
{

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
	//layout (location = 0) in vec3 pos;
	//layout (location = 1) in vec2 uv;

    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
    }
)";

// Create a fragment shader
const char* fragmentShaderSource = R"(
    #version 330 core

    uniform vec4 viewport_size;

    out vec4 FragColor;

    void main() 
    {
        vec2 uv = gl_FragCoord.xy / viewport_size.xy;

        float tiles = 4.0f;

        float checker_x = step(0.5f, fract(uv.x * tiles));
        float checker_y = step(0.5f, fract(uv.y * tiles));
        float checker = checker_y == 0.0f ? 1.0f - checker_x : checker_x;

        FragColor = vec4(checker, 0, 0.0f, 1.0f); // orange color
    }
)";

auto GLCheckError(GLuint _obj, const std::string& _txt, HWND hWnd)
-> std::string
{
    // Check for compilation errors
    GLint success;
    glGetShaderiv(_obj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(_obj, 512, NULL, infoLog);
        System::Console::WriteLine(_txt.c_str());
        System::Console::WriteLine(infoLog);
        MessageBox(hWnd, dev::StrToStrW(infoLog).c_str(), L"Error", MB_OK);
        return std::string(infoLog);
    }
    return {};
}



void dev::HAL::RenderTextureOnHWND(HWND _hWnd, GLsizei _viewportW, GLsizei _viewportH)
{
    // Assuming hWnd is the handle to the window
    HDC hdc = GetDC(_hWnd);
    if (hdc == NULL) {
        MessageBox(_hWnd, L"Failed to get device context", L"Error", MB_OK);
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
        MessageBox(_hWnd, L"Failed to choose pixel format", L"Error", MB_OK);
        ReleaseDC(_hWnd, hdc);
        return;
    }
    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        MessageBox(_hWnd, L"Failed to set pixel format", L"Error", MB_OK);
        ReleaseDC(_hWnd, hdc);
        return;
    }




    // Create an OpenGL context
    HGLRC hglrc = wglCreateContext(hdc);
    if (hglrc == NULL) {
        MessageBox(_hWnd, L"Failed to create OpenGL context", L"Error", MB_OK);
        ReleaseDC(_hWnd, hdc);
        return;
    }

    // Make the context current
    if (!wglMakeCurrent(hdc, hglrc)) {
        MessageBox(_hWnd, L"Failed to make OpenGL context current", L"Error", MB_OK);
        wglDeleteContext(hglrc);
        ReleaseDC(_hWnd, hdc);
        return;
    }



    if (!gladLoadGL()) {
        MessageBox(_hWnd, L"Failed to initialize GLAD", L"Error", MB_OK);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(_hWnd, hdc);
        return;
    }


    ////////////////////////////////////
    // Compile the shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLCheckError(vertexShader, "fragmentShaderSource", _hWnd);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLCheckError(fragmentShader, "fragmentShaderSource", _hWnd);

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
    //int width = 80; // Replace with your window width
    //int height = 480; // Replace with your window height
    glViewport(0, 0, GLsizei(_viewportW), GLsizei(_viewportH));

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        MessageBox(_hWnd, L"OpenGL error occurred", L"Error", MB_OK);
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(_hWnd, hdc);
        return;
    }

    // Set the clear color to green
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // RGBA

    // Clear the screen with the green color
    glClear(GL_COLOR_BUFFER_BIT);

    // Check for OpenGL errors
    error = glGetError();
    if (error != GL_NO_ERROR) {
        MessageBox(_hWnd, L"OpenGL error occurred", L"Error", MB_OK);
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(_hWnd, hdc);
        return;
    }

    // Render the quad
    glUseProgram(program);
    auto paramId = glGetUniformLocation(program, "viewport_size");
    glUniform4f(paramId, _viewportW, _viewportH, 0, 0);
    glBindVertexArray(vtxBufferObj);
    glDrawArrays(GL_TRIANGLES, 0, 6);  // 6 vertices for two triangles
    glBindVertexArray(0);
    glUseProgram(0);


    // Swap the buffers to display the green color
    if (!SwapBuffers(hdc)) {
        MessageBox(_hWnd, L"Failed to swap buffers", L"Error", MB_OK);
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(hglrc);
        ReleaseDC(_hWnd, hdc);
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
    ReleaseDC(_hWnd, hdc);
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

    auto vramShaderRes = m_gl_utilsP->InitShader(vtxShaderS, fragShaderS);
    if (!vramShaderRes) return false;
    m_vramShaderId = *vramShaderRes;

    auto m_vramTexRes = m_gl_utilsP->InitTexture(Display::FRAME_W, Display::FRAME_H, GLUtils::Texture::Format::RGBA);
    if (!m_vramTexRes) return false;
    m_vramTexId = *m_vramTexRes;

    GLUtils::ShaderParams shaderParams = {
        { "m_activeArea_pxlSize", &(*m_activeArea_pxlSizeP) },
        { "m_scrollV_crtXY_highlightMul", &(*m_scrollV_crtXY_highlightMulP) },
        { "m_bordsLRTB", &(*m_bordsLRTBP) }
    };
    auto vramMatRes = m_gl_utilsP->InitMaterial(m_vramShaderId, Display::FRAME_W, Display::FRAME_H,
        { m_vramTexRes }, shaderParams);
    if (!vramMatRes) return false;
    m_vramMatId = *vramMatRes;

    return true;
}