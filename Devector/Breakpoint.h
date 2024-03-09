#pragma once
#ifndef DEV_BREAKPOINTS_H
#define DEV_BREAKPOINTS_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "Types.h"

namespace dev
{
	class Breakpoint
	{
	public:
		enum class Status : int {
			DELETED = 0,
			DISABLED,
			ACTIVE
		};
		Breakpoint(const GlobalAddr _globalAddr, const Status _status = Status::ACTIVE, const std::string& _comment = "");
		auto CheckStatus() const -> const bool;
		auto GetStatus() const -> const Status;
		auto GetStatusI() const -> const int;
		auto GetComment() const -> const std::string&;
		auto IsActiveS() const -> const char*;
		void Print() const;

	private:
		Status m_status;
		GlobalAddr m_globalAddr;
		std::string m_comment;
	};
}
#endif // !DEV_BREAKPOINTS_H