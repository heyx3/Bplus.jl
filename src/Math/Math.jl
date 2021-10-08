module Math

using Setfield
using ..Utilities

# Define @bp_math_assert.
@make_toggleable_asserts bp_math_

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

include("vec.jl")
include("mat.jl")
include("quat.jl")

end # module