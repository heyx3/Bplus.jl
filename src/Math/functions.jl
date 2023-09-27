"""
Linear interpolation: a smooth transition from `a` to `b` based on a `t`.
`t=0` results in `a`, `t=1.0` results in `b`, `t=0.5` results in the midpoint between them.
"""
@inline lerp(a, b, t) = a + (t * (b - a))

"""
Smooth interpolation: a non-linear version of lerp
    that moves faster around 0.5 and slower around the ends (0 and 1),
    creating a more organic transition.
Unlike lerp(), this function clamps t to the range (0, 1),
    as its behavior outside this range is not intuitive.
"""
@inline function smoothstep(t::TVec) where {TVec<:Vec}
    t = clamp(t, zero(TVec), one(TVec))
    THREE = TVec(Val(3))
    TWO = TVec(Val(2))
    return t * t * (THREE - (TWO * t))
end
@inline smoothstep(t::Number) = smoothstep(Vec(t)).x
@inline smoothstep(a, b, t) = lerp(a, b, smoothstep(t))

"""
An even smoother version of `smoothstep()`, at the cost of a little performance.
Again, unlike lerp(), this function clamps t to the range (0, 1),
    as its behavior outside this range is not intuitive.
"""
@inline function smootherstep(t::TVec) where {TVec<:Vec}
    t = clamp(t, zero(TVec), one(TVec))
    SIX = TVec(Val(6))
    TEN = TVec(Val(10))
    nFIFTEEN = TVec(Val(-15))
    return t * t * t * (TEN + (t * (nFIFTEEN + (t * SIX))))
end
@inline smootherstep(t::Number) = smootherstep(Vec(t)).x
@inline smootherstep(a, b, t) = lerp(a, b, smootherstep(t))

export lerp, smoothstep, smootherstep


"The inverse of `lerp()`. Given a min, max, and value, finds the interpolant for that value."
@inline inv_lerp(a, b, x) = (x - a) / (b - a)
"An alternative inverse-lerp that uses integer division to return an integer value."
@inline inv_lerp_i(a, b, x) = (x - a) ÷ (b - a)
export inv_lerp, inv_lerp_i

"
Gets the fractional part of f.

Negative values wrap around; for example `fract(-0.1) == 0.9` (within floating-point error).
This matches the behavior of GLSL's `fract()`, and I believe HLSL's `frac()` too.
"
@inline fract(f) = f - floor(f)

"Clamps between 0 and 1."
@inline saturate(f) = clamp(f, zero(typeof(f)), one(typeof(f)))

export fract, saturate


"Like typemin(), but returns a finite value for floats"
typemin_finite(T) = typemin(T)
typemin_finite(T::Type{<:AbstractFloat}) = nextfloat(typemin(T))
export typemin_finite

"Like typemax(), but returns a finite value for floats"
typemax_finite(T) = typemax(T)
typemax_finite(T::Type{<:AbstractFloat}) = prevfloat(typemax(T))
export typemax_finite

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

"Multiplies a value by itself"
square(x) = (x*x)
export square

"
Rounds a value up to the nearest multiple of another value.
Works with both integers and `Vec`s of integers (`Vec{<:Integer}`).

Behavior with float inputs is not defined, because I haven't bothered to think about it
    (feel free to do it yourself and update this comment!)

Behavior with negative inputs is not defined, because I haven't bothered to think about it
    (feel free to do it yourself and update this comment!)
"
function round_up_to_multiple(value, multiple)
    (quotient, remainder) = divrem(value, multiple)
    return iszero(remainder) ?
        value :
        value + (multiple - remainder)
end
export round_up_to_multiple