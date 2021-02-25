#pragma once

#include <array>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/fwd.hpp>

namespace Bplus::Math
{
    //Not actually defined in the standard...
    const double PI = 3.1415926535897932384626433832795028841971693993751;
    const double E  = 2.71828182845904523536028747135266;


    //Rounds the given integer value up to the next multiple of some other integer value.
    //Supports both plain numbers and GLM vectors.
    template<typename N1, typename N2>
    auto PadI(N1 x, N2 multiple) { return ((x + N1{(multiple - N2{1})}) / multiple) * multiple; }

    //Solves the given quadratic equation given a, b, and c.
    //Returns whether there were any real solutions.
    //Note that if there is only one solution, both elements of "result" will be set to it.
    //If there are two solutions, they will be returned in ascending order.
    template<typename F>
    bool SolveQuadratic(F a, F b, F c, std::array<F, 2>& result)
    {
        using namespace std;

        F discriminant = (b * b) - (4 * a * c);
        if (discriminant == 0)
            return false;

        float q = (b > 0) ?
                      -0.5 * (b + sqrt(discriminant)) :
                      -0.5 * (b - sqrt(discriminant));
        result[0] = q / a;
        result[1] = c / q;

        if (result[0] > result[1])
            swap(result[0], result[1]);

        return true;
    }

    //Gets the log of some number 'x' in a desired base.
    template<typename Float_t>
    Float_t Log(Float_t x, Float_t base)
    {
        using std::log10;
        return log10(x) / log10(base);
    }


    //GLM helpers:

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
}