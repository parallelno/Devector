#pragma once
#ifndef BASE_WINDOW_H
#define BASE_WINDOW_H

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
#endif // !BASE_WINDOW_H

