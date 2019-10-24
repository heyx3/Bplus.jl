#pragma once

#include "../Platform.h"


namespace Bplus::GL
{
    //Clears the current framebuffer's color and depth.
    void BP_API Clear(float r, float g, float b, float a, float depth);

    //Clears the current framebuffer's color.
    void BP_API Clear(float r, float g, float b, float a);
    //Clears the current framebuffer's depth.
    void BP_API Clear(float depth);
    
    template<typename TVec4>
    void Clear(const TVec4& rgba) { Clear(rgba.r, rgba.g, rgba.b, rgba.a); }


    void BP_API SetViewport(int minX, int minY, int width, int height);
    inline void BP_API SetViewport(int width, int height) { SetViewport(0, 0, width, height); }

    void BP_API SetScissor(int minX, int minY, int width, int height);
    void BP_API DisableScissor();
    bool BP_API IsScissorEnabled();

    //The enum values line up with the SDL codes in SDL_GL_SetSwapInterval().
    enum class BP_API VsyncModes
    {
        Off = 0,
        On = 1,
        Adaptive = -1 //i.e. AMD FreeSync and NVidia G-Sync
    };
}