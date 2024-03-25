#pragma once
#ifndef DEV_GL_UTILS_H
#define DEV_GL_UTILS_H

#include "../Devector/Types.h"
#include "../Devector/Hardware.h"

namespace dev 
{
	class GLUtils
	{
	public:
		static constexpr int FRAME_BUFFER_W = 1024;
		static constexpr int FRAME_BUFFER_H = 512;

		struct ShaderData {
			GLuint1 shaderProgram = 0;
			GLuint1 texture = 0;
			GLuint1 framebufferTexture = 0;
			GLuint1 framebuffer = 0;
			GLuint1 vtxArrayObj = 0;
			GLuint1 vtxBufferObj = 0;
			GLuint1 globalColorBgId = -1;
			GLuint1 globalColorFgId = -1;
		};
	private:
		int m_frameSizeW, m_frameSizeH;

		ShaderData m_shaderData;
		Hardware& m_hardware;

		GLenum1 Init();
		void DrawDisplay();
		void CreateRamTexture();
		GLuint1 CompileShader(GLenum1 _shaderType, const char* _source);
		GLuint1 CreateShaderProgram(const char* _vertexShaderSource, const char* _fragmentShaderSource);


	public:
		GLUtils(Hardware& _hardware);
		~GLUtils();
		void Update();
		GLuint1 GLCheckError(GLuint1 _obj, const std::string& _txt);
		auto GetShaderData() -> const ShaderData*;
		auto IsShaderDataReady() -> const bool;
	};
}

#endif // !DEV_GL_UTILS_H