#include "BaseWindow.h"

void dev::BaseWindow::Close()
{
	m_visible = false;
}

void dev::BaseWindow::Open()
{
	m_visible = true;
}

void dev::BaseWindow::SetVisibility(const bool _visible)
{
	m_visible = _visible;
}