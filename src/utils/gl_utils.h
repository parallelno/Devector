#pragma once
#include <glad/glad.h>
#include <SDL3/SDL_opengl.h>

#ifdef APIENTRY
    #undef APIENTRY
#endif

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

		using ShaderParamIds = std::unordered_map <std::string, dev::Id>;
		using ShaderParamData = std::unordered_map <dev::Id, Vec4>;
		using ShaderParams = std::unordered_map <std::string, Vec4>;
		using ShaderTextureParams = std::unordered_map<GLenum, dev::Id>;
		using TextureIds = std::vector<dev::Id>;

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
			Id shaderId = 0;
			bool renderToTexture;
			GLuint framebufferTexture;
			GLuint framebuffer;
			int viewportW;
			int viewportH;
			Vec4 backColor;
			ShaderParamData params;
			ShaderParamIds paramIds;
			ShaderTextureParams textureParams;

			Material(Id _shaderId, 
				const ShaderParams& _shaderParams,
				const int _framebufferW, const int _framebufferH,
				const bool _renderToTexture = true,
				const Vec4& _backColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f));
			Material() = delete;
			auto GetParamId(const std::string& _paramName) -> Id;
			auto SetParam(const Id _id, const Vec4& _data) -> ErrCode;
		};

	private:
		Id m_materialId = 0;
		std::unordered_map<Id, Material> m_materials;
		GLuint vtxArrayObj = 0;
		GLuint vtxBufferObj = 0;
		std::unordered_map<GLuint, Texture> m_textures;
		std::vector<Id> m_shaders;

		GLenum m_gladInited = 0;

		void InitGeometry();
		auto CompileShader(GLenum _shaderType, const char* _source) -> Id;
		auto GLCheckError(Id _id, const std::string& _txt) -> Id;

	public:
		GLUtils(bool _init);
		~GLUtils();

		auto InitShader(const char* _vtxShaderS, const char* _fragShaderS) -> Id;

		auto InitMaterial(Id _shaderId,
			const TextureIds& _textureIds, const ShaderParams& _shaderParams,
			const int _framebufferW, const int _framebufferH, 
			const bool _renderToTexture = true,
			const int _framebufferTextureFilter = GL_NEAREST)
				-> Id;
		auto InitTexture(GLsizei _w, GLsizei _h, Texture::Format _format, 
			const GLint textureFilter = GL_NEAREST) 
			-> Id;

		auto Draw(const Id _renderDataId) const -> ErrCode;

		auto GetMaterialParamId(const Id _materialId, const std::string& _paramName) -> Id;
		auto UpdateMaterialParam(const Id _materialId, const Id _paramId, const Vec4& _param) 
			-> ErrCode;
		
		void UpdateTexture(const Id _texureId, const uint8_t* _memP);
		auto GetFramebufferTexture(const Id _materialId) const -> Id;
		auto GetMaterial(const Id _matId) -> Material*;
		auto GetVtxArrayObj() const -> Id { return vtxArrayObj; };
		auto GetVtxBufferObj() const -> Id { return vtxBufferObj; };
		auto IsInited() const -> GLenum { return m_gladInited; };
		bool IsMaterialReady(const Id _materialId) const;
	};
}