#include "AboutWindow.h"

#include <format>
#include "Utils/ImGuiUtils.h"
#include "Utils/StrUtils.h"

dev::AboutWindow::AboutWindow(const float* const _fontSizeP, const float* const _dpiScaleP)
	:
	BaseWindow("About Devector", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _fontSizeP, _dpiScaleP)
{}

void dev::AboutWindow::Update(bool& _visible)
{
	BaseWindow::Update();

	if (_visible) 
	{
		ImVec2 center = ImGui::GetMainViewport()->GetCenter(); 	// Always center this window when appearing
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::Begin(m_name.c_str(), &_visible, 
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking | 
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse))
		{
			Draw();
			ImGui::End();
		}
	}
}

void dev::AboutWindow::Draw()
{
	ImGui::BeginChild("##aboutchframe", { DEFAULT_WINDOW_W, 205}, ImGuiChildFlags_FrameStyle);

	ImGui::Text("Devector is an emulator of the Soviet personal computer\nVector 06C, offering advanced debugging options to enhance \nyour development experience.");
	ImGui::Dummy({1, 5});
	ImGui::Text("Your feedback is invaluable, whether positive or negative. \nPlease share your thoughts using the Feedback window. \nYour contributions and support help improve this emulator.");
	ImGui::Dummy({ 1, 20 });
	ImGui::Text("Please join our Telegram channel");
	ImGui::SameLine();
	if (dev::HyperLink("http://t.me/devector06C"))
	{
		dev::OsOpenInShell("http://t.me/devector06C");
	}
	ImGui::Text("for updates and discussions.");

	//ImGui::Separator();

	//ImGui::Text("Devector - эмулятор советского персонального компьютера\nVector 06Ц, со встроенными возможностями отладки \nдля улучшения вашего опыта разработки.");
	//ImGui::Dummy({ 1, 5 });
	//ImGui::Text("Ваши положительные и даже отрицательные отзывы бесценны. \nПожалуйста, поделитесь вашими предложениями в окне Feedback. \nЭто поможет сделать эмулятор лучше.");
	//ImGui::Dummy({ 1, 20 });
	//ImGui::Text("Подключайтесь в");
	//ImGui::SameLine();
	//if (dev::HyperLink("Telegram канал"))
	//{
	//	dev::OsOpenInShell("http://t.me/devector06C");
	//}
	//ImGui::SameLine(); ImGui::Text("для дальнейших обновлений и обсуждений.");

	ImGui::EndChild();

	ImGui::Separator();
	ImGui::Dummy({ 1, 5 });
	ImGui::Text("Developed by Alexander Fedotovskikh");
	ImGui::Text("Build details: %s", compilation_date.c_str());
}