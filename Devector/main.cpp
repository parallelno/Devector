#include "Utils/ArgsParser.h"
#include "Utils/Globals.h"
#include "Utils/Utils.h"
#include "Utils/StringUtils.h"
#include "Utils/JsonUtils.h"
#include "Utils/Consts.h"
#include "DevectorApp.h"

int main(int argc, char** argv)
{
    // TODO: update
    dev::ArgsParser argsParser(argc, argv,
        "This is an emulator of the Soviet personal computer Vector06C. It has built-in debugger functionality. It also expose debugging options to VS Code via the debugging protocall. Please, install VS Code extension...");
    
    const auto settingsPath = argsParser.GetString("settingsPath",
        "The path to the settings.");
    
    if (!argsParser.IsRequirementSatisfied())
    {
        dev::Exit("Required parameters are missing", dev::ERROR_UNSPECIFIED);
    }

    if (dev::IsFileExist(dev::StrToStrW(settingsPath)) == false)
    {
        // TODO: make it working if there is no setting file (create default one with necessary settings)
        auto msg = std::format("settingsPath is invalid. file path: {}",
            settingsPath);
        dev::Exit(msg, dev::ERROR_UNSPECIFIED);
    }

    auto settingsJ = dev::LoadJson(settingsPath);
    auto mainWindowWidth = dev::GetJsonInt(settingsJ, "mainWindowWidth", false, dev::MAIN_WINDOW_W);
    auto mainWindowHeight = dev::GetJsonInt(settingsJ, "mainWindowHeight", false, dev::MAIN_WINDOW_H);

    auto app = dev::DevectorApp(settingsPath, settingsJ, mainWindowWidth, mainWindowHeight);
    if (!app.Inited()) return dev::ERROR_UNSPECIFIED;
    app.Run();

	return dev::NO_ERRORS;
}