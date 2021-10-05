module Math

using Setfield
using ..Utilities

#TODO: There's a macro that can check for type instability/performance issues. Use it on this code.

@inline lerp(a, b, t) = a + (t * (b - a))
export lerp

include("vec.jl")
include("mat.jl")
include("quat.jl")

end # module