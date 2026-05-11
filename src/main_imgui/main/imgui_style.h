#include "utils/gl_utils.h"
#include "imgui_app.h"

#include "utils/types.h"
#include "utils/str_utils.h"
#include "utils/imgui_utils.h"


using GetSettingsStringFunc = std::function<std::string(const std::string&, const std::string&)>;

void AppStyleInit(GetSettingsStringFunc GetSettingsString)
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Text", "D4D4D4FF") ));
	colors[ImGuiCol_TextDisabled] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TextDisabled", "858585FF") ));
	colors[ImGuiCol_WindowBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_WindowBg", "1F1F1FFF") ));
	colors[ImGuiCol_ChildBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ChildBg", "1717171F") ));
	colors[ImGuiCol_PopupBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PopupBg", "212121FF") ));
	colors[ImGuiCol_Border] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Border", "3D3D3D40") ));
	colors[ImGuiCol_BorderShadow] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_BorderShadow", "1412121C") ));
	colors[ImGuiCol_FrameBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_FrameBg", "363636FF") ));
	colors[ImGuiCol_FrameBgHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_FrameBgHovered", "2173CC85") ));
	colors[ImGuiCol_FrameBgActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_FrameBgActive", "0080D4A1") ));
	colors[ImGuiCol_TitleBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TitleBg", "262626FF") ));
	colors[ImGuiCol_TitleBgActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TitleBgActive", "1F1F1FFF") ));
	colors[ImGuiCol_TitleBgCollapsed] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TitleBgCollapsed", "2E2E2EFF") ));
	colors[ImGuiCol_MenuBarBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_MenuBarBg", "333333FF") ));
	colors[ImGuiCol_ScrollbarBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarBg", "1414143D") ));
	colors[ImGuiCol_ScrollbarGrab] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarGrab", "3D3D3DFF") ));
	colors[ImGuiCol_ScrollbarGrabHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarGrabHovered", "7D7D7D73") ));
	colors[ImGuiCol_ScrollbarGrabActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ScrollbarGrabActive", "7575759C") ));
	colors[ImGuiCol_CheckMark] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_CheckMark", "4091D2FF") ));
	colors[ImGuiCol_SliderGrab] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SliderGrab", "216EC78C") ));
	colors[ImGuiCol_SliderGrabActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SliderGrabActive", "248FF7D4") ));
	colors[ImGuiCol_Button] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Button", "398FFFB0") ));
	colors[ImGuiCol_ButtonHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ButtonHovered", "0C5EBEFF") ));
	colors[ImGuiCol_ButtonActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ButtonActive", "0F86FAFF") ));
	colors[ImGuiCol_Header] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Header", "145AB2B0") ));
	colors[ImGuiCol_HeaderHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_HeaderHovered", "004FB3A3") ));
	colors[ImGuiCol_HeaderActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_HeaderActive", "145A99FF") ));
	colors[ImGuiCol_Separator] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Separator", "878CBF1C") ));
	colors[ImGuiCol_SeparatorHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SeparatorHovered", "1A66BFC7") ));
	colors[ImGuiCol_SeparatorActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_SeparatorActive", "1A66BFFF") ));
	colors[ImGuiCol_ResizeGrip] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ResizeGrip", "4D596933") ));
	colors[ImGuiCol_ResizeGripHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ResizeGripHovered", "4297F9AB") ));
	colors[ImGuiCol_ResizeGripActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ResizeGripActive", "4297F9F2") ));
	colors[ImGuiCol_Tab] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_Tab", "454A4FDC") ));
	colors[ImGuiCol_TabHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabHovered", "125CB6CC") ));
	colors[ImGuiCol_TabActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabActive", "1759A8FF") ));
	colors[ImGuiCol_TabUnfocused] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabUnfocused", "121A26F7") ));
	colors[ImGuiCol_TabUnfocusedActive] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TabUnfocusedActive", "23426BFF") ));
	colors[ImGuiCol_DockingPreview] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_DockingPreview", "72708751") ));
	colors[ImGuiCol_DockingEmptyBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_DockingEmptyBg", "171717FF") ));
	colors[ImGuiCol_PlotLines] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotLines", "878787FF") ));
	colors[ImGuiCol_PlotLinesHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotLinesHovered", "87B0D6FF") ));
	colors[ImGuiCol_PlotHistogram] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotHistogram", "2666EDA8") ));
	colors[ImGuiCol_PlotHistogramHovered] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_PlotHistogramHovered", "738AB9FF") ));
	colors[ImGuiCol_TableHeaderBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableHeaderBg", "333335FF") ));
	colors[ImGuiCol_TableBorderStrong] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableBorderStrong", "3D3D3DD1") ));
	colors[ImGuiCol_TableBorderLight] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableBorderLight", "3B3D40FF") ));
	colors[ImGuiCol_TableRowBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableRowBg", "00000000") ));
	colors[ImGuiCol_TableRowBgAlt] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TableRowBgAlt", "5957661C") ));
	colors[ImGuiCol_TextSelectedBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_TextSelectedBg", "0569DED6") ));
	colors[ImGuiCol_DragDropTarget] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_DragDropTarget", "FFFF00E6") ));
	colors[ImGuiCol_NavHighlight] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_NavHighlight", "0A6FE0C2") ));
	colors[ImGuiCol_NavWindowingHighlight] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_NavWindowingHighlight", "FFFFFFB3") ));
	colors[ImGuiCol_NavWindowingDimBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_NavWindowingDimBg", "CCCCCC33") ));
	colors[ImGuiCol_ModalWindowDimBg] = dev::IM_VEC4(dev::StrHexToInt( GetSettingsString("ImGuiCol_ModalWindowDimBg", "CCCCCC59") ));
}