#pragma once

#include <vector>
#include <algorithm>
#include <optional>
#include "utils/utils.h"

namespace dev
{
	// A simple fixed-size history manager for generic type T
	// Provides Add, GetPrev, and GetNext methods
	// Uses a circular buffer internally

	template<typename T>
	class History{
	public:
		History(int _len)
			: m_maxLen(_len)
		{
			m_history.resize(_len);
		}

		void Add(T _data){
			if (m_len > 0 && m_history[m_currentIdx] == _data){
				return;
			}

			if (m_len == 0){
				Init(_data);
				return;
			}

			m_currentIdx = (m_currentIdx + 1) % m_maxLen;
			m_history[m_currentIdx] = _data;

			int endIdx = (m_startIdx + m_len) % m_maxLen;


			if (m_currentIdx == endIdx)
			{
				m_len++;
				if (m_len > m_maxLen)
				{
					m_len = m_maxLen;
					m_startIdx = (m_startIdx + 1) % m_maxLen;
				}
			} else {
				m_len = (m_currentIdx + m_maxLen - m_startIdx) % m_maxLen + 1;
			}

			Print("Add", _data);
		}

		auto GetPrev()
		-> std::optional<T>
		{
			if (m_currentIdx == m_startIdx || m_len == 0)
			{
				return std::nullopt;
			}

			m_currentIdx = (m_currentIdx - 1 + m_maxLen) % m_maxLen;
			auto data = m_history[m_currentIdx];

			Print("Add", *data);

			return data;
		}

		auto GetNext()
		-> std::optional<T>
		{
			int endIdx = (m_startIdx + m_len - 1) % m_maxLen;
			if (m_len == 0 || m_currentIdx == endIdx) return std::nullopt;

			m_currentIdx = (m_currentIdx + 1) % m_maxLen;
			auto data = m_history[m_currentIdx];

			Print("Add", *data);

			return data;
		}

		void Init(T _data){
			m_startIdx = 0;
			m_currentIdx = 0;
			m_history[0] = _data;
			m_len = 1;

			Print("Init", _data);
		}

		void Print(const char* _func_name, Addr value){
			dev::Log("Func: {}, Addr: {:04X}, "
				"m_currentIdx: {}, m_startIdx: {}, m_len: {}",
				_func_name, value, m_currentIdx, m_startIdx, m_len);
		}

	private:
		int m_maxLen = 0;
		std::vector<std::optional<T>> m_history;
		int m_startIdx = 0;
		int m_currentIdx = -1;
		int m_len = 0;
	};
}