#pragma once

#include "Consts.h"
#include "Core/Hardware.h"
#include <vector>
#include <unordered_map>

namespace dev 
{
	class GLUtils
	{
	public:
		struct Vec4 { 
			float x, y, z, w; 
			Vec4() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {};	
			Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {};
		};
		using ShaderParamData = std::unordered_map<GLuint1, Vec4*>;
		using ShaderParams = std::unordered_map<std::string, Vec4*>;
		using ShaderTextureParams = std::unordered_map<GLenum1, GLuint1>;
		using TextureIds = std::vector<GLuint1>;
		using MaterialId = uint32_t;

		struct Texture
		{
			enum class Format { RGB, RGBA, R8, R32, };
			Format format;
			GLsizei1 w, h;
			GLuint1 id;
			GLint1 internalFormat;
			GLenum1 type;
			GLint1 filter;

			Texture(GLsizei1 _w, GLsizei1 _h, Texture::Format _format, GLint1 _filter);
		};

		struct Material
		{
			GLuint1 shaderId = 0;
			GLuint1 framebufferTexture;
			GLuint1 framebuffer;
			int framebufferW;
			int framebufferH;
			Vec4 backColor;
			ShaderParamData params;
			ShaderTextureParams textureParams;

			Material(GLuint1 _shaderId, const int _framebufferW, const int _framebufferH, const ShaderParams& _paramParams,
					const Vec4& _backColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f));
			Material() = delete;
		};

	private:
		MaterialId m_materialId = 0;
		std::unordered_map<MaterialId, Material> m_materials;
		GLuint1 vtxArrayObj = 0;
		GLuint1 vtxBufferObj = 0;
		std::unordered_map<GLuint1, Texture> m_textures;
		std::vector<GLuint1> m_shaders;

		GLenum1 m_glewInitCode;

		auto CompileShader(GLenum1 _shaderType, const char* _source) -> Result<GLuint1>;
		auto GLCheckError(GLuint1 _obj, const std::string& _txt) -> Result<GLuint1>;

	public:
		GLUtils();
		~GLUtils();

		auto InitShader(const char* _vertexShaderSource, const char* _fragmentShaderSource) -> Result<GLuint1>;

		auto InitMaterial(GLuint1 _shaderId, const int _framebufferW, const int _framebufferH, 
			const TextureIds& _textureIds, const ShaderParams& _paramParams, 
			const int _framebufferTextureFilter = GL_NEAREST)
				-> dev::Result<MaterialId>;
		auto InitTexture(GLsizei1 _w, GLsizei1 _h, Texture::Format _format, const int textureFilter = GL_NEAREST)
				-> Result<GLuint1>;

		auto Draw(const MaterialId _renderDataId) const -> int;
		void UpdateTexture(const int _texureId, const uint8_t* _memP);
		auto GetFramebufferTexture(const int _materialId) const -> GLuint1;
		bool IsMaterialReady(const int _materialId) const;
	};
}