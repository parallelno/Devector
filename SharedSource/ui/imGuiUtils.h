#pragma once
#ifndef IMGUI_UTILS_H
#define IMGUI_UTILS_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string>
#include <list>

namespace dev {

#define DEV_COL32_R_MASK     0xFF000000
#define DEV_COL32_G_MASK     0x00FF0000
#define DEV_COL32_B_MASK     0x0000FF00
#define DEV_COL32_A_MASK     0x000000FF

#define DEV_COL32_R_SHIFT    24
#define DEV_COL32_G_SHIFT    16
#define DEV_COL32_B_SHIFT    8
#define DEV_COL32_A_SHIFT    0

	// Make the UI compact because there are so many fields
	void PushStyleCompact();
	void PopStyleCompact();

	void UpdatePropertyPrintStat(const char* _parameterName);
	void DrawSelectableText(const std::string& _text);

	static constexpr ImVec4 IM_VEC4(const uint32_t col)
	{
		return ImVec4(
			((col & DEV_COL32_R_MASK) >> DEV_COL32_R_SHIFT) / 255.0f,
			((col & DEV_COL32_G_MASK) >> DEV_COL32_G_SHIFT) / 255.0f,
			((col & DEV_COL32_B_MASK) >> DEV_COL32_B_SHIFT) / 255.0f,
			((col & DEV_COL32_A_MASK) >> DEV_COL32_A_SHIFT) / 255.0f);
	}
	static constexpr ImU32 IM_U32(const uint32_t col) 
	{
		return
			(col & DEV_COL32_R_MASK) >> DEV_COL32_R_SHIFT << 0 |
			(col & DEV_COL32_G_MASK) >> DEV_COL32_G_SHIFT << 8 |
			(col & DEV_COL32_B_MASK) >> DEV_COL32_B_SHIFT << 16 |
			(col & DEV_COL32_A_MASK) >> DEV_COL32_A_SHIFT << 24;
	}

	void HelpMarker(const char* _text);

	void ShowPopup(const char* _title, const char* _text);
}

#endif // !IMGUI_UTILS_H

