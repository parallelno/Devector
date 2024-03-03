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

	protected:
		const float* const m_fontSizeP = nullptr;
		const float* const m_dpiScaleP = nullptr;

	public:
		BaseWindow(const int _defaultW, const int _defaultH, const float const* _fontSizeP, const  float const* _dpiScaleP)
			:
			m_defaultW(_defaultW), m_defaultH(_defaultH), m_fontSizeP(_fontSizeP), m_dpiScaleP(_dpiScaleP)
		{};

		void Update();
		void Close();
		void Open();
		void SetVisibility(const bool _visible);
		void SetWindowDefaultPosSize();
	};
}
#endif // !DEV_BASE_WINDOW_H

