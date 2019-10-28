#pragma once

#include "Helpers.h"


namespace Bplus::GL
{
    //Provides numerous helper functions and enums for managing OpenGL global state.
    //Ensures good performance by remembering the current state and ignoring duplicate calls.
    //Note that only one of these can exist in each thread,
    //    and this fact is enforced in the constructor.
    class BP_API Context
    {
    public:
        static const char* GLSLVersion() { return "#version 450"; }
        static uint8_t GLVersion_Major() { return 4; }
        static uint8_t GLVersion_Minor() { return 5; }

        //May be null if no context exists right now.
        static Context* GetCurrentContext();


        //Creates the context based on the given SDL window.
        //If there was an error, "errorMsg" will be set to it.
        Context(SDL_Window* owner, std::string& errorMsg);
        ~Context();

        //Gets whether this context got initialized properly.
        bool WasInitialized() const { return isInitialized; }
        //Gets the SDL window this context was created for.
        SDL_Window* GetOwner() const { return owner; }


        //Clears the current framebuffer's color and depth.
        void Clear(float r, float g, float b, float a, float depth);

        //Clears the current framebuffer's color.
        void Clear(float r, float g, float b, float a);
        //Clears the current framebuffer's depth.
        void Clear(float depth);
    
        template<typename TVec4>
        void Clear(const TVec4& rgba) { Clear(rgba.r, rgba.g, rgba.b, rgba.a); }

        void SetViewport(int minX, int minY, int width, int height);
        void SetViewport(int width, int height) { SetViewport(0, 0, width, height); }

        void SetScissor(int minX, int minY, int width, int height);
        void DisableScissor();

        bool SetVsyncMode(VsyncModes mode);
        VsyncModes GetVsyncMode() const { return currentVsync; }

        void SetFaceCulling(FaceCullModes mode);
        FaceCullModes GetFaceCulling() const { return currentCullMode; }

        void SetDepthTest(DepthTests mode);
        DepthTests GetDepthTest() const { return currentDepthTest; }

        void SetDepthWrites(bool canWriteToDepth);
        bool GetDepthWrites() const { return isDepthWriteEnabled; }

        //TODO: Stencil.
        //TODO: Blending.
        //TODO: Anything else?


    private:

        bool isInitialized = false;

        bool isScissorEnabled, isDepthWriteEnabled;
        VsyncModes currentVsync;
        FaceCullModes currentCullMode;
        DepthTests currentDepthTest;
        
        SDL_GLContext sdlContext;
        SDL_Window* owner;
    };
}