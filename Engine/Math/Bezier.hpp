#pragma once

#include <glm/glm.hpp>
#include <glm/fwd.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/hash.hpp>

#include "../Utils.h"

namespace Bplus::Math
{
    //A Bezier curve with N-dimensional points.
    template<glm::length_t N, typename Float_t = float>
    struct Bezier
    {
        using vec_t = glm::vec<N, Float_t>;

        vec_t Start, End;
        vec_t StartTangent, EndTangent;

        vec_t GetStartControl() const { return Start + StartTangent; }
        vec_t GetEndControl() const { return End + EndTangent; }

        //Finds the value at the given time (from 0 to 1).
        vec_t Evaluate(Float_t t) const
        {
            Float_t inverseT = Float_t{1} - t,
                    inverseT_2 = inverseT * inverseT,
                    t_2 = t * t;
            return (Start             * (inverseT * inverseT_2)) +
                   (GetStartControl() * (Float_t{3} * inverseT_2 * t)) +
                   (GetEndControl()   * (Float_t{3} * inverseT * t_2)) +
                   (End               * (t_2 * t));
        }
        //Finds the value at the given time (from 0 to 1).
        //Also returns the derivative at that time.
        std::pair<vec_t, vec_t> Evaluate2(Float_t t) const
        {
            vec_t startControl = GetStartControl(),
                  endControl = GetEndControl();
            Float_t inverseT = Float_t{1} - t,
                    inverseT_2 = inverseT * inverseT,
                    inverseT_2__3 = inverseT_2 * Float_t{3},
                    t_2 = t * t,
                    t_2__3 = t_2 * Float_t{3};

            vec_t value = (Start        * (inverseT * inverseT_2)) +
                          (startControl * (inverseT_2__3 * t)) +
                          (endControl   * (t_2__3 * inverseT)) +
                          (End          * (t_2 * t)),
             derivative = (StartTangent                * (inverseT_2__3)) +
                          ((endControl - startControl) * (Float_t{6} * inverseT * t)) +
                          (EndTangent                  * -t_2__3);

            return { value, derivative };
        }
        //Finds the value at the given time (from 0 to 1).
        //Also returns the derivative and second derivative at that time.
        std::tuple<vec_t, vec_t, vec_t> Evaluate3(Float_t t) const
        {
            vec_t startControl = GetStartControl(),
                  endControl = GetEndControl(),
                  deltaControl = endControl - startControl;

            Float_t inverseT = Float_t{1} - t,
                    inverseT_2 = inverseT * inverseT,
                    inverseT_2__3 = inverseT_2 * Float_t{3},
                    inverseT__6 = inverseT * 6,
                    t_2 = t * t,
                    t_2__3 = t_2 * Float_t{3};

            vec_t value = (Start        * (inverseT * inverseT_2)) +
                          (startControl * (inverseT_2__3 * t)) +
                          (endControl   * (t_2__3 * inverseT)) +
                          (End          * (t_2 * t)),
             derivative = (StartTangent * (inverseT_2__3)) +
                          (deltaControl * (inverseT__6 * t)) +
                          (EndTangent   * -t_2__3),
            derivative2 = ((deltaControl - startControl + Start)     * (inverseT__6)) +
                          ((-EndTangent - endControl + startControl) * (t * Float_t{6}));

            return { value, derivative, derivative2 };
        }

        //TODO: A "CalculateLength()" function.
    };


    //TODO: A "BezierPoly" struct.
}