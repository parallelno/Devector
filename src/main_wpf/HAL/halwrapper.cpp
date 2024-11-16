#include "halwrapper.h"
#include "utils\str_utils.h"
#include "utils\tqueue.h"
#include <msclr\marshal_cppstd.h>

#include <glad/glad.h>

dev::HAL::HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
	const bool _ramDiskClearAfterRestart)
{
	std::wstring pathBootData = msclr::interop::marshal_as<std::wstring>(_pathBootData);
	std::wstring pathRamDiskData = msclr::interop::marshal_as<std::wstring>(_pathRamDiskData);

	m_hardwareP = new Hardware(pathBootData, pathRamDiskData, _ramDiskClearAfterRestart);
	m_debuggerP = new Debugger(*m_hardwareP);
	m_winGlUtilsP = new WinGlUtils();
}

bool dev::HAL::CreateGfxContext(System::IntPtr _hWnd, GLsizei _viewportW, GLsizei _viewportH)
{
	auto hWnd = static_cast<HWND>(_hWnd.ToPointer());
	auto glContextStatus = m_winGlUtilsP->CreateGfxContext(hWnd, _viewportW, _viewportH);
	auto glInited = glContextStatus == WinGlUtils::Status::INITED;

	return glInited;
}

dev::HAL::~HAL()
{
	this->!HAL(); // Ensure finalizer is called
}

dev::HAL::!HAL()
{
	delete m_debuggerP; m_debuggerP = nullptr;
	delete m_hardwareP; m_hardwareP = nullptr;
	delete m_winGlUtilsP; m_winGlUtilsP = nullptr;
}

auto dev::HAL::InitShader(System::IntPtr _hWnd, 
		System::String^ _vtxShaderS, System::String^ _fragShaderS) 
-> Id
{
	auto hWnd = static_cast<HWND>(_hWnd.ToPointer());
	std::string vtxShaderS = msclr::interop::marshal_as<std::string>(_vtxShaderS);
	std::string fragShaderS = msclr::interop::marshal_as<std::string>(_fragShaderS);

	return m_winGlUtilsP->InitShader(hWnd, vtxShaderS.c_str(), fragShaderS.c_str());
}

auto dev::HAL::InitMaterial(System::IntPtr _hWnd, 
	Id _shaderId, 
	cli::array<System::Int32>^ _textureIds, 
	cli::array<System::String^>^ _shaderParamNames, 
	cli::array<System::Numerics::Vector4>^ _shaderParamValues, 
	int _framebufferW, int _framebufferH)
-> Id
{
	GLUtils::TextureIds textureIds;
	GLUtils::ShaderParams shaderParams;

	for (int i = 0; i < _textureIds->Length; i++) {
		textureIds.push_back(static_cast<Id>(_textureIds[i]));
	}
	
	for (int i = 0; i < _shaderParamNames->Length; i++) 
	{
		auto nameR = _shaderParamNames[i];
		auto name = msclr::interop::marshal_as<std::string>(nameR);
		auto value = _shaderParamValues[i];
		shaderParams[name] = { value.X, value.Y, value.Z, value.W };
	}
	
	return static_cast<System::Int32>(m_winGlUtilsP->InitMaterial(
		static_cast<HWND>(_hWnd.ToPointer()),
		static_cast<Id>(_shaderId),
		textureIds,
		shaderParams,
		_framebufferW,
		_framebufferH));
}

auto dev::HAL::InitTexture(System::IntPtr _hWnd, GLsizei _w, GLsizei _h) 
-> Id
{
	auto hWnd = static_cast<HWND>(_hWnd.ToPointer());
	return m_winGlUtilsP->InitTexture(hWnd, _w, _h);
}


void dev::HAL::UpdateFrameTexture(System::IntPtr _hWnd, Id _textureId, const bool _vsync)
{
	auto frameP = m_hardwareP->GetFrame(_vsync);
	m_winGlUtilsP->UpdateTexture(
		static_cast<HWND>(_hWnd.ToPointer()),
		_textureId, (uint8_t*)frameP->data());
}

auto dev::HAL::Draw(System::IntPtr _hWnd, const Id _materialId,
	const GLsizei _viewportW, const GLsizei _viewportH)
-> int
{
	return static_cast<int>(m_winGlUtilsP->Draw(
		static_cast<HWND>(_hWnd.ToPointer()), 
		_materialId, _viewportW, _viewportH));
}

////////////////////////////////////////////////
//
// Requests
//
////////////////////////////////////////////////


auto dev::HAL::Request(const Req _req, System::String^ _dataS)
-> System::Text::Json::JsonDocument^
{
	auto dataJ = System::String::IsNullOrEmpty(_dataS) ?
		nlohmann::json() :
		nlohmann::json::parse(msclr::interop::marshal_as<std::string>(_dataS));

	auto req = m_hardwareP->Request(static_cast<Hardware::Req>(_req), dataJ);
	if (!req) return nullptr;

	auto dataS = req->dump();
	return System::Text::Json::JsonDocument::Parse(
		msclr::interop::marshal_as<System::String^>(dataS), {});
}

auto dev::HAL::ReqCC()
-> uint64_t
{
	auto res = m_hardwareP->Request(Hardware::Req::GET_CC);
	const auto& data = *res;

	return data["cc"];
}

bool dev::HAL::ReqIsRunning()
{
	return m_hardwareP->Request(Hardware::Req::IS_RUNNING)->at("isRunning");
}

void dev::HAL::ReqRun()
{
	m_hardwareP->Request(Hardware::Req::RUN);
}
