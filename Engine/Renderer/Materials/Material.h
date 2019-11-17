#pragma once

#include "../Context.h"

namespace Bplus::GL
{
    //A basic compiled shader + render settings.
    class Material
    {
    public:
        
        //The render state this Material should run in.
        RenderState RenderSettings;

    private:

        GLuint programHandle;
    };
}