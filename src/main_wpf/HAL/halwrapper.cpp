#include "halwrapper.h"
#include "utils\str_utils.h"
#include "utils\tqueue.h"
#include <msclr\marshal_cppstd.h>

#include <glad/glad.h>

dev::HAL::HAL(System::String^ _pathBootData, System::String^ _pathRamDiskData,
	const bool _ramDiskClearAfterRestart)
{
	auto pathBootData = msclr::interop::marshal_as<std::wstring>(_pathBootData);
	auto pathRamDiskData = msclr::interop::marshal_as<std::wstring>(_pathRamDiskData);

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
	auto vtxShaderS = msclr::interop::marshal_as<std::string>(_vtxShaderS);
	auto fragShaderS = msclr::interop::marshal_as<std::string>(_fragShaderS);

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

auto dev::HAL::GetMaterialParamId(System::IntPtr _hWnd,
	const Id _materialId, System::String^ _paramName)
-> Id
{
	auto paramName = msclr::interop::marshal_as<std::string>(_paramName);

	return m_winGlUtilsP->GetMaterialParamId(
		static_cast<HWND>(_hWnd.ToPointer()),
		static_cast<Id>(_materialId), paramName);
}

auto dev::HAL::UpdateMaterialParam(System::IntPtr _hWnd,
	const Id _materialId, const Id _paramId, 
	const System::Numerics::Vector4^ _paramVal)
-> int
{
	GLUtils::Vec4 paramVal { _paramVal->X, _paramVal->Y, _paramVal->Z, _paramVal->W };

	return static_cast<int>(m_winGlUtilsP->UpdateMaterialParam(
		static_cast<HWND>(_hWnd.ToPointer()),
		_materialId, _paramId, paramVal));
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

int dev::HAL::LoadRecording(System::String^ _path)
{
	auto path = msclr::interop::marshal_as<std::wstring>(_path);

	auto result = dev::LoadFile(path);
	if (!result || result->empty()) return (int)ErrCode::NO_FILES;

	m_hardwareP->Request(Hardware::Req::STOP);
	m_hardwareP->Request(Hardware::Req::RESET);
	m_hardwareP->Request(Hardware::Req::RESTART);

	m_hardwareP->Request(Hardware::Req::DEBUG_RECORDER_DESERIALIZE, { {"data", nlohmann::json::binary(*result)} });

	m_hardwareP->Request(Hardware::Req::DEBUG_RESET, { {"resetRecorder", false} }); // has to be called after Hardware loading Rom because it stores the last state of Hardware
	m_debuggerP->GetDebugData().LoadDebugData(path);

	return (int)(ErrCode::NO_ERRORS);
}

int dev::HAL::LoadRom(System::String^ _path)
{
	auto path = msclr::interop::marshal_as<std::wstring>(_path);

	auto result = dev::LoadFile(path);
	if (!result || result->empty()) return (int)ErrCode::NO_FILES;

	m_hardwareP->Request(Hardware::Req::STOP);
	m_hardwareP->Request(Hardware::Req::RESET);
	m_hardwareP->Request(Hardware::Req::RESTART);

	auto reqData = nlohmann::json({ {"data", *result}, {"addr", Memory::ROM_LOAD_ADDR} });
	m_hardwareP->Request(Hardware::Req::SET_MEM, reqData);

	m_hardwareP->Request(Hardware::Req::DEBUG_RESET, { {"resetRecorder", true} }); // has to be called after Hardware loading Rom because it stores the last state of Hardware
	m_debuggerP->GetDebugData().LoadDebugData(path);
	m_hardwareP->Request(Hardware::Req::RUN);

	return (int)ErrCode::NO_ERRORS;
}

int dev::HAL::LoadFdd(System::String^ _path, const int _driveIdx, const bool _autoBoot)
{
	auto path = msclr::interop::marshal_as<std::wstring>(_path);

	auto fddResult = dev::LoadFile(path);
	if (!fddResult || fddResult->empty()) return false;

	auto origSize = fddResult->size();
	auto fddimg = *fddResult;

	int res = (int)ErrCode::NO_ERRORS;

	// TODO: provide error string output
	if (fddimg.size() > FDD_SIZE) 
	{
		res = (int)ErrCode::WARNING_FDD_IMAGE_TOO_BIG;
		fddimg.resize(FDD_SIZE);
	}

	if (_autoBoot) m_hardwareP->Request(Hardware::Req::STOP);

	// loading the fdd data
	m_hardwareP->Request(Hardware::Req::LOAD_FDD, {
		{"data", fddimg },
		{"driveIdx", _driveIdx},
		{"path", dev::StrWToStr(path)}
		});

	if (_autoBoot)
	{
		m_hardwareP->Request(Hardware::Req::RESET);
		m_hardwareP->Request(Hardware::Req::DEBUG_RESET, { {"resetRecorder", true} }); // has to be called after Hardware loading FDD image because it stores the last state of Hardware
		m_hardwareP->Request(Hardware::Req::RUN);
	}

	return res;
}