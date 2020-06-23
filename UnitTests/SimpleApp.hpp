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

    //Used for OpenGL debugging callbacks.
    void GLAPIENTRY OnOglMsg(GLenum source, GLenum type,
                             GLuint id, GLenum severity,
                             GLsizei msgLength, const GLchar* msg,
                             const void* userData)
    {
        bool isFatal = false;

        //Generate relevant pieces of text describing the message.
        std::string sourceStr, typeStr, severityStr;
        switch (source)
        {
            case GL_DEBUG_SOURCE_API: sourceStr = "calling a 'gl' method"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "calling an SDL-related method"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "compiling a shader"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY: sourceStr = "within some internal OpenGL app"; break;
            case GL_DEBUG_SOURCE_APPLICATION: sourceStr = "a manual, user-raised error call ('glDebugMessageInsert()')"; break;
            case GL_DEBUG_SOURCE_OTHER: sourceStr = "some unspecified source"; break;

            default:
                sourceStr = "[error: unexpected source ";
                sourceStr += Bplus::ToStringInBase(source, 16, "0x");
                sourceStr += "]";
            break;
        }
        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR:
                isFatal = true;
                typeStr = "error";
            break;

            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "deprecated usage"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "undefined behavior detected"; break;
            case GL_DEBUG_TYPE_PORTABILITY: typeStr = "non-portable behavior detected"; break;
            case GL_DEBUG_TYPE_PERFORMANCE: typeStr = "suboptimal performance detected"; break;
            case GL_DEBUG_TYPE_MARKER: typeStr = "command stream annotation event"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP: typeStr = "BEGIN group"; break;
            case GL_DEBUG_TYPE_POP_GROUP: typeStr = "END group"; break;
            case GL_DEBUG_TYPE_OTHER: typeStr = "unspecified event"; break;

            default:
                typeStr = "[error: unexpected event type ";
                typeStr += Bplus::ToStringInBase(type, 16, "0x");
                typeStr += "]";
            break;
        }
        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH:
                isFatal = true;
                severityStr = "Severe";
            break;

            case GL_DEBUG_SEVERITY_MEDIUM: severityStr = "Concerning"; break;
            case GL_DEBUG_SEVERITY_LOW: severityStr = "Mild"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = ""; break;

            default:
                severityStr = "[error: unexpected severity ";
                severityStr += Bplus::ToStringInBase(severity, 16, "0x");
                severityStr += "]";
            break;
        }

        //Put the pieces together into a coherent string.
        std::string generatedMsg;
        if (severityStr.size() > 0)
            generatedMsg += severityStr + " ";
        else
            typeStr[0] = std::toupper(typeStr[0]);
        generatedMsg += typeStr + " from " + sourceStr + ": ";
        generatedMsg.append(msg, msgLength);

        //If the event is bad, make it an error.
        //Otherwise, just print it to stdout.
        if (isFatal)
            BPAssert(false, generatedMsg.c_str());
        else
            std::cout << "\t\t" << generatedMsg << "\n";
    }
}


class SimpleConfigFile : public Bplus::ConfigFile
{
public:

    SimpleConfigFile(const fs::path& filePath, bool disableWrite)
        : Bplus::ConfigFile(filePath, Simple::OnError, disableWrite)
    {
        WindowSize = { 1000, 1000 };
    }

    virtual void ResetToDefaults() override
    {
        Bplus::ConfigFile::ResetToDefaults();
        WindowSize = { 1000, 1000 };
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

    virtual void OnBegin()
    {
        //Add an error/debug-message handler for OpenGL that asserts "false".
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(Simple::OnOglMsg, 0);
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
    //Runs a SimpleApp with the given logic for unit testing.
    void Run(std::function<void(float)> onUpdate,
             std::function<void(float)> onRender,
             std::function<void()> onQuit)
    {
        //Swap out the BPAssert function with something that hooks into acutest.
        auto oldAssertFunc = Bplus::GetAssertFunc();
        Bplus::SetAssertFunc([](bool condition, const char* msg)
        {
            //Originally I put a TEST_CHECK_ here, but
            //    sometimes we want to test that something fails as expected,
            //    so instead we'll go with the more flexible exception.
            //Acutest can catch exceptions.

            if (!condition)
            {
                std::string errorMsg = "Assert failed: ";
                errorMsg += msg;
                throw std::exception(errorMsg.c_str());
            }
        });


        //For unit-testing apps, don't write to the config file.
        //TODO: DO write to the config file, and add a final unit test that checks that config exists and has the expected values.
        Config = std::make_unique<SimpleConfigFile>(fs::current_path() / "Config.toml", true);

        App = std::make_unique<SimpleApp>(onUpdate, onRender, onQuit);
        try
        {
            App->Run();
        }
        catch (std::exception e)
        {
            if (App->IsRunning())
                App->Quit(true);
        }

        //Restore the previous assertion function.
        Bplus::SetAssertFunc(oldAssertFunc);
    }

    //Starts a SimpleApp, runs the given test, then immediately quits.
    //TODO: Change current tests to use this.
    void RunTest(std::function<void()> test,
                 std::optional<std::function<void()>> onQuit = std::nullopt)
    {
        Run([&](float deltaT) { test(); App->Quit(true); },
            [&](float deltaT) { },
            [&]() { if (onQuit.has_value()) onQuit.value()(); });
    }


    std::mt19937 rngEngine{ std::random_device{}() };
    std::uniform_real_distribution<double> rngDistribution{ 0, 1 };

    //Returns a random double between 0 and 1.
    double Rng() { return rngDistribution(rngEngine); }
}