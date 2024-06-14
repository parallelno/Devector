#pragma once

#include <map>
#include <array>
#include <format>

#include "Utils/Types.h"
#include "Core/Breakpoint.h"


namespace dev
{
	class TraceLog
	{
		static const constexpr size_t TRACE_LOG_SIZE = 100000;

		struct Item
		{
			GlobalAddr globalAddr;
		};

		std::array <Item, TRACE_LOG_SIZE> m_items;
		size_t m_traceLogIdx = 0;
		int m_traceLogIdxViewOffset = 0;
	};


}