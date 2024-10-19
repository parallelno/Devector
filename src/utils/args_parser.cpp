// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <format>
#include <iostream>

#include "utils/args_parser.h"
#include "utils/utils.h"

dev::ArgsParser::ArgsParser(int& _argc, char** _argv,
	const std::string& _description)
{
	AddDescriptionToHelp(_description);
	
	auto argc = static_cast<size_t>(_argc);

	for (size_t i = 1; i < argc; i++)
	{
		// wait for the first param name
		if (_argv[i][0] == '-')
		{
			const std::string paramName = _argv[i]+1;
			std::string value;

			if (i + 1 < argc && _argv[i+1][0] != '-')
			{
				i++;
				value = _argv[i];
			}

			m_args.emplace(paramName, value);
		}
	}
}

void dev::ArgsParser::RequirementMsg(const std::string& _arg)
{
	std::cout << std::format("Required parameter \"{}\" or its value was not provided.\n", _arg);
	m_requirementSatisfied = false;
}

auto dev::ArgsParser::GetString(const std::string& _arg, 
	const ArgHelp& _help, const bool _required, const std::string& _defaultV)
->const std::string
{
	AddParamToHelp(_arg, ArgType::STRING, _required, _defaultV, _help );

	if (!m_args.contains(_arg) || m_args[_arg].empty()) 
	{
		if (_required) ArgsParser::RequirementMsg(_arg);
		return _defaultV;
	}

	return m_args[_arg];
}

double dev::ArgsParser::GetDouble(const std::string& _arg, 
	const ArgHelp& _help, const bool _required, const double _defaultV)
{
	AddParamToHelp(_arg, ArgType::DOUBLE, _required, _defaultV, _help );

	if (!m_args.contains(_arg) || m_args[_arg].empty())
	{
		if (_required) ArgsParser::RequirementMsg(_arg);
		return _defaultV;
	}

	return strtod(m_args[_arg].c_str(), nullptr);
}

int dev::ArgsParser::GetInt(const std::string& _arg, 
	const ArgHelp& _help, const bool _required, const int _defaultV)
{
	AddParamToHelp(_arg, ArgType::INT, _required, _defaultV, _help );

	if (!m_args.contains(_arg) || m_args[_arg].empty())
	{
		if (_required) ArgsParser::RequirementMsg(_arg);
		return _defaultV;
	}
	return strtol(m_args[_arg].c_str(), nullptr, 10);
}

void dev::ArgsParser::AddDescriptionToHelp(const std::string& _description)
{
	m_help += "Help:\n";
	m_help += std::format("Description: {}\n", _description);
	m_help += "format: -paramName <value> or -h, -help to show this guide.\n";
	m_help += "Parameters:\n";
}

void dev::ArgsParser::AddParamToHelp(const std::string& _arg, 
	ArgType _type, Required _required, std::any _default, const ArgHelp& _help)
{
	std::string defaultStr;
	if (_type == ArgType::DOUBLE) defaultStr = std::to_string(std::any_cast<double>(_default));
	if (_type == ArgType::INT) defaultStr = std::to_string(std::any_cast<int>(_default));
	if (_type == ArgType::STRING) defaultStr = std::any_cast<std::string>(_default);

	m_help += std::format("-{} \n\ttype: {}, required: {}, default: {}\n{}\n\n",
		_arg,
		ArgTypeStr[static_cast<size_t>(_type)],
		_required ? "true" : "false",
		_required ? "no default" : defaultStr,
		("\t" + _help));
}

void dev::ArgsParser::PrintHelp() const
{
	if (!m_args.contains("help") && 
		!m_args.contains("h") && 
		!m_args.empty() &&
		m_requirementSatisfied) return;

	std::cout << std::format("\n{}", m_help);
}

bool dev::ArgsParser::IsRequirementSatisfied() const
{
	PrintHelp();
	return m_requirementSatisfied;
}