#pragma once

#include <cstdint>

namespace GL
{
    //Clears the current framebuffer's color and depth.
    void Clear(float r, float g, float b, float a, float depth);

    //Clears the current framebuffer's color.
    void Clear(float r, float g, float b, float a);
    //Clears the current framebuffer's depth.
    void Clear(float depth);
    
    template<typename TVec4>
    void Clear(const TVec4& rgba) { Clear(rgba.r, rgba.g, rgba.b, rgba.a); }


    void SetViewport(int minX, int minY, int width, int height);
    inline void SetViewport(int width, int height) { SetViewport(0, 0, width, height); }

    void SetScissor(int minX, int minY, int width, int height);
    void DisableScissor();
    bool IsScissorEnabled();
}