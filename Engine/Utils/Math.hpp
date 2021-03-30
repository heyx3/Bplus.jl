#pragma once

#include <array>
#include <algorithm>
#include <optional>


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

    //Performs "lerp", both for GLM data and for regular numbers.
    template<typename Number_t, typename Float_t = AppropriateFloat_t<Number_t>>
    inline Number_t Lerp(Number_t a, Number_t b, Float_t t)
    {
        return Number_t{ (t * Float_t{b}) + ((1 - t) * Float_t{a}) };
    }
    //The GLM vector versions of Lerp() are defined
    //    in the same file that GLM itself is imported from, "Dependencies.h".
    //The interpolant can be a single number or a vector,
    //    and same goes for the start/end values.

    //Performs an inverse lerp on the given numbers.
    //The result is undefined if 'a' and 'b' are equal.
    template<typename T, typename Float_t = AppropriateFloat_t<T>>
    inline Float_t InverseLerp(T a, T b, T x)
    {
        return ((Float_t)x - (Float_t)a) / ((Float_t)b - (Float_t)a);
    }
    //The GLM vector versions of InverseLerp() are defined
    //    in the same file that GLM itself is imported from, "Dependencies.h".

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
}