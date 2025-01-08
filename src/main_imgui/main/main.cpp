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
        "The path to the settings.", false, dev::GetExecutableDir() + "settings.json");

    auto rom_fdd_recPath = argsParser.GetString("path",
        "The path to the rom/fdd/rec file.", false, "");

    if (!rom_fdd_recPath.empty() && !dev::IsFileExist(rom_fdd_recPath)){
        dev::Log("A path is invalid: {}", rom_fdd_recPath);
        rom_fdd_recPath = "";
    }

    if (!argsParser.IsRequirementSatisfied())
    {
        dev::Log("---Settings parameters are missing");
    }

    nlohmann::json settingsJ;
    if (dev::IsFileExist(settingsPath) == false)
    {
        dev::Log("The settings wasn't found. Created new default settings: {}", settingsPath);
    }
    else {
        settingsJ = dev::LoadJson(settingsPath);
    }

    auto app = dev::DevectorApp(settingsPath, settingsJ, rom_fdd_recPath);
    if (!app.IsInited()) return (int)app.GetError();
    app.Run();

	return (int)dev::ErrCode::NO_ERRORS;
}