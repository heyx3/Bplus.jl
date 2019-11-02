#pragma once

#include "../Platform.h"

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

    #pragma region Default implementation
    //The default implementation of ImGuiSDLInterface. This is usually enough.
    //Designed to be overridden if your custom use-case isn't too complicated.
    class BP_API ImGuiSDLInterface_Default : public ImGuiSDLInterface
    {
    public:
        ImGuiSDLInterface_Default(SDL_Window* mainWindow, SDL_GLContext glContext);
        virtual ~ImGuiSDLInterface_Default() override;

        void BeginFrame(float deltaTime) override;
        virtual void ProcessEvent(const SDL_Event& event) override;

    protected:

        std::array<bool, 3> mousePressed = { false, false, false };
        std::string clipboard;

        virtual void SetSDLClipboardText(const char* text);
        virtual const char* GetSDLClipboardText();

        virtual std::array<bool, 3> RefreshMouseData(glm::ivec2& outPos);
        virtual void GetWindowDisplayScale(glm::ivec2& windowSize,
                                           glm::fvec2& windowFramebufferScale);
        virtual void ProcessGamepadInput(ImGuiIO& io);

        SDL_Cursor*& GetSDLCursor(ImGuiMouseCursor index) { return mouseCursors[index]; }

    private:

        std::array<SDL_Cursor*, ImGuiMouseCursor_COUNT> mouseCursors;
    };
    #pragma endregion



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

    #pragma region Default implementation
    //The default implementation of ImGuiOpenGLInterface. This is usually enough.
    //Designed to be overridden if your custom use-case isn't too complicated.
    class BP_API ImGuiOpenGLInterface_Default : public ImGuiOpenGLInterface
    {
    public:
        //GLSL Version defaults to Bplus::GL::Context::GLSLVersion(), which is usually what you want.
        //Leaves "outErrorMsg" alone if nothing bad happened.
        ImGuiOpenGLInterface_Default(std::string& outErrorMsg,
                                     const char* glslVersion = nullptr);
        virtual ~ImGuiOpenGLInterface_Default() override;
        
        virtual void RenderFrame() override;
          
    protected:
        //Used inside "RenderFrame()".
        virtual void ResetRenderState(ImDrawData& drawData, glm::ivec2 framebufferSize,
                                      GLuint vao);
        //Used inside "RenderFrame()".
        virtual void RenderCommandList(ImDrawData& drawData, const ImDrawList& commandList,
                                       glm::ivec2 framebufferSize,
                                       glm::fvec2 clipOffset, glm::fvec2 clipScale,
                                       bool clipOriginIsLowerLeft,
                                       GLuint vao);

    private:

        //All handles are default-initialized to 0 so we can tell whether the constructor actually got to them.

        GLuint handle_fontTexture = 0,
               handle_shaderProgram = 0,
               handle_vertShader = 0,
               handle_fragShader = 0,
               handle_vbo = 0,
               handle_elements = 0;
        int uniform_tex = 0,
            uniform_projectionMatrix = 0,
            attrib_pos = 0,
            attrib_uv = 0,
            attrib_color = 0;
    };
    #pragma endregion
}