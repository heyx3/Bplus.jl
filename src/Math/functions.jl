"""
Linear interpolation: a smooth transition from `a` to `b` based on a `t`.
`t=0` results in `a`, `t=1.0` results in `b`, `t=0.5` results in the midpoint between them.
"""
@inline lerp(a, b, t) = a + (t * (b - a))
export lerp


"Like typemin(), but returns a finite value for floats"
typemin_finite(T) = typemin(T)
typemin_finite(T::Type{<:AbstractFloat}) = nextfloat(typemin(T))
export typemin_finite

"Like typemax(), but returns a finite value for floats"
typemax_finite(T) = typemax(T)
typemax_finite(T::Type{<:AbstractFloat}) = prevfloat(typemax(T))
export typemax_finite


"
Rounds the given integer value up to the next multiple of some other integer value.
"
@inline pad_i(x::I1, multiple::I2) where {I1<:Integer, I2<:Integer} =
    ((x + convert(I1, multiple - one(I2))) // multiple) * multiple
export pad_i

"Solves the quadratic equation given a, b, and c."
function solve_quadratic( a::F, b::F, c::F
                        )::Option{Tuple{F, F}} where {F<:Real}
    discriminant::F = (b * b) - (convert(F, 4) * a * c)
    if discriminant == 0
        return nothing
    end

    s_term::F = sqrt(discriminant)
    denom::F = one(F) / a
    return (
        (s_term - b) * denom,
        -(s_term + b) * denom
    )
end
export solve_quadratic