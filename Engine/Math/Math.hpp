#pragma once

#include <array>
#include <algorithm>
#include <optional>

#include "../Dependencies.h"
#include <glm/gtc/matrix_access.hpp>


namespace Bplus::Math
{
    //Not actually defined in the standard...
    const double PI = 3.1415926535897932384626433832795028841971693993751;
    const double E  = 2.71828182845904523536028747135266;

    //Defined as 'double' for 64-bit numbers, and 'float' for everything else.
    template<typename Number_t>
    using AppropriateFloat_t = std::conditional_t<std::disjunction_v<std::is_same<Number_t, double>,
                                                                     std::is_same<Number_t, uint64_t>,
                                                                     std::is_same<Number_t, int64_t>>,
                                                  double,
                                                  float>;


    //Rounds the given integer value up to the next multiple of some other integer value.
    //Supports both plain numbers and GLM vectors.
    template<typename N1, typename N2>
    inline auto PadI(N1 x, N2 multiple) { return ((x + N1{(multiple - N2{1})}) / multiple) * multiple; }

    //Solves the given quadratic equation given a, b, and c.
    //Returns whether there were any real solutions.
    //Note that if there is only one solution, both elements of "result" will be set to it.
    //If there are two solutions, they will be returned in ascending order.
    template<typename F>
    inline bool SolveQuadratic(F a, F b, F c, std::array<F, 2>& result)
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
    inline Float_t Log(Float_t x, Float_t base)
    {
        using std::log10;
        return log10(x) / log10(base);
    }

    //Performs an inverse lerp on the given numbers.
    //The result is undefined if 'a' and 'b' are equal.
    template<typename T, typename Float_t = AppropriateFloat_t<T>>
    inline Float_t InverseLerp(T a, T b, T x)
    {
        return ((Float_t)x - (Float_t)a) / ((Float_t)b - (Float_t)a);
    }
    //Performs an inverse lerp on the given GLM vectors.
    //The result is undefined if 'a' and 'b' are equal in at least one component.
    template<glm::length_t L, typename T, enum glm::qualifier Q = glm::packed_highp,
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

    //Checks whether an integer value is within range of another integer type.
    template<typename SmallerInt_t, typename Int_t>
    inline bool IsInRange(Int_t i)
    {
        static_assert(!std::is_same_v<SmallerInt_t, Int_t>,
                      "The input type and desired type are the same!"
                        " We don't allow that to make sure this behavior is intentional.");
        return (i >= std::numeric_limits<SmallerInt_t>::min()) &
               (i <= std::numeric_limits<SmallerInt_t>::max());
    }

    //Addition that protects against overflow and underflow.
    template<typename Int_t>
    inline std::optional<Int_t> SafeAdd(Int_t a, Int_t b)
    {
        constexpr auto maxVal = std::numeric_limits<Int_t>().max(),
                       minVal = std::numeric_limits<Int_t>().min();
        if (((a > 0) & (maxVal - a < b)) |
            ((a < 0) & (minVal - a > b)))
        {
            return std::nullopt;
        }
        
        return a + b;
    }
    //Subtraction that protects against overflow and underflow.
    template<typename Int_t>
    inline std::optional<Int_t> SafeSub(Int_t a, Int_t b)
    {
        constexpr auto maxVal = std::numeric_limits<Int_t>().max(),
                       minVal = std::numeric_limits<Int_t>().min();
        if (((b < 0) & (maxVal + b < a)) |
            ((b > 0) & (minVal + b > a)))
        {
            return std::nullopt;
        }

        return a - b;
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
}