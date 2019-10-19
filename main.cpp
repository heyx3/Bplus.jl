//Redefine "exit" to throw an exception instead of plain quitting,
//    so that destructors can still be called.
//#define exit(code) throw code

#include <memory>

#include "RenderLibs.h"
#include "Config.h"

#include <WFC++\Tiled3D\State.h>

#include "Apps/Base.h"


namespace
{
    //Relative to the executable path.
    const char* ConfigFile = "config.json";

    fs::path GetConfigFullPath() { return fs::current_path() / "config.json"; }
}


//The overarching stuff in the app, managed by RAII.
//Also offers some covenience stuff like clearing the window.
class AppResources
{
public:

    static const int OpenGLVersion_Major = 4,
                     OpenGLVersion_Minor = 0;
    static inline const char* OpenGLVersionStr = "#version 400";

    SDL_Window* Window = nullptr;

    SDL_GLContext OpenGL;
    ImGuiIO ImGuiContext;

    Config Config;
    bool WriteConfigOnQuit;

    ErrorCallback OnError;


    void OnWindowEvent(const SDL_WindowEvent& data)
    {
        //Update config settings about the window.
        Config.WasWindowMaximized = (SDL_GetWindowFlags(Window) & SDL_WINDOW_MAXIMIZED) != 0;
        if (!Config.WasWindowMaximized)
        {
            int wndW, wndH;
            SDL_GetWindowSize(Window, &wndW, &wndH);
            Config.LastWindowWidth = wndW;
            Config.LastWindowHeight = wndH;
        }
    }


    #pragma region Initialization/shutdown

    AppResources(const std::string& windowTitle, bool writeConfigOnQuit,
                 ErrorCallback onError)
    {
        WriteConfigOnQuit = writeConfigOnQuit;

        OnError = onError;

        //Load the config.
        if (!IO::ReadJsonFromFile(GetConfigFullPath(), Config, OnError))
            return;

        //Set up SDL.
        if (!TrySDL(SDL_Init(SDL_INIT_VIDEO), "Couldn't initialize SDL"))
            return;
        //Set up the window.
        auto windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
        if (Config.WasWindowMaximized)
            windowFlags |= SDL_WINDOW_MAXIMIZED;
        Window = SDL_CreateWindow(windowTitle.c_str(),
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  (int)Config.LastWindowWidth, (int)Config.LastWindowHeight,
                                  windowFlags);
        if (!TrySDL(Window, "Error creating window"))
            return;

        //Initialize OpenGL.
        if (!TrySDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OpenGLVersion_Major), "Error setting GL major version") ||
            !TrySDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OpenGLVersion_Minor), "Error setting GL minor version") ||
            !TrySDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), "Error setting context profile") ||
            !TrySDL(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "Error setting double-buffering") ||
            !TrySDL(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24), "Error setting back buffer's depth bits") ||
            !TrySDL(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8), "Error setting back buffer's stencil bits") ||
            !TrySDL(OpenGL = SDL_GL_CreateContext(Window), "Error initializing OpenGL context") ||
            !TrySDL(SDL_GL_SetSwapInterval(1), "Error setting v-sync") //Enable vsync (0 is off, -1 is freesync/G-sync I think?)
            )
        {
            return;
        }

        //Initialize GLEW
        glewExperimental = GL_TRUE;
        auto glewError = glewInit();
        if (glewError != GLEW_OK)
        {
            OnError(std::string("Error setting up GLEW: ") + (const char*)glewGetErrorString(glewError));
            return;
        }

        //Initialize Dear ImGUI.
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiContext = ImGui::GetIO();
        ImGuiContext.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForOpenGL(Window, OpenGL);
        ImGui_ImplOpenGL3_Init(OpenGLVersionStr);
    }
    ~AppResources()
    {
        //Clean up the window/SDL.
        if (OpenGL != nullptr)
            SDL_GL_DeleteContext(OpenGL);
        if (Window != nullptr)
            SDL_DestroyWindow(Window);
        SDL_Quit();

        //Write out the new config file.
        if (WriteConfigOnQuit)
            IO::WriteJsonToFile(GetConfigFullPath(), Config, OnError);
    }

    //No copy or move operator.
    AppResources(const AppResources& cpy) = delete;
    AppResources& operator=(const AppResources& cpy) = delete;

    #pragma endregion

private:

    bool TrySDL(int returnCode, const std::string& msgPrefix)
    {
        if (returnCode != 0)
            OnError(msgPrefix + ": " + SDL_GetError());

        return returnCode == 0;
    }
    bool TrySDL(void* shouldntBeNull, const std::string& msgPrefix)
    {
        if (shouldntBeNull == nullptr)
            return TrySDL(-1, msgPrefix);
        else
            return true;
    }
};


int main(int argc, char* argv[])
{
    std::cout << "Program path: " << fs::current_path() << "\n\n";

    //Set up error management.
    bool quitApp = false;
    auto errorCallback = [&quitApp](const std::string& msg) {
        quitApp = true;
        SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags::SDL_MESSAGEBOX_ERROR,
                                 "Error", msg.c_str(),
                                 nullptr);
    };

    //Load command-line settings.
    bool writeConfigOnExit = true;
    for (int argI = 0; argI < argc; ++argI)
        if (std::string(argv[argI]) == "-noWriteConfig")
            writeConfigOnExit = false;

    //Initialize the app's various modules.
    AppResources app("WFCpp Tile3D Editor", writeConfigOnExit, errorCallback);

    //Run the main loop.
    std::unique_ptr<Apps::Base> currentApp;
    while (!quitApp)
    {
        //TODO: Pull into "Main Menu" app. Turn "Apps::Base" into more of a state machine.

        //Process window events.
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent) != 0)
        {
            ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
            if (currentApp != nullptr)
                currentApp->ProcessOSEvent(sdlEvent);

            if (sdlEvent.type == SDL_EventType::SDL_QUIT ||
                (sdlEvent.type == SDL_EventType::SDL_WINDOWEVENT &&
                 sdlEvent.window.event == SDL_WINDOWEVENT_CLOSE))
            {
                quitApp = true;
            }
        }

        if (currentApp != nullptr)
        {
            if (quitApp)
                quitApp = currentApp->DoQuit(false);
            if (currentApp->DidQuit())
               currentApp.reset();
        }

        //Update the app.
        if (currentApp == nullptr)
        {
            GL::Clear(0.2f, 0.2f, 0.5f, 1);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(app.Window);
            ImGui::NewFrame();

            ImGui::Begin("Main Menu", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);

            if (ImGui::Button("Tile Prefab Editor"))
                ; //TODO: Make Tile Prefab Editor.
            if (ImGui::Button("Tile Editor"))
                ; //TODO: Make Tile Editor.
            if (ImGui::Button("Tile Runner"))
                ; //TODO: Make Tile Runner.

            ImGui::End();

            ImGui::Render();
            GL::SetViewport((int)app.ImGuiContext.DisplaySize.x,
                (int)app.ImGuiContext.DisplaySize.y);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(app.Window);
        }
        else
        {
            currentApp->RunAppFrame();
        }
    }

    return 0;
}