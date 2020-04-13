#pragma once

#include <optional>

#include "Box.hpp"
#include "Sphere.hpp"
#include "Math.h"


namespace Bplus::Math
{
    //An N-dimensional ray with the given number type
    //  (should be floating-point).
    template<glm::length_t N, typename F>
    struct Ray
    {
        using vec_t = glm::vec<N, F>;
        vec_t Start, Dir;

        vec_t GetAt(F t) const { return Start + (Dir * t); }
    };

    template<typename F>
    using Ray2D = Ray<2, F>;
    template<typename F>
    using Ray3D = Ray<3, F>;

    using Ray2Df = Ray2D<float>;
    using Ray3Df = Ray3D<float>;
}