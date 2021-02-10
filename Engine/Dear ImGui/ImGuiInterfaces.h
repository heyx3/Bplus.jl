#pragma once

#include "../Utils.h"

#include <glm/vec2.hpp>
#include <glm/fwd.hpp>
#include "imgui.h"

#include <string>
#include <array>


struct SDL_Window;
union SDL_Event;
struct SDL_Cursor;
typedef void* SDL_GLContext;
typedef unsigned int GLuint;

//TODO: Bring in RAII helpers: https://github.com/ocornut/imgui/blob/0a25a49e946c1557b05456c02366773b34996a1d/misc/cpp/imgui_scoped.h


namespace Bplus
{
    //An abstract class defining an interface for connecting ImGUI to SDL.
    //Initialization/cleanup is done through RAII.
    //This class is a thread-local singleton.
    class BP_API ImGuiSDLInterface
    {
    public:
        static ImGuiSDLInterface* GetThreadInstance();

        ImGuiSDLInterface(SDL_Window* _mainWindow, SDL_GLContext _glContext);
        virtual ~ImGuiSDLInterface();

        //Can't copy/move this interface -- think of it like a singleton.
        ImGuiSDLInterface(const ImGuiSDLInterface& cpy) = delete;
        ImGuiSDLInterface& operator=(const ImGuiSDLInterface& cpy) = delete;


        SDL_Window* GetWindow() const { return mainWindow; }
        SDL_GLContext GetContext() const { return glContext; }

        virtual void BeginFrame(float deltaTime) = 0;
        virtual void ProcessEvent(const SDL_Event& event) = 0;


    private:
        SDL_Window* mainWindow;
        SDL_GLContext glContext;
    };

    //An abstract class defining an interface for connecting ImGUI to OpenGL.
    //This class is a thread-local singleton.
    //Initialization/cleanup is done through RAII.
    class BP_API ImGuiOpenGLInterface
    {
    public:
        static ImGuiOpenGLInterface* GetThreadInstance();

        ImGuiOpenGLInterface(const char* _glslVersion);
        virtual ~ImGuiOpenGLInterface();

        //Can't copy/move this interface -- think of it like a singleton.
        ImGuiOpenGLInterface(const ImGuiOpenGLInterface& cpy) = delete;
        ImGuiOpenGLInterface& operator=(const ImGuiOpenGLInterface& cpy) = delete;


        const std::string& GetGlslVersion() const { return glslVersion; }

        virtual void BeginFrame() { };
        virtual void RenderFrame() = 0;

    private:
        std::string glslVersion;
    };
}