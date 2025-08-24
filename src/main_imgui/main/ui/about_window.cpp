#include <format>
#include "ui/about_window.h"
#include "utils/str_utils.h"
#include "version.h"

dev::AboutWindow::AboutWindow(
	dev::Scheduler& _scheduler,
	bool* _visibleP,
	const float* const _dpiScaleP)
	:
	BaseWindow("About Devector", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP, _dpiScaleP,
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse)
{}

void dev::AboutWindow::Draw(
	const dev::Signals _signals, dev::Scheduler::SignalData _data)
{
	SetWindowPos(WinPosPreset::CENTER);
	DrawContext();
}

void dev::AboutWindow::DrawContext()
{
	ImGui::BeginChild("##aboutchframe",
		{ DEFAULT_WINDOW_W, 205}, ImGuiChildFlags_FrameStyle);

	ImGui::Text(
		"Devector is an emulator of the Soviet personal computer\n"
		"Vector 06C, offering advanced debugging options to enhance \n"
		"your development experience.");
	ImGui::Dummy({1, 5});
	ImGui::Text(
		"Your feedback is invaluable, whether positive or negative. \n"
		"Please share your thoughts using the Feedback window. \n"
		"Your contributions and support help improve this emulator.");
	ImGui::Dummy({ 1, 20 });
	ImGui::Text("Please join our Telegram channel");
	ImGui::SameLine();
	if (dev::HyperLink("http://t.me/devector06C"))
	{
		dev::OsOpenInShell("http://t.me/devector06C");
	}
	ImGui::Text("for updates and discussions.");

	//ImGui::Separator();

	//ImGui::Text(
	//	"Devector - эмулятор советского персонального компьютера\n"
	//	"Vector 06Ц, со встроенными возможностями отладки \n"
	//	"для улучшения вашего опыта разработки.");
	//ImGui::Dummy({ 1, 5 });
	//ImGui::Text(
	//	"Ваши положительные и даже отрицательные отзывы бесценны. \n"
	//	"Пожалуйста, поделитесь вашими предложениями в окне Feedback. \n"
	//	"Это поможет сделать эмулятор лучше.");
	//ImGui::Dummy({ 1, 20 });
	//ImGui::Text("Подключайтесь в");
	//ImGui::SameLine();
	//if (dev::HyperLink("Telegram канал"))
	//{
	//	dev::OsOpenInShell("http://t.me/devector06C");
	//}
	//ImGui::SameLine();
	//ImGui::Text("для дальнейших обновлений и обсуждений.");

	ImGui::EndChild();

	ImGui::Separator();
	ImGui::Dummy({ 1, 5 });
	ImGui::Text("Developed by Alexander Fedotovskikh");
	ImGui::Text("Build details:\n"
				"version %s, built on %s", APP_VERSION , __DATE__);
}