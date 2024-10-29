#pragma once

#include <stdio.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

#include "utils/glu_utils.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
	#include <SDL3/SDL_opengles2.h>
#else
	#include <SDL3/SDL_opengl.h>
#endif

#include "utils/json_utils.h"

namespace dev {

	class ImGuiApp
	{
	public:
		enum class AppStatus {
			NOT_INITED = 0,
			INITED,
			RUN,
			REQ_PREPARE_FOR_EXIT,
			PREPARE_FOR_EXIT,
			EXIT,
		};

		ImGuiApp(nlohmann::json _settingsJ, const std::string& _stringPath, const std::string& _title = "New Window");
		~ImGuiApp();

		void Run();
		bool IsInited() const { return m_status == AppStatus::INITED || m_status == AppStatus::RUN; };
		auto GetStatus() const -> AppStatus { return m_status; };
		auto GetError() const -> ErrCode { return m_error; };

		virtual void Update() {};

	protected:
		const std::string m_stringPath;
		int m_width;
		int m_height;
		int m_posX;
		int m_posY;
		std::string m_title;
		ImVec4 m_backColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
		AppStatus m_status = AppStatus::NOT_INITED;
		dev::ErrCode m_error = dev::ErrCode::NO_ERRORS;

		static void glfw_error_callback(int _error, const char* _description);
		SDL_Window* m_window = nullptr;
		SDL_GLContext m_gl_context = nullptr;
		ImGuiIO* m_ioP = nullptr;

		static constexpr double AUTO_UPDATE_COOLDOWN = 2.0;

		ImFont* m_font = nullptr;
		ImFont* m_fontItalic = nullptr;
		float m_dpiScale = 1.0f;

		enum class Req { LOAD_FONT, CHECK_WINDOW_SIZE_POS, };
		TQueue <std::pair<Req, int64_t>> m_reqs; // request
		std::thread m_autoUpdateThread;

		bool m_prepare_for_exit = false;

		// reqs
		void AutoUpdate();
		void Request(const Req _req, const int64_t _val = 0);
		void ReqHandling();
		void LoadFonts();
		void SettingsUpdate(const std::string& _fieldName, nlohmann::json _json);
		void SettingsSave(const std::string& _path);
		auto GetSettingsString(const std::string& _fieldName, const std::string& _defaultValue) -> std::string;
		auto GetSettingsObject(const std::string& _fieldName ) -> nlohmann::json;
		int GetSettingsInt(const std::string& _fieldName, int _defaultValue);
		bool GetSettingsBool(const std::string& _fieldName, bool _defaultValue);
		float GetDpiScale();

	private:
		std::mutex m_settingsMutex;
		nlohmann::json m_settingsJ;
	};
}