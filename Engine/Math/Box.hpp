#pragma once

#include <glm/glm.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/hash.hpp>

#include "../Utils.h"

namespace Bplus::Math
{
    //An axis-aligned, N-dimensional rectangle represented with coordinates of type T.
    //T should be a float or an integer (or any custom number type
    //    that implements comparisons and std::numeric_limits).
    template<glm::length_t N, typename T>
    struct Box
    {
        using vec_t = glm::vec<N, T>;
        static constexpr T Epsilon()
        {
            //std::numeric_limits<T>::epsilon() returns 0 for integers, not 1,
            //    so we need to check the type of T in order to find its epsilon.
            if constexpr (std::is_integral_v<T>)
                return 1;
            else
                return std::numeric_limits<T>::epsilon();
        }

        vec_t MinCorner, Size;


        //Constructors with different types of inputs.
        static Box<N, T> MakeMinMax(const vec_t& minCorner, const vec_t& maxCorner)
        {
            if constexpr(std::is_unsigned_v<T>)
                BPAssert(glm::all(glm::greaterThanEqual(maxCorner, minCorner)),
                         "Box with unsigned number type can't have negative size");

            return { minCorner, maxCorner - minCorner };
        }
        static Box<N, T> MakeCenterSize(const vec_t& center, const vec_t& size)
        {
            auto halfSize = size / (T)2;
            return { center - halfSize, center + halfSize };
        }
        static Box<N, T> MakeMinSize(const vec_t& minCorner, const vec_t& size)
        {
            return { minCorner, size };
        }
        static Box<N, T> MakeSize(const vec_t& size)
        {
            return { vec_t{ 0 }, size };
        }

        //Constructs a bounding box for the given iterator of vec_t.
        template<typename IterT>
        static Box<N, T> BoundPoints(IterT begin, IterT end)
        {
            vec_t min{ std::numeric_limits<T>::max() },
                  max{ std::numeric_limits<T>::min() };
            for (auto iter = begin; iter != end; ++iter)
            {
                min = glm::min(min, *iter);
                max = glm::max(max, *iter);
            }

            return MakeMinMax(min, max);
        }

        
        //Gets the exclusive max corner of this rectangle.
        vec_t GetMaxCorner() const { return MinCorner + Size; }
        vec_t GetMaxCornerInclusive() const { return MinCorner + Size - Epsilon(); }

        //Gets half the size of this rectangle.
        vec_t GetHalfSize() const { return Size / (T)2; }

        //Gets the N-dimensional volume of this box.
        //Note that if Size has negative values, this volume may be negative.
        T GetVolume() const { return glm::compMul(Size); }

        //Gets whether this box has no volume (area, length, etc).
        //Note that a box with "negative" volume will count as "empty".
        bool IsEmpty() const
        {
            vec_t zero{ (T)0 };
            return glm::all(glm::lessThanEqual(Size, zero));
        }

        //Gets whether the point is inside the box (i.e. inside the edges, not touching them).
        //For floating-point boxes, this isn't really different from Touches().
        bool IsInside(const vec_t& point) const
        {
            return glm::all(glm::greaterThan(point, MinCorner) &
                            glm::lessThan(point, GetMaxCornerInclusive()));
        }
        bool IsInside(const Box<N, T>& outer) const
        {
            return IsInside(outer.MinCorner) &
                   IsInside(outer.GetMaxCorner());
        }

        //Returns the intersection of this box with the given one.
        //If there is no intersection, the width and/or height will be 0.
        Box<N, T> GetIntersection(const Box<N, T>& other) const
        {
            auto newMin = glm::max(MinCorner, other.MinCorner);
            auto newMax = glm::min(GetMaxCorner(), other.GetMaxCorner());

            //Make sure the intersection doesn't have negative size (clamp it to 0),
            //    in case we're working with unsigned numbers.
            newMax = glm::max(newMax, newMin);

            return MakeMinMax(newMin, newMax);
        }
        //Gets the smallest Box which contains both this one and the given one.
        Box<N, T> GetUnion(const Box<N, T>& other) const
        {
            return MakeMinMax(glm::min(MinCorner, other.MinCorner),
                              glm::max(GetMaxCorner(), other.GetMaxCorner()));
        }

        //Gets the closest point on this box to the given point.
        //If the point is inside the box, then it itself is returned.
        vec_t GetClosestPointTo(const vec_t& point) const
        {
            return glm::clamp(point, MinCorner, GetMaxCornerInclusive());
        }

        //Casts this box to a box of the given number of dimensions,
        //    assuming that any new dimensions are positioned at 0
        //    and have the smallest non-zero size.
        template<glm::length_t N2>
        Box<N2, T> ChangeDimensions() const
        {
            const auto& me = *this;
            Box<N2, T> newBox;

            for (glm::length_t d = 0; d < N; ++d)
            {
                newBox.MinCorner[d] = me.MinCorner[d];
                newBox.Size[d] = me.Size[d];
            }
            for (glm::length_t d = N; d < N2; ++d)
            {
                newBox.MinCorner[d] = 0;
                newBox.Size[d] = Box<N2, T>::Epsilon();
            }

            return newBox;
        }


        bool operator==(const Box<N, T>& other) const
        {
            return (MinCorner == other.MinCorner) &
                   (Size == other.Size);
        }
        bool operator!=(const Box<N, T>& other) const { return !operator==(other); }
    };


    //Some type aliases for the most common use-cases:

    template<typename T>
    using Box2D = Box<2, T>;
    template<typename T>
    using Box3D = Box<3, T>;
    template<typename T>
    using Box4D = Box<4, T>;

    using Box2Df = Box2D<float>;
    using Box3Df = Box3D<float>;
    using Box4Df = Box4D<float>;

    using Box2Du = Box2D<glm::u32>;
    using Box3Du = Box3D<glm::u32>;
    using Box4Du = Box4D<glm::u32>;

    using Box2Di = Box2D<glm::i32>;
    using Box3Di = Box3D<glm::i32>;
    using Box4Di = Box4D<glm::i32>;

    
    //A type alias "Interval", for 1D boxes.

    template<typename T>
    using Interval = Box<1, T>;

    using IntervalF = Interval<float>;
    using IntervalI = Interval<glm::i32>;
    using IntervalU = Interval<glm::u32>;
    using IntervalUL = Interval<glm::u64>;
}


//Allow Box<> to be hashed, for use in STL collections.
namespace std {
    template<glm::length_t N, typename T>
    struct hash<Bplus::Math::Box<N, T>> {
        size_t operator()(const Bplus::Math::Box<N, T>& value) const {
            return Bplus::MultiHash(value.MinCorner, value.Size);
        }
    };
}