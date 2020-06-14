#pragma once

#include "Data.h"
#include <functional>


namespace Bplus::GL
{
    //TODO: Changing viewport Y axis and depth (how can GLM support this depth mode?): https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glClipControl.xhtml
    //TODO: Give various object names with glObjectLabel

    //Represents OpenGL's global state, like the current blend mode and stencil test.
    //Does not include some things like bound objects, shader uniforms, etc.
    class BP_API RenderState
    {
    public:

        bool EnableDepthWrite;
        glm::bvec4 ColorWriteMask;
        FaceCullModes CullMode;
        ValueTests DepthTest;
        BlendStateRGB ColorBlending;
        BlendStateAlpha AlphaBlending;
        StencilTest StencilTestFront, StencilTestBack;
        StencilResult StencilResultFront, StencilResultBack;
        GLuint StencilMaskFront, StencilMaskBack;
        //TODO: Anything else?


        //Must give the better_enum types constructed values to avoid a compile error.
        RenderState(FaceCullModes cullMode = FaceCullModes::On,
                    ValueTests depthTest = ValueTests::LessThan)
            : CullMode(cullMode), DepthTest(depthTest) { }
    };


    //Manages OpenGL initialization, shutdown,
    //    and global state such as the current blend mode and stencil test.
    //Ensures good performance by remembering the current state and ignoring duplicate calls.
    //Note that only one of these should exist in each thread,
    //    and this constraint is enforced in the constructor.
    class BP_API Context
    {
    public:
        static const char* GLSLVersion() { return "#version 450"; }
        static uint8_t GLVersion_Major() { return 4; }
        static uint8_t GLVersion_Minor() { return 5; }

        //May be null if no context exists right now.
        //Note that each thread has its own singleton instance.
        static Context* GetCurrentContext();


        #pragma region Callbacks

        //Registers a callback for when this thread's context is destroyed.
        static void RegisterCallback_Destroyed(std::function<void()>);

        //Registers a callback for when this thread's context's "RefreshState()" is called.
        static void RegisterCallback_RefreshState(std::function<void()>);

        #pragma endregion


        //Creates the context based on the given SDL window.
        //If there was an error, "errorMsg" will be set to it.
        Context(SDL_Window* owner, std::string& errorMsg,
                VsyncModes vsync = VsyncModes::Off);
        ~Context();

        //Gets whether this context got initialized properly.
        bool WasInitialized() const { return isInitialized; }
        //Gets the SDL window this context was created for.
        SDL_Window* GetOwner() const { return owner; }
        SDL_GLContext GetSDLContext() const { return sdlContext; }

        //Queries OpenGL for the current context state.
        //Call this after any OpenGL work is done not through this class.
        void RefreshState();


        const RenderState& GetState() const { return state; }
        void SetState(const RenderState& newState);


        bool SetVsyncMode(VsyncModes mode);
        VsyncModes GetVsyncMode() const { return vsync; }

        //Clears the current framebuffer's color and depth.
        void Clear(float r, float g, float b, float a, float depth);

        //Clears the current framebuffer's color.
        void Clear(float r, float g, float b, float a);
        //Clears the current framebuffer's depth.
        void Clear(float depth);
    
        template<typename TVec4>
        void Clear(const TVec4& rgba) { Clear(rgba.r, rgba.g, rgba.b, rgba.a); }


        void SetFaceCulling(FaceCullModes mode);
        FaceCullModes GetFaceCulling() const { return state.CullMode; }

        #pragma region Viewport

        void SetViewport(int minX, int minY, int width, int height);
        void SetViewport(int width, int height) { SetViewport(0, 0, width, height); }
        void GetViewport(int& outMinX, int& outMinY, int& outWidth, int& outHeight);

        void SetScissor(int minX, int minY, int width, int height);
        void DisableScissor();

        //If scissor is disabled, returns false.
        //Otherwise, returns true and sets the given output parameters.
        bool GetScissor(int& outMinX, int& outMinY, int& outWidth, int& outHeight) const;

        #pragma endregion

        #pragma region Depth/Color

        void SetDepthTest(ValueTests mode);
        ValueTests GetDepthTest() const { return state.DepthTest; }

        void SetDepthWrites(bool canWriteToDepth);
        bool GetDepthWrites() const { return state.EnableDepthWrite; }

        void SetColorWriteMask(glm::bvec4 canWrite);
        glm::bvec4 GetColorWriteMask() const { return state.ColorWriteMask; }

        #pragma endregion

        #pragma region Blending

        //Gets the current global blend operation, assuming
        //    both color and alpha have the same setting.
        BlendStateRGBA GetBlending() const;
        //Sets both color and alpha blending to the given state.
        void SetBlending(const BlendStateRGBA& state);

        BlendStateRGB GetColorBlending() const { return state.ColorBlending; }
        void SetColorBlending(const BlendStateRGB& state);

        BlendStateAlpha GetAlphaBlending() const { return state.AlphaBlending; }
        void SetAlphaBlending(const BlendStateAlpha& state);

        #pragma endregion

        #pragma region Stencil

        //Gets the current global stencil test, assuming
        //    both front- and back-faces have the same stencil test setting.
        const StencilTest& GetStencilTest() const;
        //Sets both front- and back-faces to use the given stencil test.
        void SetStencilTest(const StencilTest& test);

        const StencilTest& GetStencilTestFrontFaces() const { return state.StencilTestFront; }
        void SetStencilTestFrontFaces(const StencilTest& test);

        const StencilTest& GetStencilTestBackFaces() const { return state.StencilTestBack; }
        void SetStencilTestBackFaces(const StencilTest& test);


        //Gets the current global stencil write operations, assuming
        //    both front- and back-faces have the same stencil write settings.
        const StencilResult& GetStencilResult() const;
        //Sets both front- and back-faces to use the given stencil write operations.
        void SetStencilResult(const StencilResult& writeResults);

        const StencilResult& GetStencilResultFrontFaces() const { return state.StencilResultFront; }
        void SetStencilResultFrontFaces(const StencilResult& writeResult);

        const StencilResult& GetStencilResultBackFaces() const { return state.StencilResultBack; }
        void SetStencilResultBackFaces(const StencilResult& writeResult);


        //Gets the current global stencil mask, determining which bits
        //    can actually be written to by the "StencilResult" settings.
        GLuint GetStencilMask() const;
        void SetStencilMask(GLuint newMask);

        GLuint GetStencilMaskFrontFaces() const { return state.StencilMaskFront; }
        void SetStencilMaskFrontFaces(GLuint newMask);

        GLuint GetStencilMaskBackFaces() const { return state.StencilMaskBack; }
        void SetStencilMaskBackFaces(GLuint newMask);

        #pragma endregion

    private:

        bool isInitialized = false;
        SDL_GLContext sdlContext;
        SDL_Window* owner;

        RenderState state;
        glm::ivec4 viewport;
        Lazy<glm::ivec4> scissor;
        VsyncModes vsync;
    };
}