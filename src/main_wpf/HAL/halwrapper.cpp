#include "halwrapper.h"
#include <msclr\marshal_cppstd.h>

/*
#include <d3d11.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3dcompiler.h>
#include <DirectXMath.h>
*/
//#define GLEW_STATIC
#include <GL/glew.h>

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


////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Simple pixel shader (example)
/*
const char* vertexShaderCode = R"(
// VertexShader.hlsl
struct VS_INPUT {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.position = input.position;
    output.texCoord = input.texCoord;
    return output;
}
)";

const char* pixelShaderCode = R"(
// PixelShader.hlsl
struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D texture : register(t0);
SamplerState samplerState : register(s0);

float4 PS(PS_INPUT input) : SV_TARGET {
    return texture.Sample(samplerState, input.texCoord);
}
)";

struct Vertex {
    DirectX::XMFLOAT3 position; // 3D position
    DirectX::XMFLOAT2 texCoord;  // Texture coordinate
};

Vertex quadVertices[] = {
    { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } }, // Top-left
    { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } }, // Top-right
    { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }, // Bottom-left
    { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }  // Bottom-right
};

UINT indices[] = { 0, 1, 2, 1, 3, 2 }; // Two triangles for the quad

ID3DBlob* CompileShaderFromString(const std::string& shaderCode, const char* entryPoint, const char* shaderModel) 
{
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        shaderCode.c_str(),
        shaderCode.length(),
        nullptr,  // Optional, can provide a shader file name here for debugging
        nullptr,  // Compile macros
        nullptr,  // Optional, use the default allocator
        entryPoint,
        shaderModel,
        0,        // Compile flags
        0,        // Effect flags
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        throw std::runtime_error("Failed to compile shader from string.");
    }

    if (errorBlob) {
        errorBlob->Release();
    }

    return shaderBlob;
}
*/

// Create a vertex shader
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
        FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // orange color
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


    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        MessageBox(hWnd, L"Failed to init GLEW", L"Error", MB_OK);
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

    // Create a vertex buffer object (VBO) for the quad
    GLfloat quadVertices[] = {
        -1.0f, -1.0f, // bottom left
         1.0f, -1.0f, // bottom right
         1.0f,  1.0f, // top right
        -1.0f,  1.0f  // top left
    };
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Create a vertex array object (VAO) for the quad
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Specify the vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

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
    glBindVertexArray(vao);
    glDrawArrays(GL_QUADS, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);    // Swap buffers
    
    
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

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    wglMakeCurrent(hdc, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hWnd, hdc);


/*
* ///////////////////// 
*   DX 11 rendering

    // 1. Create Direct3D device and swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;  // Automatically set by the window
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                     // Adapter
        D3D_DRIVER_TYPE_HARDWARE,    // Driver Type
        nullptr,                     // Software
        0,                           // Flags
        nullptr,                     // Feature Levels
        0,                           // Num Feature Levels
        D3D11_SDK_VERSION,           // SDK Version
        &sd,                         // Swap Chain Desc
        &swapChain,                  // [out] Swap Chain
        &d3dDevice,                  // [out] Device
        nullptr,                     // Feature Level
        &d3dContext                  // [out] Device Context
    );

    if (FAILED(hr))
    {
        throw gcnew System::Exception("Failed to initialize Direct3D.");
    }

    // 2. Create render target view
    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr))
    {
        throw gcnew System::Exception("Failed to get back buffer.");
    }

    ID3D11RenderTargetView* renderTargetView = nullptr;
    hr = d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();  // Release the back buffer once we're done with it

    if (FAILED(hr))
    {
        throw gcnew System::Exception("Failed to create render target view.");
    }

    d3dContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

    // 3. Set up viewport
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(800);  // Set to your window size
    vp.Height = static_cast<float>(600);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    d3dContext->RSSetViewports(1, &vp);

    // 4. Create a 2D texture
    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* textureView = nullptr;

    // Define the texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = 256;  // Texture width
    textureDesc.Height = 256; // Texture height
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // Create the texture
    hr = d3dDevice->CreateTexture2D(&textureDesc, nullptr, &texture);
    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create texture.");
    }

    // Fill the texture with data (for example, a solid color)
    std::vector<unsigned char> texData(256 * 256 * 4, 255); // Solid white texture
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = texData.data();
    initData.SysMemPitch = 256 * 4; // Width * number of bytes per pixel
    initData.SysMemSlicePitch = 0;

    d3dContext->UpdateSubresource(texture, 0, nullptr, initData.pSysMem, initData.SysMemPitch, initData.SysMemSlicePitch);
    if (FAILED(hr))
    {
        texture->Release();
        throw std::runtime_error("Failed to update texture subresource.");
    }

    // Create the shader resource view
    hr = d3dDevice->CreateShaderResourceView(texture, nullptr, &textureView);
    texture->Release(); // Release the texture after creating the view

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create shader resource view.");
    }

    // Set the shader resource view
    d3dContext->PSSetShaderResources(0, 1, &textureView);

    // 5. Rendering loop (simple for one frame)
    float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };  // RGBA
    d3dContext->ClearRenderTargetView(renderTargetView, clearColor);

    // Render your texture here (set up shaders, input layout, and draw call)

    // 6. Setup and compile shaders (assuming you have a method to compile and set shaders)
    // You will need to implement your own shader compilation and setting mechanism.
    ID3DBlob* vertexShaderBlob = CompileShaderFromString(vertexShaderCode, "VS", "vs_5_0");
    ID3D11VertexShader* vertexShader = nullptr;
    hr = d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertex shader.");
    }

    ID3DBlob* pixelShaderBlob = CompileShaderFromString(pixelShaderCode, "PS", "ps_5_0");
    ID3D11PixelShader* pixelShader = nullptr;
    hr = d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create pixel shader.");
    }


    // 7. Draw a full-screen quad to render the texture
    // This is a simplified example; you would need to set up vertex buffers, input layouts, etc.
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(quadVertices);
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = quadVertices;

    hr = d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertex buffer.");
    }

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(indices);
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    hr = d3dDevice->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create index buffer.");
    }


    // For example, you could set up vertices for a quad covering the screen and draw it.
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    // Bind the vertex buffer
    d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    // Bind the index buffer
    d3dContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set the primitive topology
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw the quad
    d3dContext->DrawIndexed(6, 0, 0); // 6 indices to draw






    // 8. Present the swap chain
    swapChain->Present(1, 0);  // Swap the back and front buffers (i.e., present the rendered image)

    // Cleanup (release all Direct3D resources)
    vertexShader->Release();
    pixelShader->Release();
    vertexBuffer->Release();
    indexBuffer->Release();
    vertexShaderBlob->Release();
    pixelShaderBlob->Release();


    renderTargetView->Release();
    swapChain->Release();
    d3dContext->Release();
    d3dDevice->Release();

    */
}

