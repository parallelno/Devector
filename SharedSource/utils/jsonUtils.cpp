// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "jsonUtils.h"
#include "utils/utils.h"
#include <fstream>

auto dev::LoadJson(const std::string& _path) -> nlohmann::json
{
	std::ifstream file(_path);
	nlohmann::json json;
	file >> json;

	return json;
}

void dev::SaveJson(const std::string& _path, const nlohmann::json& _json)
{
	std::ofstream file(_path);
	file << std::setw(4) << _json << std::endl;
}

void dev::JsonParsingExit(const std::string& _fieldName)
{
	auto msg = std::format("json doesn't have \"{}\" field.", _fieldName);
	dev::Exit(msg, dev::ERROR_UNSPECIFIED);
}

void dev::JsonParsingTypeMissmatchExit(
	const nlohmann::json& _json,
	const std::string& _fieldName, 
	const std::string& _expectedType)
{
	auto msg = std::format("json field \"{}\" values type missmatching. expected \"{}\", got {}",
		_fieldName, _expectedType, _json[_fieldName].type_name());
	dev::Exit(msg, dev::ERROR_UNSPECIFIED);
}

int dev::GetJsonInt(
	const nlohmann::json& _json,
	const std::string& _fieldName,
	const bool _exit,
	int _defaultValue)
{
	if ( !_json.contains(_fieldName))
	{
		if (_exit){
			JsonParsingExit(_fieldName);
		}
		return _defaultValue;
	}
	if ((_json[_fieldName].type() != nlohmann::json::value_t::number_integer &&
		_json[_fieldName].type() != nlohmann::json::value_t::number_unsigned))
	{
		if (_exit)
		{
			JsonParsingTypeMissmatchExit(_json, _fieldName, "int");
		}
		return _defaultValue;
	}
	return _json[_fieldName];
}

double dev::GetJsonDouble(
	const nlohmann::json& _json,
	const std::string& _fieldName,
	const bool _exit,
	double _defaultValue)
{
	if ( !_json.contains(_fieldName))
	{
		if (_exit) {
			JsonParsingExit(_fieldName);
		}
		return _defaultValue;

	}
	if ((_json[_fieldName].type() != nlohmann::json::value_t::number_integer &&
		_json[_fieldName].type() != nlohmann::json::value_t::number_unsigned &&
		_json[_fieldName].type() != nlohmann::json::value_t::number_float))
	{
		if (_exit)
		{
			JsonParsingTypeMissmatchExit(_json, _fieldName, "double");
		}
		return _defaultValue;
	}
	return _json[_fieldName];
}

bool dev::GetJsonBool(
	const nlohmann::json& _json,
	const std::string& _fieldName,
	const bool _exit,
	bool _defaultValue)
{
	if ( !_json.contains(_fieldName))
	{
		if (_exit) {
			JsonParsingExit(_fieldName);
		}
		return _defaultValue;

	}
	if (_json[_fieldName].type() != nlohmann::json::value_t::boolean)
	{
		if (_exit)
		{
			JsonParsingTypeMissmatchExit(_json, _fieldName, "bool");
		}
		return _defaultValue;
	}
	return _json[_fieldName];
}

auto dev::GetJsonString(
	const nlohmann::json& _json,
	const std::string& _fieldName,
	const bool _exit,
	const std::string& _defaultValue)
->std::string
{
	if ( !_json.contains(_fieldName))
	{
		if (_exit) {
			JsonParsingExit(_fieldName);
		}
		return _defaultValue;

	}
	if (_json[_fieldName].type() != nlohmann::json::value_t::string)
	{
		if (_exit)
		{
			JsonParsingTypeMissmatchExit(_json, _fieldName, "std::string");
		}
		return _defaultValue;
	}
	return _json[_fieldName];
}

auto dev::GetJsonObject(
	const nlohmann::json& _json,
	const std::string& _fieldName,
	const bool _exit)
->nlohmann::json
{
	if ( !_json.contains(_fieldName))
	{
		if (_exit) {
			JsonParsingExit(_fieldName);
		}
		return nlohmann::json();
	}
	if (_json[_fieldName].type() != nlohmann::json::value_t::object &&
		_json[_fieldName].type() != nlohmann::json::value_t::array)
	{
		if (_exit)
		{
			JsonParsingTypeMissmatchExit(_json, _fieldName, "nlohmann::json");
		}
		return nlohmann::json();
	}
	return _json[_fieldName];
}