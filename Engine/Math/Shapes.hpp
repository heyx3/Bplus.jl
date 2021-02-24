#pragma once

#include "Ray.hpp"


//Provides numerous helper functions for shapes,
//    including collision tests and ray-casts.

namespace Bplus::Math
{
    //Given an "up" vector (assumed to be normalized),
    //    generates a forward and side vector (in that order).
    //In a right-handed coordinate system, the "side" vector will point rightward
    //    (leftward for a left-handed system).
    template<typename F>
    std::pair<glm::vec<3, F>,
              glm::vec<3, F>>
        MakeBasis(const glm::vec<3, F>& up,
                  glm::vec<3, F> forward = { 1, 0, 0 })
    {
        BP_ASSERT(std::abs(glm::length2(forward) - 1) < (F)0.0001,
                 "Forward vector isn't normalized");
        BP_ASSERT(std::abs(glm::length2(  up   ) - 1) < (F)0.0001,
                 "Up vector isn't normalized");

        //If forward and up are equal, fabricate a new forward.
        if (forward == up)
            if (up.x == 1)
                forward = { 0, 0, 1 };
            else
                forward = { 1, 0, 0 };

        auto side = glm::cross(forward, up);
        forward = glm::cross(up, side);

        return std::make_pair(forward, side);
    }


    template<glm::length_t L, typename F>
    using Triangle = std::array<glm::vec<L, F>, 3>;

    template<typename F>
    struct Plane { glm::vec<3, F> Origin, Normal; };


    #pragma region Line intersections

    //Each of these functions is named "GetIntersection", and
    //    gets the intersection(s) between a line and some other object.
    //Intersections are represented by a floating-point value;
    //    to get the actual point, use "ray.GetAt(intersection)".

    //Most functions return "optional" values, indicating that the line
    //    may not hit the shape entirely.
    //If a shape can intersect a ray more than once,
    //    the function returns something like "pair<int, array<float, 2>>",
    //    where the first value indicates how many intersections exist,
    //    and the second value is a buffer for the intersections.
    //Some functions also take a boolean indicating whether to clamp the line's start to t=0.

    //2D line-line intersection:
    template<typename F>
    std::optional<std::pair<F, F>> GetIntersection(const Ray2D<F>& line1, const Ray2D<F>& line2)
    {
        //https://en.wikipedia.org/wiki/Line-line_intersection

        auto p1 = line1.Start,
             p2 = line1.Start + line1.Dir,
             p3 = line2.Start,
             p4 = line2.Start + line2.Dir;

        //Cache some terms that are done more than once in the equation.
        auto m_1_2 = p1 - p2,
             m_1_3 = p1 - p3,
             m_3_4 = p3 - p4;

        //Calculate.
        F determinant = (m_1_2.x * m_3_4.y) - (m_1_2.y * m_3_4.x);
        if (determinant == 0)
            return std::nullopt;
        F denom = (F)1.0 / determinant;
        return { { ((m_1_3.x * m_3_4.y) - (m_1_3.y * m_3_4.x)) * denom,
                   ((m_1_2.x * m_1_3.y) - (m_1_2.y * m_1_3.x)) * denom } };
    }

    //3D Line-Plane intersection:
    template<typename F>
    std::optional<F> GetIntersection(const Ray3D<F>& line, const Plane<F>& plane)
    {
        //Reference: https://en.wikipedia.org/wiki/Line-plane_intersection

        F determinant = glm::dot(line.Dir, plane.Normal);
        if (determ == 0)
            return std::nullopt;

        return glm::dot(plane.Normal, plane.Origin - line.Start) / determinant;
    }

    //3D Line-Triangle intersection:
    template<typename F>
    std::optional<F> GetIntersection(const Ray3D<F>& line,
                                     const Triangle<3, F>& triangle,
                                     bool lineIsRay = false,
                                     std::optional<glm::vec<3, F>> invRayDir = std::nullopt)
    {
        //The Möller-Trumbore algorithm:
        //    https://en.wikipedia.org/wiki/Möller–Trumbore_intersection_algorithm

        auto _invRayDir = invRayDir.has_value() ?
                              invRayDir.value() :
                              ((F)1.0 / line.Dir);

        auto edge1 = triangle[1] - triangle[0],
             edge2 = triangle[2] - triangle[0];
        auto h = glm::cross(line.Dir, edge2);

        F a = glm::dot(edge1, h);
        if (a == 0)
            return std::nullopt;

        F f = (F)1.0 / a;
        auto s = line.Start - triangle[0];
        F u = f * glm::dot(s, h);
        if ((u < 0) | (u > 1))
            return std::nullopt;

        auto q = glm::cross(s, edge1);
        F v = f * glm::dot(line.Dir, q);
        if ((v < 0) | ((u + v) > 1))
            return std::nullopt;

        F t = f * glm::dot(edge2, q);
        if (lineIsRay & (t < 0))
            return std::nullopt;
        else
            return t;
    }


    //Line-Box intersection (up to 2 intersection points):
    template<glm::length_t N, typename F>
    std::pair<uint_fast8_t, std::array<F, 2>>
        GetIntersection(const Ray<N, F>& line,
                        const Box<N, F>& box,
                        bool lineIsRay = false,
                        std::optional<glm::vec<3, F>> invRayDir = std::nullopt)
    {
        using vec_t = glm::vec<N, F>;

        auto _invRayDir = invRayDir.has_value() ?
                              invRayDir.value() :
                              ((F)1.0 / line.Dir);
        //The "Slab" method.
        //References:
        //    https://tavianator.com/cgit/dimension.git/tree/libdimension/bvh/bvh.c#n196
        //    https://github.com/tmpvar/ray-aabb-slab/blob/master/ray-aabb-slab.js
        //    https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525

        //Get the intersection with each cube face
        //    (actually the infinite plane that the face is on).
        //Solve `X=vt + x_0` for t and you get `t = (X - x_0) / v`.
        std::array<vec_t, 2> faceHitTs = {
            (box.MinCorner      - line.Start) * _invRayDir, // Hits on the min faces
            (box.GetMaxCorner() - line.Start) * _invRayDir  // Hits on the max faces
        };

        //Along each axis, get the closer and the farther of the two intersections.
        auto  closerFaceHits = glm::min(faceHitTs[0], faceHitTs[1]),
             fartherFaceHits = glm::max(faceHitTs[0], faceHitTs[1]);

        //Find the actual intersection min and max.
        F minT = glm::compMax(closerFaceHits),
          maxT = glm::compMin(fartherFaceHits);

        //Count the number of intersections and output.
        if (minT > maxT)
            return { 0, std::array<F, 2>{ 0, 0 } };
        else if (lineIsRay & (minT < 0))
            return { 1, std::array<F, 2>{ maxT, -1 } };
        else
            return { 2, std::array<F, 2>{ minT, maxT } };
    }


    //Line-Sphere intersection (up to 2 intersection points):
    template<glm::length_t N, typename F>
    std::pair<uint_fast8_t, std::array<F, 2>>
        GetIntersection(const Ray<N, F>& line,
                        const Sphere<N, F>& sphere,
                        bool lineIsRay = false,
                        std::optional<glm::vec<3, F>> invRayDir = std::nullopt)
    {
        using namespace std;

        //References:
        //    https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
        //    https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
            
        auto sphereToRayStart = line.Start - sphere.Center;
            
        F a = glm::length2(line.Dir);
        F b = 2 * glm::dot(line.Dir, sphereToRayStart);
        F c = glm::length2(sphereToRayStart) - (sphere.Radius * sphere.Radius);
        std::array<F, 2> hits;
        if (!SolveQuadratic(a, b, c, hits))
            return { 0, std::array<F, 2>{ 0, 0 } };

        if (hits[0] > hits[1])
            swap(hits[0], hits[1]);

        if (lineIsRay & (hits[0] < 0))
            if (hits[1] < 0)
                return { 0, std::array<F, 2>{ 0, 0 } };
            else
                return { 1, std::array<F, 2>{ hits[1], 0 } };
        else
            return { 2, hits };
    }

    #pragma endregion


    #pragma region Collision tests

    //These functions test whether two shapes are touching;
    //    they all have the signature "bool Touches(...)".

    //Note that the number type is templated as
    //    "typename T" if supporting both integer and float types,
    //    or "typename F" if only supporting float types.

    #pragma region Box-Point collision test

    template<glm::length_t N, typename T>
    bool Touches(const      Box<N, T>& box,
                 const glm::vec<N, T>& point)
    {
        return glm::all(glm::greaterThanEqual(point, box.MinCorner) &
                        glm::lessThan(point, box.GetMaxCorner()));
    }

    //A version that flips the parameter order, for completeness:
    template<glm::length_t N, typename T>
    bool Touches(const glm::vec<N, T>& point, const Box<N, T>& box) { return Touches(box, point); }

    #pragma endregion
    
    #pragma region Sphere-Point collision test

    template<glm::length_t N, typename T>
    bool Touches(const   Sphere<N, T>& sphere,
                 const glm::vec<N, T>& point)
    {
        return glm::distance2(sphere.Center, point) <= (sphere.Radius * sphere.Radius);
    }

    //A version that flips the parameter order, for completeness:
    template<glm::length_t N, typename T>
    bool Touches(const glm::vec<N, T>& point, const Sphere<N, T>& sphere) { return Touches(sphere, point); }

    #pragma endregion

    //Box-box collision test:
    template<glm::length_t N, typename T>
    bool Touches(const Box<N, T>& box1, const Box<N, T>& box2)
    {
        return glm::all(glm::lessThan(box1.MinCorner, box2.GetMaxCorner()) &
                        glm::greaterThan(box1.GetMaxCorner(), box2.MinCorner));
    }

    //Sphere-sphere collision test:
    template<glm::length_t N, typename T>
    bool Touches(const Sphere<N, T>& sphere1, const Sphere<N, T>& sphere2)
    {
        Sphere<N, T> testSphere{ sphere1.Center,
                                 sphere1.Radius + sphere2.Radius };
        return testSphere.Touches(sphere2.Center);
    }

    #pragma region Sphere-Box collision test
    
    template<glm::length_t N, typename T>
    bool Touches(const Box<N, T>& box, const Sphere<N, T>& sphere)
    {
        return sphere.Touches(box.GetClosestPointTo(sphere.Center));
    }

    //A version that flips the parameter order, for completeness:
    template<glm::length_t N, typename T>
    bool Touches(const Sphere<N, T>& sphere, const Box<N, T>& box) { return Touches(box, sphere); }

    #pragma endregion

    //Line-Box collision test:
    template<glm::length_t N, typename F>
    bool Touches(const Ray<N, F>& line,
                 const Box<N, F>& box,
                 bool lineIsRay = false,
                 std::optional<glm::vec<3, F>> invRayDir = std::nullopt)
    {
        return GetIntersection(line, box, lineIsRay, invRayDir).first > 0;
    }

    //Line-Sphere collision test:
    template<glm::length_t N, typename F>
    bool Touches(const Ray<N, F>& line,
                 const Sphere<N, F>& sphere,
                 bool lineIsRay = false,
                 std::optional<glm::vec<3, F>> invRayDir = std::nullopt)
    {
        return GetIntersection(line, box, lineIsRay, invRayDir).first > 0;
    }

    #pragma endregion
}