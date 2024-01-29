// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once
#ifndef ARGS_PARSER_H
#define ARGS_PARSER_H

#include <map>
#include <string>
#include <any>

namespace dev
{

	class ArgsParser
	{
	private:
		using ArgName = std::string;
		using ArgHelp = std::string;
		enum class ArgType : unsigned {
			INT = 0,
			DOUBLE,
			STRING,
			END
		};
		const char* ArgTypeStr[static_cast<size_t>(ArgType::END)] = {
			"int",
			"double",
			"string"
		};
		using Required = bool;

		std::map <ArgName, std::string> m_args;
		std::string m_help;
		bool m_requirementSatisfied = true;

	public:
		ArgsParser(int& _argc, char** _argv, const std::string& _description);

		auto GetString(const std::string& _arg,
			const ArgHelp&,
			const bool _required = true,
			const std::string& _defaultV = "")
			-> const std::string;

		double GetDouble(const std::string& _arg,
			const ArgHelp&,
			const bool _required = true,
			const double _defaultV = 0.0);

		int GetInt(const std::string& _arg,
			const ArgHelp&,
			const bool _required = true,
			const int _defaultV = 0);

		void RequirementMsg(const std::string& _arg);

		void AddParamToHelp(const std::string& _arg,
			ArgType _type, Required _required, std::any _default, 
			const ArgHelp& _help);

		void AddDescriptionToHelp(const std::string& _description);

		void PrintHelp() const;

		bool IsRequirementSatisfied() const;

		// deleted to prevent implicit convertion into bool type of the _required parameter in case thee user mixed up the parameters
		template <typename T>
		double GetDouble(const std::string& _arg,
			const T _required = true,
			const double _defaultV = 0.0) = delete;

		// deleted to prevent implicit convertion into bool type of the _required parameter in case thee user mixed up the parameters
		template <typename T>
		int GetInt(const std::string& _arg,
			const T _required = true,
			const int _defaultV = 0) = delete;
	};

} // namespace dev
#endif // !ARGS_PARSER_H