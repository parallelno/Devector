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

		#define RAM_TEXTURES (Memory::GLOBAL_MEMORY_LEN / Memory::MEM_64K)
		#define	RAM_TEXTURE_W 256
		#define	RAM_TEXTURE_H (GLsizei)(Memory::MEMORY_MAIN_LEN / 256)

		struct ShaderData {
			GLuint1 shaderProgram = 0;
			GLuint1 ramTextures[RAM_TEXTURES] = {0};
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
		void CreateRamTextures();
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