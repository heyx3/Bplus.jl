#pragma once

#include <B+/App.h>
#include <B+/TomlIO.h>


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
    void Run(std::function<void(float)> onUpdate,
             std::function<void(float)> onRender,
             std::function<void()> onQuit)
    {
        //For unit-testing apps, don't write to the config file.
        //TODO: DO write to the config file, and add a final unit test that checks that config exists and has the expected values.

        Config = std::make_unique<SimpleConfigFile>(fs::current_path() / "Config.toml", true);

        App = std::make_unique<SimpleApp>(onUpdate, onRender, onQuit);
        App->Run();
    }
}