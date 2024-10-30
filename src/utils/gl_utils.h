#pragma once
#include <glad/glad.h>
//#include <SDL3/SDL_opengl.h>

//#ifdef APIENTRY
//    #undef APIENTRY
//#endif

#include <vector>
#include <unordered_map>

#include "utils/consts.h"
#include "core/hardware.h"


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
		using ShaderParamData = std::unordered_map<GLuint, Vec4*>;
		using ShaderParams = std::unordered_map<std::string, Vec4*>;
		using ShaderTextureParams = std::unordered_map<GLenum, GLuint>;
		using TextureIds = std::vector<GLuint>;
		using MaterialId = uint32_t;

		struct Texture
		{
			enum class Format { RGB, RGBA, R8, R32, };
			Format format;
			GLsizei w, h;
			GLuint id;
			GLint internalFormat;
			GLenum type;
			GLint filter;

			Texture(GLsizei _w, GLsizei _h, Texture::Format _format, GLint _filter);
		};

		struct Material
		{
			GLuint shaderId = 0;
			GLuint framebufferTexture;
			GLuint framebuffer;
			int framebufferW;
			int framebufferH;
			Vec4 backColor;
			ShaderParamData params;
			ShaderTextureParams textureParams;

			Material(GLuint _shaderId, const int _framebufferW, const int _framebufferH, const ShaderParams& _paramParams,
					const Vec4& _backColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f));
			Material() = delete;
		};

	private:
		MaterialId m_materialId = 0;
		std::unordered_map<MaterialId, Material> m_materials;
		GLuint vtxArrayObj = 0;
		GLuint vtxBufferObj = 0;
		std::unordered_map<GLuint, Texture> m_textures;
		std::vector<GLuint> m_shaders;

		GLenum m_gladInited = 0;

		auto CompileShader(GLenum _shaderType, const char* _source) -> Result<GLuint>;
		auto GLCheckError(GLuint _obj, const std::string& _txt) -> Result<GLuint>;

	public:
		GLUtils(bool _init);
		~GLUtils();

		auto InitShader(const char* _vertexShaderSource, const char* _fragmentShaderSource) -> Result<GLuint>;

		auto InitMaterial(GLuint _shaderId, const int _framebufferW, const int _framebufferH, 
			const TextureIds& _textureIds, const ShaderParams& _paramParams, 
			const int _framebufferTextureFilter = GL_NEAREST)
				-> dev::Result<MaterialId>;
		auto InitTexture(GLsizei _w, GLsizei _h, Texture::Format _format, const GLint _textureFilter = GL_NEAREST)
				-> Result<GLuint>;

		auto Draw(const MaterialId _renderDataId) const -> int;
		void UpdateTexture(const int _texureId, const uint8_t* _memP);
		auto GetFramebufferTexture(const int _materialId) const -> GLuint;
		bool IsMaterialReady(const int _materialId) const;
	};
}