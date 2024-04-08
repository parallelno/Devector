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

		Breakpoint(const GlobalAddr _globalAddr, 
			const Status _status = Status::ACTIVE, 
			const bool _autoDel = false, const std::string& _comment = "");
		auto GetGlobalAddr() const -> GlobalAddr;
		auto CheckStatus() const -> bool;
		auto GetStatus() const -> Status;
		auto GetStatusI() const -> int;
		void SetStatus(const Status _status);
		auto GetConditionS() const -> std::string;
		auto GetComment() const -> const std::string&;
		bool IsActive() const;
		bool IsAutoDel() const;
		void Print() const;

	private:
		Status m_status;
		GlobalAddr m_globalAddr;
		bool m_autoDel;
		std::string m_comment;

		auto IsActiveS() const -> const char*;
	};
}
#endif // !DEV_BREAKPOINTS_H