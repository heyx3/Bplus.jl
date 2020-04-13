#pragma once

#include <glm/glm.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>

#include "../Utils.h"


namespace Bplus::Math
{
    //An N-dimensional sphere represented with coordinates of type T.
    //T should be a float (or any custom type that implements
    //    comparisons and std::numeric_limits).
    template<glm::length_t N, typename T>
    struct Sphere
    {
        using vec_t = glm::vec<N, T>;

        vec_t Center;
        T Radius;

        //Gets the N-dimensional volume of this box.
        //Note that if Radius is negative, this volume will be negative.
        T GetVolume() const
        {
            //There's a general formula for N-dimensional balls,
            //    but for the most common use-cases, we use the simplified formulas
            //    that minimize the number of computations.
            if constexpr (N == 0)
                return 0;
            else if constexpr (N == 1)
                return Radius * 2;
            else if constexpr (N == 2)
                return glm::pi<T>() * (Radius * Radius);
            else if constexpr (N == 3)
                return glm::pi<T>() * ((T)4 / 3) * (Radius * Radius * Radius);
            else
                //https://en.wikipedia.org/wiki/Volume_of_an_n-ball
                //There is a simple recursive formula that
                //    weirdly jumps down 2 dimensions at a time.
                return 2 * glm::pi<T>() * (Radius * Radius)
                         * Sphere<N-2, T>{{0}, Radius}.GetVolume();
        }

        bool IsEmpty() const { return Radius <= 0; }

        //Gets the closest point on this sphere to the given point.
        //If the point is inside the sphere, then it itself is returned.
        vec_t GetClosestPointTo(const vec_t& point) const
        {
            auto toPoint = point - Center;
            
            auto len2 = glm::length2(toPoint);
            if (len2 > Radius * Radius)
                toPoint /= glm::sqrt(len2);
            
            return Center + toPoint;
        }

        //TODO: Approximate intersection/unions. https://mathworld.wolfram.com/Sphere-SphereIntersection.html
    };

    using Sphere2D = Sphere<2, float>;
    using Sphere3D = Sphere<3, float>;
}