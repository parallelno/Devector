#pragma once
#ifndef IMGUI_UTILS_H
#define IMGUI_UTILS_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string>
#include <list>

namespace dev {

	// Make the UI compact because there are so many fields
	void PushStyleCompact();
	void PopStyleCompact();

	void UpdatePropertyPrintStat(const char* parameterName);
}

#endif // !IMGUI_UTILS_H

