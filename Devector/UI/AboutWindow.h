#pragma once
#include "imgui.h"

#include "Utils/ImGuiUtils.h"
#include "Ui/BaseWindow.h"

namespace dev
{
	class AboutWindow : public BaseWindow
	{
		static constexpr int DEFAULT_WINDOW_W = 500;
		static constexpr int DEFAULT_WINDOW_H = 300;

		const std::string compilation_date = __DATE__;

	public:
		AboutWindow(const float* const _fontSizeP, const float* const _dpiScaleP);
		void Update(bool& _visible);
		void Draw();
	};
};