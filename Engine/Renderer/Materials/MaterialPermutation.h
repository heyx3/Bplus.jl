#pragma once

#include "../Context.h"
#include "UniformSet.h"


namespace Bplus::GL
{
    //A specific compiled shader.
    //Tracks its own set of uniforms.
    class MaterialPermutation
    {
    public:

        //A unique identifier for this permutation across the entire material.
        using ID = uint32_t;

        UniformSet Uniforms;


    private:

        GLuint programHandle;
        UniformSet uniforms;
        ID id;
    };

    //TODO: Base class for bitfield data covering a material permutation ID.
}