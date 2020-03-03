#include <B+/App.h>
#include <B+/TomlIO.h>

//Runs unit tests for BPlus.
//Uses the "acutest" single-header unit test library: https://github.com/mity/acutest

#include <acutest.h>

//AllTests.h is an auto-generated code file.
//It is generated as a pre-build step in Visual Studio,
//    so if you're getting an error about it not existing,
//    just hit "Compile" to fix.
#include "AllTests.h"

namespace
{
    std::unique_ptr<Bplus::ConfigFile> Config;
    std::unique_ptr<Bplus::App> App;

    thread_local std::string errorMsgBuffer;

    void OnError(const std::string& msg)
    {
        errorMsgBuffer = msg;
        App->Quit(true);
        throw std::exception(errorMsgBuffer.c_str());
    }
}


class MyConfigFile : public Bplus::ConfigFile
{
public:

    MyConfigFile(const fs::path& filePath, bool disableWrite)
        : Bplus::ConfigFile(filePath, ::OnError, disableWrite)
    {

    }

    virtual void ResetToDefaults() override
    {
        Bplus::ConfigFile::ResetToDefaults();
    }

protected:

    virtual void _FromToml(const toml::Value& document) override
    {

    }
    virtual void _ToToml(toml::Value& document) const override
    {

    }
};


class MyApp : public Bplus::App
{
public:

    MyApp() : Bplus::App(*::Config, ::OnError) { }

protected:

    virtual void ConfigureMainWindow(int& flags, std::string& title)
    {
        App::ConfigureMainWindow(flags, title);
        title = "My Unit tests App";
    }
    virtual void ConfigureOpenGL(bool& doubleBuffering,
                                 int& depthBits, int& stencilBits,
                                 Bplus::GL::VsyncModes& vSyncMode)
    {
        App::ConfigureOpenGL(doubleBuffering, depthBits, stencilBits, vSyncMode);
    }

    virtual void OnRendering(float deltaT)
    {
        auto& context = GetContext();
        context.Clear(1, 1, 1, 1,
                      1);

        ImGui::Text("My Unit-Test App");
    }
};


void SetUpBplusApp()
{
    //For unit-testing apps, don't write to the config file.
    //TODO: DO write to the config file, and add a final unit test that checks that config exists and has the expected values.

    Config = std::make_unique<MyConfigFile>(fs::current_path() / "Config.toml", true);

    App = std::make_unique<MyApp>();
    App->Run();
}

void TesterTest()
{
    std::cout << "Test\n";
    char d;
    std::cin >> d;

    std::cout << "You entered " << d << "\n";
}