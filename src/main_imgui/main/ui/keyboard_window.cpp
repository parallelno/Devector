#include "ui/keyboard_window.h"

#include <format>
#include "utils/str_utils.h"
#include "utils/utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

dev::KeyboardWindow::KeyboardWindow(Hardware& _hardware,
	dev::Scheduler& _scheduler,
	bool* _visibleP, GLUtils& _glUtils,
	const std::string& _pathImgKeyboard)
	:
	BaseWindow("Keyboard", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP),
	m_hardware(_hardware),
	m_glUtils(_glUtils), m_pathImgKeyboard(_pathImgKeyboard)
{
	m_isGLInited = Init();
}

bool dev::KeyboardWindow::Init()
{
	auto img_path = dev::GetExecutableDir() + m_pathImgKeyboard;

	auto dataP = stbi_load(img_path.c_str(), &m_imgKeyboardW, &m_imgKeyboardH, &m_imgKeyboardCh, 0);
	if (!dataP){
		dev::Log("Keyboard Window: Failed to load keyboard image. Reason: {}\nPath: {}", stbi_failure_reason(), img_path);
		return false;
	}

	auto vramTexId = m_glUtils.InitTexture(m_imgKeyboardW, m_imgKeyboardH, GLUtils::Texture::Format::RGB);
	if (vramTexId == INVALID_ID) return false;
	m_vramTexId = vramTexId;

	m_glUtils.UpdateTexture(m_vramTexId, (uint8_t*)dataP);

	return true;
}

void dev::KeyboardWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	bool isRunning = dev::Signals::HW_RUNNING & _signals;

	m_windowFocused = ImGui::IsWindowFocused();

	DrawContext(isRunning);
}

struct ButtonTransform{
	float x,y,w,h;
	int scancode = SDL_SCANCODE_0;
	const char* hint = nullptr;
};

ButtonTransform buttons[] = {
	{103.0f + 63.25f * 0, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_SEMICOLON, "Hotkey: ';"},
	{103.0f + 63.25f * 1, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_1, "Hotkey: '1'"},
	{103.0f + 63.25f * 2, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_2, "hotkey: '2'"},
	{103.0f + 63.25f * 3, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_3, "Hotkey: '3'"},
	{103.0f + 63.25f * 4, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_4, "Hotkey: '4'"},
	{103.0f + 63.25f * 5, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_5, "Hotkey: '5'"},
	{103.0f + 63.25f * 6, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_6, "Hotkey: '6'"},
	{103.0f + 63.25f * 7, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_7, "Hotkey: '7'"},
	{103.0f + 63.25f * 8, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_8, "Hotkey: '8'"},
	{103.0f + 63.25f * 9, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_9, "Hotkey: '9'"},
	{103.0f + 63.25f * 10, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_0, "Hotkey: '0'"},
	{103.0f + 63.25f * 11, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_EQUALS, "Hotkey: '='"},
	{103.0f + 63.25f * 12, 304.0f, 62.0f, 60.0f, SDL_SCANCODE_SLASH, "Hotkey: '/'"},

	{135.0f + 63.25f * 0, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_J, "Hotkey: 'j'"},
	{135.0f + 63.25f * 1, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_C, "Hotkey: 'c'"},
	{135.0f + 63.25f * 2, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_U, "Hotkey: 'u'"},
	{135.0f + 63.25f * 3, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_K, "Hotkey: 'k'"},
	{135.0f + 63.25f * 4, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_E, "Hotkey: 'e'"},
	{135.0f + 63.25f * 5, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_N, "Hotkey: 'n'"},
	{135.0f + 63.25f * 6, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_G, "Hotkey: 'g'"},
	{135.0f + 63.25f * 7, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_LEFTBRACKET, "Hotkey: '['"},
	{135.0f + 63.25f * 8, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_RIGHTBRACKET, "Hotkey: ']'"},
	{135.0f + 63.25f * 9, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_Z, "Hotkey: 'z'"},
	{135.0f + 63.25f * 10, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_H, "Hotkey: 'h'"},
	{135.0f + 63.25f * 11, 369.0f, 62.0f, 60.0f, SDL_SCANCODE_APOSTROPHE, "Hotkey: ':'"},

	{103.0f + 63.25f * 0, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_LCTRL, "Hotkey: Left ctrl"},
	{103.0f + 63.25f * 1, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_F, "Hotkey: 'f'"},
	{103.0f + 63.25f * 2, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_Y, "Hotkey: 'y'"},
	{103.0f + 63.25f * 3, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_W, "Hotkey: 'w'"},
	{103.0f + 63.25f * 4, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_A, "Hotkey: 'a'"},
	{103.0f + 63.25f * 5, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_P, "Hotkey: 'p'"},
	{103.0f + 63.25f * 6, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_R, "Hotkey: 'r'"},
	{103.0f + 63.25f * 7, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_O, "Hotkey: 'o'"},
	{103.0f + 63.25f * 8, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_L, "Hotkey: 'l'"},
	{103.0f + 63.25f * 9, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_D, "Hotkey: 'd'"},
	{103.0f + 63.25f * 10, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_V, "Hotkey: 'v'"},
	{103.0f + 63.25f * 11, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_BACKSLASH, "Hotkey: '\'"},
	{103.0f + 63.25f * 12, 431.0f, 62.0f, 60.0f, SDL_SCANCODE_PERIOD, "Hotkey: '.'"},

	{106.0f + 63.25f * 0, 495.0f, 90.0f, 60.0f, SDL_SCANCODE_LSHIFT, "Hotkey: Left shift"},
	{199.0f + 63.25f * 0, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_Q, "Hotkey: 'q'"},
	{199.0f + 63.25f * 1, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_GRAVE, "Hotkey: '~"},
	{199.0f + 63.25f * 2, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_S, "Hotkey: 's'"},
	{199.0f + 63.25f * 3, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_M, "Hotkey: 'm'"},
	{199.0f + 63.25f * 4, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_J, "Hotkey: 'j'"},
	{199.0f + 63.25f * 5, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_T, "Hotkey: 't'"},
	{199.0f + 63.25f * 6, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_X, "Hotkey: 'x'"},
	{199.0f + 63.25f * 7, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_B, "Hotkey: 'b'"},
	{199.0f + 63.25f * 8, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_MINUS, "Hotkey: '-'"},
	{199.0f + 63.25f * 9, 495.0f, 62.0f, 60.0f, SDL_SCANCODE_COMMA, "Hotkey: ','"},
	{199.0f + 63.25f * 10, 495.0f, 90.0f, 60.0f, SDL_SCANCODE_RETURN, "Hotkey: Enter"},

	{106.0f + 93.0f * 0, 557.0f, 90.0f, 60.0f, SDL_SCANCODE_F6, "Hotkey: 'F6'"},
	{106.0f + 93.0f * 1, 557.0f, 90.0f, 60.0f, SDL_SCANCODE_TAB, "Hotkey: Tab"},
	{106.0f + 93.0f * 2, 557.0f, 440.0f, 60.0f, SDL_SCANCODE_SPACE, "Hotkey: Space"},
	{738.0f + 93.0f * 0, 557.0f, 90.0f, 60.0f, SDL_SCANCODE_RALT, "Hotkey: Right Alt"},
	{738.0f + 93.0f * 1, 557.0f, 90.0f, 60.0f, SDL_SCANCODE_BACKSPACE, "Hotkey: Backspace"},

	{989.0f + 62.25f * 0, 305.0f + 63.25f * 0, 62.0f, 60.0f, SDL_SCANCODE_F11, "Hotkey: 'F11'"},
	{989.0f + 62.25f * 1, 305.0f + 63.25f * 0, 62.0f, 60.0f, SDL_SCANCODE_F12, "Hotkey: 'F12'"},
	{989.0f + 62.25f * 2, 305.0f + 63.25f * 0, 62.0f, 60.0f, SDL_SCANCODE_F12, "Hotkey: 'F12'"},
	{989.0f + 62.25f * 0, 305.0f + 63.25f * 1, 62.0f, 60.0f, SDL_SCANCODE_F1, "Hotkey: 'F1'"},
	{989.0f + 62.25f * 1, 305.0f + 63.25f * 1, 62.0f, 60.0f, SDL_SCANCODE_F2, "Hotkey: 'F2'"},
	{989.0f + 62.25f * 2, 305.0f + 63.25f * 1, 62.0f, 60.0f, SDL_SCANCODE_F3, "Hotkey: 'F3'"},
	{989.0f + 62.25f * 0, 305.0f + 63.25f * 2, 62.0f, 60.0f, SDL_SCANCODE_F4, "Hotkey: 'F4'"},
	{989.0f + 62.25f * 1, 305.0f + 63.25f * 2, 62.0f, 60.0f, SDL_SCANCODE_F5, "Hotkey: 'F5'"},
	{989.0f + 62.25f * 2, 305.0f + 63.25f * 2, 62.0f, 60.0f, SDL_SCANCODE_ESCAPE, "Hotkey: Esc"},
	{989.0f + 62.25f * 0, 305.0f + 63.25f * 3, 62.0f, 60.0f, SDL_SCANCODE_F7, "Hotkey: 'F7'"},
	{989.0f + 62.25f * 1, 305.0f + 63.25f * 3, 62.0f, 60.0f, SDL_SCANCODE_UP, "Hotkey: 'UP'"},
	{989.0f + 62.25f * 2, 305.0f + 63.25f * 3, 62.0f, 60.0f, SDL_SCANCODE_F8, "Hotkey: 'F8'"},
	{989.0f + 62.25f * 0, 305.0f + 63.25f * 4, 62.0f, 60.0f, SDL_SCANCODE_LEFT, "Hotkey: Left arrow"},
	{989.0f + 62.25f * 1, 305.0f + 63.25f * 4, 62.0f, 60.0f, SDL_SCANCODE_DOWN, "Hotkey: Down arrow"},
	{989.0f + 62.25f * 2, 305.0f + 63.25f * 4, 62.0f, 60.0f, SDL_SCANCODE_RIGHT, "Hotkey: Right arrow"},
};

void dev::KeyboardWindow::DrawContext(const bool _isRunning)
{
	if (m_isGLInited)
	{
		auto imagePos = ImGui::GetCursorPos();

		ImGuiStyle& style = ImGui::GetStyle();
		auto wMax = ImGui::GetWindowWidth() - style.FramePadding.x * 4;
		auto hMax = ImGui::GetWindowHeight() - style.FramePadding.y * 14;

		ImVec2 displaySize;
		displaySize.x = wMax;
		displaySize.y = displaySize.x * KEYBOARD_IMG_ASPECT;
		if (displaySize.y > hMax)
		{
			displaySize.y = hMax;
			displaySize.x = displaySize.y / KEYBOARD_IMG_ASPECT;
		}

		ImGui::Image(m_vramTexId, displaySize);
		m_displayIsHovered = ImGui::IsItemHovered();

		// Draw buttons
		int buttonsCount = sizeof(buttons) / sizeof(ButtonTransform);
		for( int i = 0; i < buttonsCount; i++)
		{
			float scale = displaySize.x / m_imgKeyboardW;

			float buttonW = buttons[i].w * scale;
			float buttonH = buttons[i].h * scale;
			float buttonX = imagePos.x + buttons[i].x * scale;
			float buttonY = imagePos.y + buttons[i].y * scale;

			auto action = dev::DrawTransparentButtonWithBorder(std::format("##key{}", i).c_str(), {buttonX, buttonY}, {buttonW, buttonH}, buttons[i].hint);
			if (action != dev::ButtonAction::NONE)
			{
				std::string keyS {buttons[i].hint};
				std::string actionS {action == dev::ButtonAction::PRESSED ? "SDL_EVENT_KEY_DOWN" : "SDL_EVENT_KEY_UP"};

				m_hardware.Request(Hardware::Req::KEY_HANDLING, { { "scancode", buttons[i].scancode }, { "action", action == dev::ButtonAction::PRESSED ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP} });
			};
		}
	}
}

bool dev::KeyboardWindow::IsFocused() const
{
	return m_windowFocused;
}