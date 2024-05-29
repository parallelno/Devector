#pragma once

#include "json.hpp"

namespace dev
{
#define JSON_SET(jsonObj, Var) (jsonObj[#Var] = (Var))

#define JSON_SET_ENUM(jsonObj, EmunType, Var) \
        case EmunType::##Var : \
		     jsonObj[#EmunType] = #Var; \
		break;
#define JSON_GET(jsonObj, Var) ( Var = jsonObj[#Var] )
#define JSON_GET_C(jsonObj, Var) const auto Var = jsonObj[#Var];
#define JSON_GET_I(jsonObj, Var) Var(jsonObj[#Var])

#define JSON_GET_EX(jsonObj, Parent, Var) (jsonObj[#Var])

	auto LoadJson(const std::string& _path) ->nlohmann::json;
	auto LoadJson(const std::wstring& _path) -> nlohmann::json;
	void SaveJson(const std::string& _path, const nlohmann::json& _json);

	void JsonParsingExit(const std::string& _fieldName);
	void JsonParsingTypeMissmatchExit(
		const nlohmann::json& _json, 
		const std::string& _fieldName, 
		const std::string& _expectedType);

	int GetJsonInt(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		const bool _exit = true,
		int _defaultValue = 0);

	template <typename T>
	int GetJsonInt(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		const T _exit = true,
		int _defaultValue = 0) = delete;

	double GetJsonDouble(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		const bool _exit = true,
		double _defaultValue = 0.0);
	
	template <typename T>
	double GetJsonDouble(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		const T _exit = true,
		double _defaultValue = 0.0) = delete;

	bool GetJsonBool(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		const bool _exit = true,
		bool _defaultValue = false);

	auto GetJsonString(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		bool _exit = true,
		const std::string& _defaultValue = "")
		-> std::string;

	auto GetJsonObject(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		const bool _exit = true)
		-> nlohmann::json;

	auto GetJsonVectorUint8(
		const nlohmann::json& _json,
		const std::string& _fieldName,
		const bool _exit = true,
		const std::vector<uint8_t>& _defaultValue = {})
		-> const std::vector<uint8_t>;

}