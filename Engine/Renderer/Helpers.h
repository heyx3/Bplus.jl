#pragma once

#include "../RenderLibs.h"


namespace Bplus::GL
{
    //If the given SDL error code doesn't represent success,
    //    returns false and sets "errOut" to "prefix" plus the SDL error.
    //Otherwise, returns true.
    bool BP_API TrySDL(int returnCode, std::string& errOut, const char* prefix);
    //If the given SDL object is null,
    //    returns false and sets "errOut" to "prefix" plus the SDL error.
    //Otherwise, returns true.
    bool BP_API TrySDL(void* shouldntBeNull, std::string& errOut, const char* prefix);


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

    //Lines up with the OpenGL depth/stencil test constants where applicable.
    enum class BP_API ValueTests : GLenum
    {
        //The test always passes. Note that this does NOT disable depth writes.
        Off = GL_ALWAYS,
        //The test always fails.
        Never = GL_NEVER,

        //Passes if the fragment's value is less than the "test" value.
        LessThan = GL_LESS,
        //Passes if the fragment's value is less than or equal to the "test" value.
        LessThanOrEqual = GL_LEQUAL,

        //Passes if the fragment's value is greater than the "test" value.
        GreaterThan = GL_GREATER,
        //Passes if the fragment's value is greater than or equal to the "test" value.
        GreaterThanOrEqual = GL_GEQUAL,

        //Passes if the fragment's value is equal to the "test" value.
        Equal = GL_EQUAL,
        //Passes if the fragment's value is not equal to the "test" value.
        NotEqual = GL_NOTEQUAL
    };

    //Lines up with the OpenGL stencil operation constants.
    enum class BP_API StencilOps : GLenum
    {
        //Don't modify the stencil buffer value.
        Nothing = GL_KEEP,

        //Set the stencil buffer value to 0.
        Zero = GL_ZERO,
        //Replace the buffer's value with the fragment's value.
        Replace = GL_REPLACE,
        //Bitwise-NOT the buffer's value.
        Invert = GL_INVERT,

        //Increments the stencil buffer's value, clamping it to stay inside its range.
        IncrementClamp = GL_INCR,
        //Increments the stencil buffer's value, wrapping around to 0 if it's at the max value.
        IncrementWrap = GL_INCR_WRAP,

        //Decrements the stencil buffer's value, clamping it to stay inside its range.
        DecrementClamp = GL_DECR,
        //Decrements the stencil buffer's value, wrapping around to the max value if it's at 0.
        DecrementWrap = GL_DECR_WRAP,
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


    #pragma region BlendState<> struct template
    template<typename Constant_t>
    struct BlendState
    {
        BlendFactors Src = BlendFactors::One,
                     Dest = BlendFactors::Zero;
        BlendOps Op = BlendOps::Add;

        //Only used with the various "Constant" blend factors.
        Constant_t Constant;

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
    bool operator==(const BlendState<Constant_t>& a, const BlendState<Constant_t>& b)
    {
        return (a.Src == b.Src) &
               (a.Dest == b.Dest) &
               (a.Op == b.Op) &
               (!a.UsesConstant() | (a.Constant == b.Constant));
    }
    template<typename Constant_t>
    bool operator!=(const BlendState<Constant_t>& a, const BlendState<Constant_t>& b)
    {
        return !(a == b);
    }
    #pragma endregion
    using BlendStateRGB = BlendState<glm::vec3>;
    using BlendStateAlpha = BlendState<float>;
    using BlendStateRGBA = BlendState<glm::vec4>;


    struct BP_API StencilTest
    {
        ValueTests Test = ValueTests::Off;
        GLint RefValue = 0;
        GLuint Mask = ~0;
    };
    bool BP_API operator==(const StencilTest& a, const StencilTest& b);
    bool BP_API operator!=(const StencilTest& a, const StencilTest& b) { return !(a == b); }


    struct BP_API StencilResult
    {
        //What happens to the stencil buffer when a fragment is placed into a pixel.
        StencilOps OnFailStencil = StencilOps::Nothing,
                   OnPassStencilFailDepth = StencilOps::Nothing,
                   OnPassStencilDepth = StencilOps::Nothing;

        StencilResult() { }
        StencilResult(StencilOps onFailStencil, StencilOps onPassStencilFailDepth,
                      StencilOps onPassStencilDepth)
            : OnFailStencil(onFailStencil), OnPassStencilFailDepth(onPassStencilFailDepth),
              OnPassStencilDepth(onPassStencilDepth) { }
        StencilResult(StencilOps onPassStencilDepth)
            : StencilResult(StencilOps::Nothing, StencilOps::Nothing, onPassStencilDepth) { }
    };
    bool BP_API operator==(const StencilResult& a, const StencilResult& b);
    bool BP_API operator!=(const StencilResult& a, const StencilResult& b) { return !(a == b); }


    //Converters from enums to string.
    BP_API const char* ToString(VsyncModes mode);
    BP_API const char* ToString(FaceCullModes culling);
    BP_API const char* ToString(ValueTests mode);
    BP_API const char* ToString(StencilOps op);
    BP_API const char* ToString(BlendFactors factor);
    BP_API const char* ToString(BlendOps op);

    //Converters from strings to enum (case-insensitive).
    VsyncModes    BP_API FromString_VsyncModes(const char* modeStr);
    FaceCullModes BP_API FromString_FaceCullModes(const char* cullingStr);
    StencilOps    BP_API FromString_StencilOps(const char* opStr);
    ValueTests    BP_API FromString_ValueTests(const char* modeStr);
    BlendFactors  BP_API FromString_BlendFactors(const char* factorStr);
    BlendOps      BP_API FromString_BlendOps(const char* opStr);
}