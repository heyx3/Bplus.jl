#pragma once

#include "CompiledShader.h"

namespace Bplus::GL::Materials
{
    //Abstract base class for a group of shaders that can be seamlessly swapped out
    //    based on shader-compile-time settings.
    //Each individual compiled shader is called a "variant".
    //Child classes should not inherit from this, but from the below "_Factory" class.
    class BP_API Factory
    {
    public:
        virtual ~Factory() { }
    };
}