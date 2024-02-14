#pragma once
#ifndef DEV_BASE_WINDOW_H
#define DEV_BASE_WINDOW_H

namespace dev
{
	class BaseWindow
	{
	protected:
		bool m_visible = true;

	public:
		void Close();
		void Open();
		void SetVisibility(const bool _visible);
	};
}
#endif // !DEV_BASE_WINDOW_H

