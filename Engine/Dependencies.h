#pragma once

//Basic B+ platform/macro stuff:
#include "Platform.h"
#include "Utils/BPAssert.h" //Needed to hook library asserts into the B-plus assert


//Dear ImGUI, an immediate-mode GUI library.
#include "Dear ImGui\ImGuiInterfaces.h"
#include "Dear ImGui\ImGuiAddons.h"

//SDL, for input and window management.
#define SDL_MAIN_HANDLED 1
#include <sdl\SDL.h>

//OpenGL.
#include <gl/glew.h>
#include <sdl/SDL_opengl.h>

//GLM, a popular math library for OpenGL.
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#pragma region GLM helpers

namespace Bplus::Math
{
    //Overloads of Lerp() for for GLM vectors.
    //This overload supports vector min/max values, and scalar OR vector 't' value.
    template<glm::length_t L, typename T, enum glm::qualifier Q = glm::packed_highp,
             typename Interpolator_t = glm::vec<L, AppropriateFloat_t<T>, Q>>
    inline auto Lerp(const glm::vec<L, T, Q>& a,
                     const glm::vec<L, T, Q>& b,
                     const Interpolator_t& t)
    {
        return glm::lerp(a, b, t);
    }
    //This overload supports scalar min/max values, and vector 't' value.
    template<glm::length_t L, typename T, enum glm::qualifier Q = glm::packed_highp,
             typename Float_t = AppropriateFloat_t<T>>
    inline auto Lerp(const T& a, const T& b,
                     const glm::vec<L, Float_t, Q>& t)
    {
        return Lerp(glm::vec<L, T, Q>{a},
                    glm::vec<L, T, Q>{b},
                    t);
    }

    //Overloads of InverseLerp() for GLM vectors.
    //This overload supports vector min/max values, and a vector 'x' value.
    //The result is undefined if 'a' and 'b' are equal in at least one component.
    template<glm::length_t L, typename T, enum glm::qualifier Q = glm::packed_highp,
             typename Value_t = glm::vec<L, T, Q>,
             typename Float_t = AppropriateFloat_t<T>>
    inline glm::vec<L, Float_t, Q> InverseLerp(const glm::vec<L, T, Q>& a,
                                               const glm::vec<L, T, Q>& b,
                                               const glm::vec<L, T, Q>& x)
    {
        glm::vec<L, Float_t, Q> _a(a),
                                _b(b),
                                _x(x);
        return (x - a) / (b - a);
    }
    //This overload supports scalar min/max values, and a vector 'x' value.
    //The result is undefined if 'a' and 'b' are equal in at least one component.
    template<glm::length_t L, typename T, enum glm::qualifier Q = glm::packed_highp>
    inline auto InverseLerp(const T& a, const T& b,
                            const glm::vec<L, T, Q>& x)
    {
        return InverseLerp(glm::vec<L, T, Q>{a},
                           glm::vec<L, T, Q>{b},
                           x);
    }
    //This overload supports vector min/max values, and a scalar 'x' value.
    //The result is undefined if 'a' and 'b' are equal in at least one component.
    template<glm::length_t L, typename T, enum glm::qualifier Q = glm::packed_highp>
    inline auto InverseLerp(const glm::vec<L, T, Q>& a,
                            const glm::vec<L, T, Q>& b,
                            const T& x)
    {
        return InverseLerp(a, b,
                           glm::vec<L, T, Q>{x});
    }


    //For some reason, this isn't clearly exposed in GLM's interface.
    template<typename T, enum glm::qualifier Q = glm::packed_highp>
    glm::qua<T, Q> RotIdentity() { return glm::qua<T, Q>(1, 0, 0, 0); }

    //Applies two transforms (matrices or quaternions) in the given order.
    template<typename glm_t>
    glm_t ApplyTransform(glm_t firstTransform, glm_t secondTransform)
    {
        return secondTransform * firstTransform;
    }

    template<typename T, enum glm::qualifier Q = glm::packed_highp>
    glm::vec<3, T, Q> ApplyToPoint(const glm::mat<4, 4, T, Q>& mat, const glm::vec<3, T, Q>& point)
    {
        auto point4 = mat * glm::vec<4, T, Q>(point, 1);
        return glm::vec<3, T, Q>(point4.x, point4.y, point4.z) / point4.w;
    }
    template<typename T, enum glm::qualifier Q = glm::packed_highp>
    glm::vec<3, T, Q> ApplyToVector(const glm::mat<4, 4, T, Q>& mat, const glm::vec<3, T, Q>& point)
    {
        auto point4 = mat * glm::vec<4, T, Q>(point, 0);
        return glm::vec<3, T, Q>(point4.x, point4.y, point4.z);
    }

    template<typename T, enum glm::qualifier Q = glm::packed_highp>
    glm::vec<3, T, Q> ApplyRotation(const glm::qua<T, Q>& rotation, const glm::vec<3, T, Q>& inPoint)
    {
        return rotation * inPoint;
    }

    //Makes a quaternion to rotate a point around the given axis
    //    by the given angle, clockwise when looking along the axis.
    //This function exists because glm is frustratingly vague about these details.
    template<typename T, enum glm::qualifier Q = glm::packed_highp>
    glm::qua<T, Q> MakeRotation(const glm::vec<3, T, Q>& axis,
                                T clockwiseDegrees)
    {
        return glm::angleAxis<T, Q>(glm::radians(clockwiseDegrees), axis);
    }

    //Resizes the given matrix.
    //New rows/columns are filled with zero.
    template<glm::length_t COut, glm::length_t ROut,
             glm::length_t CIn, glm::length_t RIn,
             typename T,
             enum glm::qualifier Q = glm::packed_highp>
    glm::mat<COut, ROut, T, Q> Resize(const glm::mat<CIn, RIn, T, Q>& mIn)
    {
        glm::mat<COut, ROut, T, Q> mOut;
        for (glm::length_t col = 0; col < COut; ++col)
            glm::column(mOut, col, glm::column(mIn, col));
        return mOut;
    }

    //Converts a vector of one size into a vector of another size.
    //New components are filled with 0.
    template<glm::length_t LOut,
             glm::length_t LIn, typename T,
             enum glm::qualifier Q = glm::packed_highp>
    glm::vec<LOut, T, Q> Resize(const glm::vec<LIn, T, Q>& vIn)
    {
        glm::vec<LOut, T, Q> vOut(T{0});
        for (glm::length_t i = 0; i < LIn; ++i)
            vOut[i] = vIn[i];
        return vOut;
    }
}
#pragma endregion


//EnTT, an Entity-Component System.
#define ENTT_ASSERT(condition) BP_ASSERT(condition, "[internal EnTT assert]")
#include <entt.hpp>
//To avoid compiler warnings, the use of EnTT "hashed strings" must go through the below macro.
#if !defined(COMPILER_VS)
    #define ENTT_SYMBOL(str) entt::hashed_string{str}
#else
    #define ENTT_SYMBOL(str) __pragma(warning(suppress:4307)) entt::hashed_string::value(str)
#endif


//Simple Iterator Template, which makes it much easier to construct custom STL iterators.
#include <iterator_tpl.h>

//zlib, a popular compression algorithm.
#define ZLIB_WINAPI
#define ZLIB_DLL
#include <zlib/zlib.h>

//Various image libraries.
#include <turboJPEG/jpeglib.h>
//TODO: Set up a custom error handler for TurboJPEG that uses BP_ASSERT
#define PNG_USE_DLL 1
#include <libPNG/png.h>

//TODO: Libraries for Block-Compression of textures (see below):
//        * https://github.com/richgel999/bc7enc16
//        * https://github.com/GameTechDev/ISPCTextureCompressor

//AssImp, the 3D model file saving/loading library.
#include <assimp/scene.h>