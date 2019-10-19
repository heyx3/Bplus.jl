#pragma once

#include "IO.h"


//Data loaded in from a file.
class Config
{
public:

    fs::path LastEditingDir;
    bool WasWindowMaximized;
    uint32_t LastWindowWidth, LastWindowHeight;

    //Finalizes config values after being deserialized.
    void Initialize();
};


//Helpers to easily convert the class to and from JSON.
void to_json(nlohmann::json& json, const Config& cfg);
void from_json(const nlohmann::json& json, Config& cfg);