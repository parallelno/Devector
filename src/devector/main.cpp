#include <format>

#include "utils/args_parser.h"
#include "utils/consts.h"
#include "utils/utils.h"
#include "utils/str_utils.h"
#include "utils/json_utils.h"
#include "utils/consts.h"
#include "devector_app.h"

int main(int argc, char** argv)
{
    dev::ArgsParser argsParser(argc, argv,
        "This is an emulator of the Soviet personal computer Vector06C. It has built-in debugger functionality.");
    
    auto settingsPath = argsParser.GetString("settingsPath",
        "The path to the settings.", false, "settings.json");
    
    if (!argsParser.IsRequirementSatisfied())
    {
        dev::Log("---Settings parameters are missing");
    }

    nlohmann::json settingsJ;
    if (dev::IsFileExist(dev::StrToStrW(settingsPath)) == false)
    {
        dev::Log("The settings wasn't found. Created new default settings: {}", settingsPath);
    }
    else {
        settingsJ = dev::LoadJson(settingsPath);
    }

    auto app = dev::DevectorApp(settingsPath, settingsJ);
    if (!app.IsInited()) return (int)app.GetError();
    app.Run();

	return (int)dev::ErrCode::NO_ERRORS;
}