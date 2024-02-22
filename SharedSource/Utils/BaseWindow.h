#pragma once
#ifndef DEV_BASE_WINDOW_H
#define DEV_BASE_WINDOW_H

namespace dev
{
	class BaseWindow
	{
		bool m_visible = true;
		int m_defaultW;
		int m_defaultH;

	public:
		BaseWindow(const int _defaultW = 256, const int _defaultH = 256) :
			m_defaultW(_defaultW), m_defaultH(_defaultH) {};

		void Update();
		void Close();
		void Open();
		void SetVisibility(const bool _visible);
		void SetWindowDefaultPosSize();
	};
}
#endif // !DEV_BASE_WINDOW_H

