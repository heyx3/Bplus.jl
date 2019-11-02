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

    //Lines up with the OpenGL blend constants where applicable.
    enum class BP_API BlendFactors
    {
        Zero = GL_ZERO,
        One = GL_ONE,

        SrcColor = GL_SRC_COLOR,
        SrcAlpha = GL_SRC_ALPHA,

        InverseSrcColor = GL_ONE_MINUS_SRC_COLOR,
        InverseSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,

        DestColor = GL_DST_COLOR,
        DestAlpha = GL_DST_ALPHA,

        InverseDestColor = GL_ONE_MINUS_DST_COLOR,
        InverseDestAlpha = GL_ONE_MINUS_DST_ALPHA,

        //Unlike the others, this isn't a multiplier --
        //    it replaces the original value with a user-defined constant.
        ConstantColor = GL_CONSTANT_COLOR,
        //Unlike the others, this isn't a multiplier --
        //    it replaces the original value with a user-defined constant.
        ConstantAlpha = GL_CONSTANT_ALPHA,

        //Unlike the others, this isn't a multiplier --
        //    it replaces the original value with a user-defined constant.
        InverseConstantColor = GL_ONE_MINUS_CONSTANT_COLOR,
        //Unlike the others, this isn't a multiplier --
        //    it replaces the original value with a user-defined constant.
        InverseConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA
    };
    bool BP_API UsesConstant(BlendFactors factor);

    //Lines up with the OpenGL blend constants where applicable.
    enum class BP_API BlendOps
    {
        Add = GL_FUNC_ADD,
        Subtract = GL_FUNC_SUBTRACT,
        ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
        Min = GL_MIN,
        Max = GL_MAX
    };

    template<typename Constant_t>
    struct BP_API BlendState
    {
        BlendFactors Src = BlendFactors::One,
                     Dest = BlendFactors::Zero;
        BlendOps Op = BlendOps::Add;

        //Only used with the various "Constant" blend factors.
        Constant_t Constant = 1;


        bool UsesConstant() const { return GL::UsesConstant(Src) | GL::UsesConstant(Dest); }

        static BlendState Opaque() { return BlendState(); }
        static BlendState Transparent() { return BlendState{ BlendFactors::SrcAlpha,
                                                             BlendFactors::InverseSrcAlpha }; }
        static BlendState Additive() { return BlendState{ BlendFactors::One,
                                                          BlendFactors::Zero }; }
    };
    //Note that equality comparisons don't check whether the two states are
    //    *effectively* equal;
    //    only that their fields are identical.
    //There are sometimes multiple ways to represent the same blend effect.
    template<typename Constant_t>
    BP_API bool operator==(const BlendState<Constant_t>& a, const BlendState<Constant_t>& b)
    {
        return (a.Src == b.Src) &
               (a.Dest == b.Dest) &
               (a.Op == b.Op) &
               (!a.UsesConstant() | (a.Constant == b.Constant));
    }
    template<typename Constant_t>
    BP_API bool operator!=(const BlendState<Constant_t>& a, const BlendState<Constant_t>& b)
    {
        return !(a == b);
    }

    using BlendStateRGB = BlendState<glm::vec3>;
    using BlendStateAlpha = BlendState<float>;
    using BlendStateRGBA = BlendState<glm::vec4>;


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
    BP_API const char* ToString(BlendFactors factor);
    BP_API const char* ToString(BlendOps op);

    //Converters from strings to enum (case-insensitive).
    VsyncModes BP_API FromString_VsyncModes(const char* modeStr);
    FaceCullModes BP_API FromString_FaceCullModes(const char* cullingStr);
    DepthTests BP_API FromString_DepthTests(const char* modeStr);
    BlendFactors BP_API FromString_BlendFactors(const char* factorStr);
    BlendOps BP_API FromString_BlendOps(const char* opStr);
}