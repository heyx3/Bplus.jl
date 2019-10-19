#include "Config.h"


void to_json(nlohmann::json& json, const Config& cfg)
{
    json = nlohmann::json {
        { "LastEditingDir", cfg.LastEditingDir },
        { "WasWindowMaximized", cfg.WasWindowMaximized },
        { "LastWindowWidth", cfg.LastWindowWidth },
        { "LastWindowHeight", cfg.LastWindowHeight },
    };
}
void from_json(const nlohmann::json& json, Config& cfg)
{
    cfg.LastEditingDir = json.value("LastEditingDir", "");
    cfg.WasWindowMaximized = json.value("WasWindowMaximized", true);
    cfg.LastWindowWidth = json.value("LastWindowWidth", 1280);
    cfg.LastWindowHeight = json.value("LastWindowHeight", 720);
}

void Config::Initialize()
{
    //If the previous editing directory doesn't exist anymore, forget it.
    if (LastEditingDir.empty() && !fs::is_directory(LastEditingDir))
        LastEditingDir = "";

    //If the last editing directory isn't set, default to the program's directory.
    if (LastEditingDir.empty())
        LastEditingDir = fs::current_path();
}