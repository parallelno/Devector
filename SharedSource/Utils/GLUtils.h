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
		struct Vec4 { float x, y, z, w; };
		using ShaderParamData = std::map<GLuint1, Vec4*>;
		using ShaderParams = std::map<std::string, Vec4*>;
		struct RenderData
		{
			GLuint1 shaderProgram = 0;
			std::vector<GLuint1> textures;
			std::vector<GLuint1> framebufferTextures;
			std::vector<GLuint1> framebuffers;
			int framebufferW;
			int framebufferH;
			int textureW;
			int textureH;
			int textureCount;
			ShaderParamData params;

			RenderData(const int _textureCount = 1):
				textures(_textureCount, -1), framebufferTextures(_textureCount, -1), framebuffers(_textureCount, -1),
				textureCount(_textureCount)
			{};
			RenderData() = delete;
		};

	private:
		std::vector<RenderData> m_renderDatas;
		GLuint1 vtxArrayObj = 0;
		GLuint1 vtxBufferObj = 0;

		GLenum1 m_glewInitCode;

		GLuint1 CompileShader(GLenum1 _shaderType, const char* _source);
		GLuint1 CreateShaderProgram(const char* _vertexShaderSource, const char* _fragmentShaderSource);
		GLuint1 GLCheckError(GLuint1 _obj, const std::string& _txt);

	public:
		GLUtils();
		~GLUtils();
		auto InitRenderData(const std::string& _vtxShaderS, const std::string& _fragShaderS,
			const int _framebufferW, const int _framebufferH, const ShaderParams& _paramParams,
			const int _textureCount = 1) -> int;

		auto Draw(const int _renderDataIdx) const -> int;
		void UpdateTextures(const int _renderDataIdx, const uint8_t* _memP, const int _width, const int _height, const int _colorDepth);
		auto GetFramebufferTextures(const int _renderDataIdx) const -> const std::vector<GLuint1>&;
		bool IsShaderDataReady(const int _renderDataIdx) const;
	};
}

#endif // !DEV_GL_UTILS_H