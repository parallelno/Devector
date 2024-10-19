#include <fstream>

#include "utils/json_utils.h"
#include "utils/utils.h"
#include "utils/str_utils.h"

auto dev::LoadJson(const std::string& _path) -> nlohmann::json
{
	std::ifstream file(_path);
	nlohmann::json json;
	file >> json; 

	return json;
}

auto dev::LoadJson(const std::wstring& _path) -> nlohmann::json
{
#if defined(_WIN32)
		std::ifstream file(_path);
#else
		std::ifstream file(dev::StrWToStr(_path));
#endif

	nlohmann::json json;
	file >> json;

	return json;
}

void dev::SaveJson(const std::string& _path, const nlohmann::json& _json)
{
	std::ofstream file(_path);
	file << std::setw(4) << _json << std::endl;
}

void dev::SaveJson(const std::wstring& _path, const nlohmann::json& _json)
{
#if defined(_WIN32)
		std::ofstream file(_path);
#else
		std::ofstream file(dev::StrWToStr(_path));
#endif

	file << std::setw(4) << _json << std::endl;
}

void dev::JsonParsingExit(const std::string& _key)
{
	auto msg = std::format("json doesn't have \"{}\" field.", _key);
	dev::Exit(msg, dev::ErrCode::UNSPECIFIED);
}

void dev::JsonParsingTypeMissmatchExit(
	const nlohmann::json& _json,
	const std::string& _key, 
	const std::string& _expectedType)
{
	auto msg = std::format("json field \"{}\" values type are missmatching. Expected \"{}\", got {}",
		_key, _expectedType, _json[_key].type_name());
	dev::Exit(msg, dev::ErrCode::UNSPECIFIED);
}

// if no exit, returns the default and adds [key, default value] pair to the json
int dev::GetJsonInt(
	nlohmann::json& _json,
	const std::string& _key,
	const bool _exit,
	int _defaultValue)
{
	if ( !_json.contains(_key))
	{
		if (_exit) JsonParsingExit(_key);

		_json[_key] = _defaultValue;
		return _defaultValue;
	}
	if ((_json[_key].type() != nlohmann::json::value_t::number_integer &&
		_json[_key].type() != nlohmann::json::value_t::number_unsigned))
	{
		if (_exit) JsonParsingTypeMissmatchExit(_json, _key, "int");

		_json[_key] = _defaultValue;
		return _defaultValue;
	}
	return _json[_key];
}

// if no exit, returns the default and adds [key, default value] pair to the json
double dev::GetJsonDouble(
	nlohmann::json& _json,
	const std::string& _key,
	const bool _exit,
	double _defaultValue)
{
	if ( !_json.contains(_key))
	{
		if (_exit) JsonParsingExit(_key);

		_json[_key] = _defaultValue;
		return _defaultValue;

	}
	if ((_json[_key].type() != nlohmann::json::value_t::number_integer &&
		_json[_key].type() != nlohmann::json::value_t::number_unsigned &&
		_json[_key].type() != nlohmann::json::value_t::number_float))
	{
		if (_exit) JsonParsingTypeMissmatchExit(_json, _key, "double");

		_json[_key] = _defaultValue;
		return _defaultValue;
	}
	return _json[_key];
}

// if no exit, returns the default and adds [key, default value] pair to the json
bool dev::GetJsonBool(
	nlohmann::json& _json,
	const std::string& _key,
	const bool _exit,
	bool _defaultValue)
{
	if ( !_json.contains(_key))
	{
		if (_exit) JsonParsingExit(_key);
		
		_json[_key] = _defaultValue;
		return _defaultValue;

	}
	if (_json[_key].type() != nlohmann::json::value_t::boolean)
	{
		if (_exit) JsonParsingTypeMissmatchExit(_json, _key, "bool");
		
		_json[_key] = _defaultValue;
		return _defaultValue;
	}
	return _json[_key];
}

// if no exit, returns the default and adds [key, default value] pair to the json
auto dev::GetJsonString(
	nlohmann::json& _json,
	const std::string& _key,
	const bool _exit,
	const std::string& _defaultValue)
->std::string
{
	if ( !_json.contains(_key))
	{
		if (_exit) JsonParsingExit(_key);
		
		_json[_key] = _defaultValue;
		return _defaultValue;

	}
	if (_json[_key].type() != nlohmann::json::value_t::string)
	{
		if (_exit) JsonParsingTypeMissmatchExit(_json, _key, "std::string");

		_json[_key] = _defaultValue;
		return _defaultValue;
	}
	return _json[_key];
}

// if no exit, returns the default and adds [key, default value] pair to the json
auto dev::GetJsonObject(
	nlohmann::json& _json,
	const std::string& _key,
	const bool _exit)
->nlohmann::json
{
	if ( !_json.contains(_key))
	{
		if (_exit) JsonParsingExit(_key);
		_json[_key] = {};
		return nlohmann::json();
	}
	if (_json[_key].type() != nlohmann::json::value_t::object &&
		_json[_key].type() != nlohmann::json::value_t::array)
	{
		if (_exit) JsonParsingTypeMissmatchExit(_json, _key, "nlohmann::json");
		_json[_key] = {};
		return nlohmann::json();
	}
	return _json[_key];
}

// if no exit, returns the default and adds [key, default value] pair to the json
auto dev::GetJsonVectorUint8(
	nlohmann::json& _json,
	const std::string& _key,
	const bool _exit,
	const std::vector<uint8_t>& _defaultValue)
	-> const std::vector<uint8_t>
{
	if (!_json.contains(_key))
	{
		if (_exit) JsonParsingExit(_key);

		_json[_key] = _defaultValue;
		return _defaultValue;

	}
	if (_json[_key].type() != nlohmann::json::value_t::array)
	{
		if (_exit) JsonParsingTypeMissmatchExit(_json, _key, "std::vector");
		
		_json[_key] = _defaultValue;
		return _defaultValue;
	}
	return _json[_key];
}