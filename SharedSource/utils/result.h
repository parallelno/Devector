#pragma once
#ifndef BOT_RESULT_H
#define BOT_RESULT_H
#include <optional>
#include <string>
#include "utils/globals.h"

namespace bot
{
	template<class T>
	class Result
	{
		std::optional<T> m_result;
		bot::ErrCode m_error = bot::NO_ERRORS;

	public:
		Result()
			: m_error(bot::ERROR_UNSPECIFIED)
		{}
		Result(const ErrCode _error)
			:
			m_error(_error)
		{}
		Result(T&& _res,
			const bot::ErrCode _error = bot::NO_ERRORS)
			:
			m_result(std::move(_res)),
			m_error(_error)
		{}
		Result(Result<T>&& _other) 
			: 
			m_error(std::move(_other.m_error)),
			m_result(std::move(_other.m_result))
		{}

		inline auto HasValue() const
			->const bool
		{
			return m_result.has_value();
		}
		operator bool() const
		{
			return m_result.has_value();
		}
		inline auto Error() const
			->const bot::ErrCode
		{
			return m_error;
		}

		inline bool operator==(const bot::ErrCode _err) const
		{
			return m_error == _err;
		}
		inline bool operator!=(const bot::ErrCode _err) const
		{
			return m_error != _err;
		}

		inline T&& operator*()
		{
			return std::move(m_result.value());
		}
		inline T* operator->()
		{
			return &m_result.value();
		}
		inline Result<T>& operator= (const Result<T>& _other) noexcept
		{
			if (this == &_other) return *this;

			m_error = _other.m_error;
			m_result = _other.m_result;

			return *this;
		}
		inline Result<T>& operator= (Result<T>&& _other) noexcept
		{
			if (this == &_other) return *this;

			m_error = std::move(_other.m_error);
			m_result = std::move(_other.m_result);

			return *this;
		}
	};
}

#endif // !BOT_RESULT_H