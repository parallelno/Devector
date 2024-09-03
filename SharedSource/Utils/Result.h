#pragma once

#include <optional>
#include <string>

#include "Utils/Consts.h"

namespace dev
{
	template<class T>
	class Result
	{
		std::optional<T> m_result;
		dev::ErrCode m_error = dev::NO_ERRORS;

	public:
		Result()
			: m_error(dev::ERROR_UNSPECIFIED)
		{}
		Result(const ErrCode _error)
			:
			m_error(_error)
		{}
		Result(T&& _res,
			const dev::ErrCode _error = dev::NO_ERRORS)
			:
			m_result(std::move(_res)),
			m_error(_error)
		{}
		Result(Result<T>&& _other)
			: 
			m_error(std::move(_other.m_error)),
			m_result(std::move(_other.m_result))
		{}
		/*
		template<typename U, typename = std::enable_if_t<std::is_integral_v<T>>>
		Result(U&& _res, const dev::ErrCode _error = dev::NO_ERRORS)
			:
			m_result(_res),
			m_error(_error)
		{}
		*/
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
			->const dev::ErrCode
		{
			return m_error;
		}

		inline bool operator==(const dev::ErrCode _err) const
		{
			return m_error == _err;
		}
		inline bool operator!=(const dev::ErrCode _err) const
		{
			return m_error != _err;
		}

		inline T&& operator*()
		{
			return std::move(m_result.value());
		}
		/*
		template<typename U, typename = std::enable_if_t<std::is_integral_v<T>>>
		inline U&& operator*()
		{
			return m_result.value();
		}
		*/
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