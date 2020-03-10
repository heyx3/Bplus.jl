#pragma once

#include <random>

#include <B+/App.h>
#include <B+/TomlIO.h>

#define TEST_NO_MAIN
#include <acutest.h>


namespace Simple
{
    thread_local std::unique_ptr<Bplus::ConfigFile> Config;
    thread_local std::unique_ptr<Bplus::App> App;

    thread_local std::string errorMsgBuffer;

    void OnError(const std::string& msg)
    {
        errorMsgBuffer = msg;
        App->Quit(true);
        throw std::exception(errorMsgBuffer.c_str());
    }
}


class SimpleConfigFile : public Bplus::ConfigFile
{
public:

    SimpleConfigFile(const fs::path& filePath, bool disableWrite)
        : Bplus::ConfigFile(filePath, Simple::OnError, disableWrite)
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


class SimpleApp : public Bplus::App
{
public:

    std::function<void()> _OnQuit;
    std::function<void(float)> _OnUpdate, _OnRender;

    SimpleApp(std::function<void(float)> onUpdate,
              std::function<void(float)> onRender,
              std::function<void()> onQuit)
        : Bplus::App(*Simple::Config, Simple::OnError),
          _OnUpdate(onUpdate), _OnRender(onRender), _OnQuit(onQuit)
    {

    }

protected:

    virtual void ConfigureMainWindow(int& flags, std::string& title) override
    {
        App::ConfigureMainWindow(flags, title);
        title = "My Unit tests App";
    }
    virtual void ConfigureOpenGL(bool& doubleBuffering,
                                 int& depthBits, int& stencilBits,
                                 Bplus::GL::VsyncModes& vSyncMode) override
    {
        App::ConfigureOpenGL(doubleBuffering, depthBits, stencilBits, vSyncMode);
    }

    virtual void OnUpdate(float deltaT)
    {
        Bplus::App::OnUpdate(deltaT);

        _OnUpdate(deltaT);
    }
    virtual void OnRendering(float deltaT)
    {
        Bplus::App::OnRendering(deltaT);

        _OnRender(deltaT);

    }
    virtual void OnQuit(bool force) override
    {
        _OnQuit();
        Bplus::App::OnQuit(force);
    }
};



namespace Simple
{
    //Runs a SimpleApp with the given logic.
    void Run(std::function<void(float)> onUpdate,
             std::function<void(float)> onRender,
             std::function<void()> onQuit)
    {
        //Swap out the BPAssert function with something that hooks into acutest.
        auto oldAssertFunc = Bplus::GetAssertFunc();
        Bplus::SetAssertFunc([](bool condition, const char* msg)
        {
            TEST_CHECK_(condition, "BPAssert: %s", msg);

            //If the test failed, stop running the app.
            if (!condition && App.get() != nullptr)
                App->Quit(true);
        });

        //For unit-testing apps, don't write to the config file.
        //TODO: DO write to the config file, and add a final unit test that checks that config exists and has the expected values.
        Config = std::make_unique<SimpleConfigFile>(fs::current_path() / "Config.toml", true);
        App = std::make_unique<SimpleApp>(onUpdate, onRender, onQuit);
        App->Run();

        //Restore the previous assertion function.
        Bplus::SetAssertFunc(oldAssertFunc);
    }

    //Returns a random double between 0 and 1.
    double Rng() { return rngDistribution(rngEngine); }


    std::mt19937 rngEngine;
    std::uniform_real_distribution<double> rngDistribution;
}
std::mt19937 Simple::rngEngine{ std::random_device{}() };
std::uniform_real_distribution<double> Simple::rngDistribution{ 0, 1 };