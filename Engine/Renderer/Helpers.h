#pragma once

#include "../Platform.h"
#include "../RenderLibs.h"

namespace Bplus::GL
{
    //The enum values line up with the SDL codes in SDL_GL_SetSwapInterval().
    enum class BP_API VsyncModes : int
    {
        Off = 0,
        On = 1,

        //AMD FreeSync and NVidia G-Sync
        Adaptive = -1
    };

    //The enum values line up with the OpenGL blend constants where applicable.
    enum class BP_API FaceCullModes : GLenum
    {
        Off = GL_INVALID_ENUM,
        On = GL_BACK,
        Backwards = GL_FRONT,
        All = GL_FRONT_AND_BACK
    };

    //Lines up with the OpenGL depth test constants where applicable.
    enum class BP_API DepthTests : GLenum
    {
        //Depth-testing always passes. Note that this does NOT disable depth writes.
        Off = GL_ALWAYS,
        //Depth-testing always fails.
        Never = GL_NEVER,

        //Passes if the depth is less than the test value.
        LessThan = GL_LESS,
        //Passes if the depth is less than or equal to the test value.
        LessThanOrEqual = GL_LEQUAL,

        //Passes if the depth is greater than the test value.
        GreaterThan = GL_GREATER,
        //Passes if the depth is greater than or equal to the test value.
        GreaterThanOrEqual = GL_GEQUAL,

        //Passes if the depth is equal to the test value.
        Equal = GL_EQUAL,
        //Passes if the depth is not equal to the test value.
        NotEqual = GL_NOTEQUAL
    };


    //If the given SDL error code doesn't represent success,
    //    returns false and sets "errOut" to "prefix" plus the SDL error.
    //Otherwise, returns true.
    bool BP_API TrySDL(int returnCode, std::string& errOut, const char* prefix);
    //If the given SDL object is null,
    //    returns false and sets "errOut" to "prefix" plus the SDL error.
    //Otherwise, returns true.
    bool BP_API TrySDL(void* shouldntBeNull, std::string& errOut, const char* prefix);


    //Converters from enums to string.
    BP_API const char* ToString(VsyncModes mode);
    BP_API const char* ToString(FaceCullModes culling);
    BP_API const char* ToString(DepthTests mode);

    //Converters from strings to enum (case-insensitive).
    VsyncModes BP_API FromString_VsyncModes(const char* modeStr);
    FaceCullModes BP_API FromString_FaceCullModes(const char* cullingStr);
    DepthTests BP_API FromString_DepthTests(const char* modeStr);

}