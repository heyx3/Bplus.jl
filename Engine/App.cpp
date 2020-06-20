#include "App.h"

#include <chrono>
#include <thread>

using namespace Bplus;


ConfigFile::ConfigFile(const fs::path& tomlFile,
                       ErrorCallback onError, bool disableWrite)
    : FilePath(tomlFile), DisableWrite(disableWrite),
      OnError(onError)
{
}

void ConfigFile::LoadFromFile()
{
    //If the file doesn't exist, fall back to default values.
    if (!fs::exists(FilePath))
    {
        ResetToDefaults();
        return;
    }

    auto tryParse = toml::parseFile(FilePath.string());
    if (!tryParse.valid())
    {
        OnError(std::string() + "Error reading/parsing TOML config file: " +
                    tryParse.errorReason);
        return;
    }

    auto tomlDoc = tryParse.value;

    try
    {
        FromToml(tomlDoc);
    }
    catch (...)
    {
        OnError("Unknown error loading TOML config file");
    }
}
void ConfigFile::WriteToFile() const
{
    if (DisableWrite)
        return;

    try
    {
        toml::Value doc;
        ToToml(doc);

        std::ofstream fileStream(FilePath, std::ios::trunc);
        if (fileStream.is_open())
            doc.writeFormatted(&fileStream, toml::FORMAT_INDENT);
        else
            OnError(std::string("Error opening config file to write: ") + FilePath.string());
    }
    catch (...)
    {
        OnError("Error writing updated config file");
    }
}

void ConfigFile::FromToml(const toml::Value& document)
{
    IsWindowMaximized = IO::TomlTryGet(document, "IsWindowMaximized", false);

    const auto* found = document.find("WindowSize");
    if (found != nullptr)
    {
        WindowSize.x = (glm::u32)found->get<int64_t>(0);
        WindowSize.y = (glm::u32)found->get<int64_t>(1);
    }

    this->_FromToml(document);
    this->OnDeserialized();
}
void ConfigFile::ToToml(toml::Value& document) const
{
    document["IsWindowMaximized"] = IsWindowMaximized;
    
    auto windowSize = toml::Array();
    windowSize.push_back((int64_t)WindowSize.x);
    windowSize.push_back((int64_t)WindowSize.y);
    document["WindowSize"] = std::move(windowSize);
}

void ConfigFile::ResetToDefaults()
{
    IsWindowMaximized = false;
    WindowSize = { 800, 600 };
}



App::App(ConfigFile& config, ErrorCallback onError)
    : MainWindow(nullptr), glContext(nullptr), ImGuiContext(nullptr),
      WorkingPath(config.FilePath.parent_path()),
      ContentPath(WorkingPath / "content"),
      Config(config), OnError(onError)
{
    isRunning = false;
    Config.OnError = OnError;
}
App::~App()
{
    if (isRunning)
    {
        //Force-quit, and be careful that an exception isn't thrown.
        try { Quit(true); }
        catch (...) {}
    }
}

void App::Run()
{
    timeSinceLastPhysicsUpdate = 0;
    lastFrameStartTime = SDL_GetPerformanceCounter();
    isRunning = true;

    Config.LoadFromFile();

    #pragma region Initialization

    //Set up SDL.
    if (!TrySDL(SDL_Init(SDL_INIT_VIDEO), "Couldn't initialize SDL"))
        return;

    //Set up the window.
    int windowFlags;
    std::string windowTitle;
    ConfigureMainWindow(windowFlags, windowTitle);
    MainWindow = SDL_CreateWindow(windowTitle.c_str(),
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  (int)Config.WindowSize.x,
                                  (int)Config.WindowSize.y,
                                  windowFlags);
    if (!TrySDL(MainWindow, "Error creating main window"))
        return;

    //Configure SDL/OpenGL.
    bool doubleBuffer;
    int depthBits, stencilBits;
    GL::VsyncModes vSyncMode(GL::VsyncModes::Off);
    ConfigureOpenGL(doubleBuffer, depthBits, stencilBits, vSyncMode);
    if (!TrySDL(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, doubleBuffer ? 1 : 0), "Error setting double-buffering") ||
        !TrySDL(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits), "Error setting back buffer's depth bits") ||
        !TrySDL(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits), "Error setting back buffer's stencil bits"))
    {
        return;
    }

    //Initialize OpenGL.
    std::string errMsg;
    glContext = std::make_unique<GL::Context>(MainWindow, errMsg);
    if (errMsg.size() > 0)
    {
        OnError(errMsg);
        return;
    }

    //Set up V-Sync.
    if (!glContext->SetVsyncMode(vSyncMode))
        TrySDL(-1, "Couldn't set up vsync properly");

    //Initialize Dear ImGUI.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiContext = &ImGui::GetIO();
    ImGuiContext->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark(); //Default to dark theme, for sanity
    ConfigureImGUISDL();
    ConfigureImGUIOpenGL();
    
    //Allow child class initialization.
    OnBegin();

    #pragma endregion

    //Now, run the main loop.
    while (isRunning)
    {
        //Process window/OS events.
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent) != 0)
        {
            //Update ImGui.
            ImGuiSDL->ProcessEvent(sdlEvent);
            
            //Update this app's base functionality.
            switch (sdlEvent.type)
            {
                case SDL_EventType::SDL_QUIT:
                    OnQuit(false);
                break;

                case SDL_EventType::SDL_WINDOWEVENT:
                    switch (sdlEvent.window.event)
                    {
                        case SDL_WINDOWEVENT_CLOSE:
                            OnQuit(false);
                        break;

                        case SDL_WINDOWEVENT_RESIZED:
                            if (sdlEvent.window.data1 < (Sint32)MinWindowSize.x ||
                                sdlEvent.window.data2 < (Sint32)MinWindowSize.y)
                            {
                                SDL_SetWindowSize(
                                    MainWindow,
                                    std::max(sdlEvent.window.data1, (int)MinWindowSize.x),
                                    std::max(sdlEvent.window.data2, (int)MinWindowSize.y));
                            }
                        break;

                        default: break;
                    }
                break;

                default: break;
            }

            //Update the config.
            Config.IsWindowMaximized = (SDL_GetWindowFlags(MainWindow) &
                                            SDL_WINDOW_MAXIMIZED) != 0;
            if (!Config.IsWindowMaximized)
            {
                int wndW, wndH;
                SDL_GetWindowSize(MainWindow, &wndW, &wndH);
                Config.WindowSize.x = (glm::u32)wndW;
                Config.WindowSize.y = (glm::u32)wndH;
            }

            //Update the child class.
            OnEvent(sdlEvent);
        }

        //Exit early if the app is quitting.
        if (!isRunning)
            continue;

        //Now run an update frame of the App.
        uint64_t newFrameTime = SDL_GetPerformanceCounter();
        double deltaT = (newFrameTime - lastFrameStartTime) /
            (double)SDL_GetPerformanceFrequency();

        //If the frame-rate is too fast, wait.
        if (deltaT < MinDeltaT)
        {
            double missingTime = MinDeltaT - deltaT;
            std::this_thread::sleep_for(std::chrono::duration<double>(missingTime + .00000001));
            continue;
        }
        lastFrameStartTime = newFrameTime;

        //Initialize the GUI frame.
        ImGuiOpenGL->BeginFrame();
        ImGuiSDL->BeginFrame((float)deltaT);
        ImGui::NewFrame();

        //Update physics.
        timeSinceLastPhysicsUpdate += deltaT;
        while (timeSinceLastPhysicsUpdate > PhysicsTimeStep)
        {
            timeSinceLastPhysicsUpdate -= PhysicsTimeStep;
            OnPhysics(PhysicsTimeStep);
        }

        //Update other stuff.
        OnUpdate((float)deltaT);

        //Exit early if the app is quitting.
        if (!isRunning)
            continue;

        //Do rendering.
        glContext->SetViewport((int)ImGuiContext->DisplaySize.x,
                               (int)ImGuiContext->DisplaySize.y);
        OnRendering((float)deltaT);

        //Finally, do GUI rendering.
        ImGui::Render();
        ImGuiOpenGL->RenderFrame();
        SDL_GL_SwapWindow(MainWindow);
    }
}
void App::OnQuit(bool force)
{
    //Prevent an ImGUI error by properly ending the frame.
    ImGui::Render();
    ImGuiOpenGL->RenderFrame();

    //Clean up ImGUI.
    ImGuiOpenGL.reset();
    ImGuiSDL.reset();

    //Clean up the window's OpenGL context.
    glContext.reset();

    //Clean up the window.
    if (MainWindow != nullptr)
        SDL_DestroyWindow(MainWindow);
    MainWindow = nullptr;

    //Clean up SDL itself.
    if ((SDL_WasInit(SDL_INIT_EVERYTHING) & SDL_INIT_EVENTS) != 0)
        SDL_Quit();

    //Write out the new config file.
    Config.WriteToFile();
    isRunning = false;
}

bool App::TrySDL(int returnCode, const std::string& msgPrefix)
{
    if (returnCode != 0)
        OnError(msgPrefix + ": " + SDL_GetError());

    return returnCode == 0;
}
bool App::TrySDL(void* shouldntBeNull, const std::string& msgPrefix)
{
    if (shouldntBeNull == nullptr)
        return TrySDL(-1, msgPrefix);
    else
        return true;
}