#include "utils/argsParser.h"
#include "utils/globals.h"
#include "utils/utils.h"
#include "utils/jsonUtils.h"
#include "devectorApp.h"
#include "consts.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of 
// testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW 
// that is adequate for your version of Visual Studio.
//#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
//#pragma comment(lib, "legacy_stdio_definitions")
//#endif

int main(int argc, char** argv)
{
    // TODO: update
    dev::ArgsParser argsParser(argc, argv,
        "This is an emulator of the Soviet personal computer Vector06C. It has built-in debugger functionality. It also expose debugging options to VS Code via the debugging protocall. Please, install VS Code extension...");
    
    const auto settingsPath = argsParser.GetString("settings",
        "The path to the settings.");
    
    if (!argsParser.IsRequirementSatisfied())
    {
        dev::Exit("Required parameters are missing", dev::ERROR_UNSPECIFIED);
    }

    if (dev::IsFileExist(settingsPath) == false)
    {
        auto msg = std::format("settingsPath is invalid. file path: {}",
            settingsPath);
        dev::Exit(msg, dev::ERROR_UNSPECIFIED);
    }

    auto settingsJ = dev::LoadJson(settingsPath);
    auto mainWindowWidth = dev::GetJsonInt(settingsJ, "mainWindowWidth", false, dev::MAIN_WINDOW_W);
    auto mainWindowHeight = dev::GetJsonInt(settingsJ, "mainWindowHeight", false, dev::MAIN_WINDOW_H);

    auto app = dev::DevectorApp(settingsJ, mainWindowWidth, mainWindowHeight);
    if (app.Inited() == false) return dev::ERROR_UNSPECIFIED;
    
    app.Run();

	return dev::NO_ERRORS;
}