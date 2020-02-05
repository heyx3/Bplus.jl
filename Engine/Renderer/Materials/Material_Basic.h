#pragma once

#include "CompiledShader.h"
#include "MaterialData.h"


namespace Bplus::GL
{
    //Generates a CompiledShader, given numerous settings.
    class BP_API Material_Basic
    {
    public:


        BlendStateRGBA Blending;
        FaceCullModes CullMode;

    };
}