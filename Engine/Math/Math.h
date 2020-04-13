#pragma once

#include <array>

namespace Bplus::Math
{
    //Solves the given quadratic equation given a, b, and c.
    //Returns whether there were any real solutions.
    //Note that if there is only one solution, both elements of "result" will be set to it.
    //If there are two solutions, they will be returned in ascending order.
    template<typename F>
    bool SolveQuadratic(F a, F b, F c, std::array<F, 2>& result)
    {
        using std;

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
}