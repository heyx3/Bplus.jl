#pragma once

#include <gl/glew.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/matrix.hpp>

#include "../Utils.h"
#include "../TomlIO.h"


//Defines various enums and data structures that represent rendering state.

//TODO: Split into multiple files.
//TODO: Add "Logical Blending", which is an alternative to normal blending. https://www.khronos.org/opengl/wiki/Logical_Operation
//TODO: Add the enabling/disabling of GL_FRAMEBUFFER_SRGB, which determines what happens when rendering into an sRGB texture.


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

    #pragma region Enums

    //Define various rendering enums with the help of the "Better Enums" library.
    //The enum values generally line up with OpenGL and/or SDL codes

    //SDL Vsync settings.
    BETTER_ENUM(VsyncModes, int,
        Off = 0,
        On = 1,
        Adaptive = -1
    );

    //Whether to cull polygon faces during rendering (and which side to cull).
    BETTER_ENUM(FaceCullModes, GLenum,
        Off = GL_INVALID_ENUM,
        On = GL_BACK,
        Backwards = GL_FRONT,
        All = GL_FRONT_AND_BACK
    );

    //The various modes for depth/stencil testing.
    BETTER_ENUM(ValueTests, GLenum,
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
    );

    //The various actions that can be taken on a stencil buffer.
    BETTER_ENUM(StencilOps, GLenum,
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
        DecrementWrap = GL_DECR_WRAP
    );

    //The different factors that can be used in the blend operation.
    BETTER_ENUM(BlendFactors, GLenum,
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
    );
    bool BP_API UsesConstant(BlendFactors factor);

    //The different ways that source and destination color can be combined
    //    (after each is multiplied by their BlendFactor).
    BETTER_ENUM(BlendOps, GLenum,
        Add = GL_FUNC_ADD,
        Subtract = GL_FUNC_SUBTRACT,
        ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
        Min = GL_MIN,
        Max = GL_MAX
    );

    #pragma endregion

    #pragma region Structures

    #pragma region BlendState<> struct template

    //A GPU blending mode.
    //Note that equality/hashing is not smart when it comes to
    //    blend states which are equivalent but specified differently.
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

        #pragma region Serialization/GUI

        void FromToml(const toml::Value& tomlData)
        {
            const char* status = "[null]";
            try
            {
                status = "Src";
                Src = IO::EnumFromString<BlendFactors>(tomlData, "Src");

                status = "Dest";
                Dest = IO::EnumFromString<BlendFactors>(tomlData, "Dest");

                status = "Op";
                Op = IO::EnumFromString<BlendOps>(tomlData, "Op");

                status = "Constant";
                if (UsesConstant())
                    if (tomlData.has("Constant"))
                        Constant = IO::FromToml<Constant_t::length(),
                                                Constant_t::value_type>
                                           (*(tomlData.find("Constant")));
                    else
                        throw IO::Exception("Missing field");
            }
            catch (const IO::Exception& e)
            {
                throw IO::Exception(e,
                                    std::string("Error parsing BlendState<>::") + status + ": ");
            }
        }
        toml::Value ToToml() const
        {
            toml::Value tomlData;
            tomlData["Src"] = Src._to_string();
            tomlData["Dest"] = Dest._to_string();
            tomlData["Op"] = Op._to_string();

            if (UsesConstant())
                tomlData["Constant"] = IO::ToToml(Constant);

            return tomlData;
        }

        //Displays Dear ImGUI widgets to edit this instance.
        //Returns whether any changes were made.
        bool EditGUI(std::function<bool(const char*, Constant_t&)> editConstantValue,
                     int popupMaxItemHeight = -1)
        {
            bool changed =
                     ImGui::EnumCombo("Src Factor", Src, popupMaxItemHeight) |
                     ImGui::EnumCombo("Dest Factor", Dest, popupMaxItemHeight) |
                     ImGui::EnumCombo("Op", Op, popupMaxItemHeight);
            if (UsesConstant())
                changed |= editConstantValue("Constant", Constant);
            return changed;
        }

        #pragma endregion
    };

    #pragma endregion
    using BlendStateRGB = BlendState<glm::vec3>;
    using BlendStateAlpha = BlendState<glm::vec1>;
    using BlendStateRGBA = BlendState<glm::vec4>;


    //A test made against the stencil buffer.
    //Note that hashing and equality tries to be intelligent,
    //    ignoring the "RefValue" and "Mask" fields
    //    if the test mode doesn't involve them.
    struct BP_API StencilTest
    {
        ValueTests Test = ValueTests::Off;
        GLint RefValue = 0;
        GLuint Mask = ~0;

        //Serialization/ImGUI editor:
        void FromToml(const toml::Value& tomlData);
        toml::Value ToToml() const;
        bool EditGUI(int popupMaxItemHeight = -1);
    };

    //What happens to a pixel when it passes/fails the stencil and depth buffer tests.
    struct BP_API StencilResult
    {
        StencilOps OnFailStencil = StencilOps::Nothing,
                   OnPassStencilFailDepth = StencilOps::Nothing,
                   OnPassStencilDepth = StencilOps::Nothing;

        StencilResult() { }
        StencilResult(StencilOps onFailStencil, StencilOps onPassStencilFailDepth,
                      StencilOps onPassStencilDepth)
            : OnFailStencil(onFailStencil), OnPassStencilFailDepth(onPassStencilFailDepth),
              OnPassStencilDepth(onPassStencilDepth) { }
        StencilResult(StencilOps onPassStencilDepth) : OnPassStencilDepth(onPassStencilDepth) { }

        //Serialization/ImGUI editor:
        void FromToml(const toml::Value& tomlData);
        toml::Value ToToml() const;
        bool EditGUI(int popupMaxItemHeight = -1);
    };

    #pragma endregion
}

#pragma region OpenGL handle typedefs

//Creates a type-safe wrapper around a raw OpenGL integer value.
//MUST be invoked in the global namespace due to the std::hash implementation!
#define MAKE_GL_STRONG_TYPEDEF(NewName, glName, nullValue) \
    namespace Bplus::GL::OglPtr { \
        strong_typedef_start(NewName, glName, BP_API) \
            strong_typedef_null(NewName, nullValue) \
            strong_typedef_equatable \
        strong_typedef_end \
    } \
    strong_typedef_hashable(Bplus::GL::OglPtr::NewName, BP_API);

MAKE_GL_STRONG_TYPEDEF(ShaderProgram, GLuint, 0);
MAKE_GL_STRONG_TYPEDEF(ShaderUniform, GLint, -1);

MAKE_GL_STRONG_TYPEDEF(Texture, GLuint, 0);
MAKE_GL_STRONG_TYPEDEF(Image, GLuint, 0);
MAKE_GL_STRONG_TYPEDEF(View, GLuint64, 0);
MAKE_GL_STRONG_TYPEDEF(Sampler, GLuint, 0);

MAKE_GL_STRONG_TYPEDEF(Target, GLuint, 0); //Called a "Framebuffer" in OpenGL
MAKE_GL_STRONG_TYPEDEF(TargetBuffer, GLuint, 0); //Called a "RenderBuffer" in OpenGL

MAKE_GL_STRONG_TYPEDEF(Buffer, GLuint, 0);
MAKE_GL_STRONG_TYPEDEF(Mesh, GLuint, 0);

#undef MAKE_GL_STRONG_TYPEDEF

#pragma endregion


//Make the above types hashable/equatable:

BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::VsyncModes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::FaceCullModes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::ValueTests);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::StencilOps);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::BlendFactors);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::BlendOps);

BP_HASH_EQ_TEMPL_START(Bplus::GL,
                       class Constant_t, BlendState<Constant_t>,
                       d.Src, d.Dest, d.Op,
                       (d.UsesConstant() ? d.Constant : Constant_t(0)))
    return a.Src == b.Src &&
           a.Dest == b.Dest &&
           a.Op == b.Op &&
           a.UsesConstant() == b.UsesConstant() &&
           (!a.UsesConstant() || (a.Constant == b.Constant));
BP_HASH_EQ_END

BP_HASH_EQ_START(Bplus::GL, StencilTest,
                 d.Test,
                 (d.Test == +Bplus::GL::ValueTests::Off || d.Test == +Bplus::GL::ValueTests::Never) ?
                    0 : d.RefValue,
                 (d.Test == +Bplus::GL::ValueTests::Off || d.Test == +Bplus::GL::ValueTests::Never) ?
                    0 : d.Mask)
    bool isRealTest = (a.Test != +Bplus::GL::ValueTests::Off) &&
                      (a.Test != +Bplus::GL::ValueTests::Never);
    return (a.Test == b.Test) &&
           //Only check RefValue and Mask if they're used by this type of test:
           (!isRealTest ||
            (a.RefValue == b.RefValue && a.Mask == b.Mask));
BP_HASH_EQ_END

BP_HASH_EQ_START(Bplus::GL, StencilResult,
                 d.OnFailStencil, d.OnPassStencilFailDepth, d.OnPassStencilDepth)
    return a.OnFailStencil == b.OnFailStencil &&
           a.OnPassStencilFailDepth == b.OnPassStencilFailDepth &&
           a.OnPassStencilDepth == b.OnPassStencilDepth;
BP_HASH_EQ_END