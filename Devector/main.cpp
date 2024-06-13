#include "Utils/ArgsParser.h"
#include "Utils/Consts.h"
#include "Utils/Utils.h"
#include "Utils/StrUtils.h"
#include "Utils/JsonUtils.h"
#include "Utils/Consts.h"
#include "DevectorApp.h"
#include <format>

int main(int argc, char** argv)
{
    dev::ArgsParser argsParser(argc, argv,
        "This is an emulator of the Soviet personal computer Vector06C. It has built-in debugger functionality.");
    
    auto settingsPath = argsParser.GetString("settingsPath",
        "The path to the settings.");
    
    if (!argsParser.IsRequirementSatisfied())
    {
        dev::Exit("Required parameters are missing", dev::ERROR_UNSPECIFIED);
    }

    nlohmann::json settingsJ;
    if (dev::IsFileExist(dev::StrToStrW(settingsPath)) == false)
    {
        auto msg = std::format("settings file is not found, created a new one with default settings: {}",
            settingsPath);
        settingsPath = "settings.json";
    }
    else {
        settingsJ = dev::LoadJson(settingsPath);
    }

    auto app = dev::DevectorApp(settingsPath, settingsJ);
    if (!app.IsInited()) return dev::ERROR_UNSPECIFIED;
    app.Run();

	return dev::NO_ERRORS;
}