#include <string>

#pragma once

namespace dev
{
	class BaseWindow
	{
		int m_defaultW;
		int m_defaultH;

	protected:
		const float* const m_dpiScaleP = nullptr;

	public:
		const std::string m_name;

		BaseWindow(const std::string& _name, 
			const int _defaultW, const int _defaultH, 
			const  float* const _dpiScaleP)
			:
			m_name(_name), m_defaultW(_defaultW), m_defaultH(_defaultH), 
			m_dpiScaleP(_dpiScaleP)
		{};

		void Update();
		void SetWindowDefaultPosSize();
	};
}