module Math

using Setfield
using ..Utilities

#TODO: There's a macro that can check for type instability/performance issues. Use it on this code.

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