#pragma once
#ifndef DEV_STATUS_H
#define DEV_STATUS_H

#include <atomic>

namespace dev
{
	template <class T>
	class Status
	{
	protected:
		std::atomic_size_t m_status;
	public:
		Status() = delete;
		Status(const T _status = static_cast<T>(0))
		: 
		m_status(static_cast<size_t>(_status))
		{}

		inline bool operator==(const T& _status) const
		{
			return m_status == static_cast<size_t>(_status);
		}
		inline bool operator!=(const T& _status) const
		{
			return m_status != static_cast<size_t>(_status);
		}
		inline Status& operator=(const T& _other)
		{
			m_status = static_cast<size_t>(_other);

			return *this;
		}
		inline const size_t value() const { return m_status; }

		inline const T type() const 
		{ 
			auto t = static_cast<size_t>(m_status);
			return static_cast<T>(t);
		}
	};
}

#endif // !DEV_STATUS_H

