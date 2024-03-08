#pragma once
#ifndef DEV_BREAKPOINTS_H
#define DEV_BREAKPOINTS_H

#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <format>

#include "I8080.h"
#include "Memory.h"

namespace dev
{
	class Breakpoint
	{
	public:
		enum class Status : int {
			NONE = 0,
			DISABLED,
			ENABLED
		};
		Breakpoint(const uint32_t _globalAddr, const Status _status = Status::ENABLED);
		auto CheckStatus() const -> const bool;
		auto GetStatus() const -> const Status;
		auto GetStatusI() const -> const int;
		auto IsActiveS() const -> const char*;
		void Print() const;

	private:
		Status m_status;
		uint32_t m_globalAddr;
	};
}
#endif // !DEV_BREAKPOINTS_H