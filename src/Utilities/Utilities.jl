module Utilities

Optional{T} = Union{T, Nothing}
exists(x) = !isnothing(x)
export Optional, exists

@inline none(args...) = !any(args...)
export none

"""
Game math is mostly done with 32-bit floats,
   especially when interacting with computer graphics.
This is a quick short-hand for making a 32-bit float.
"""
macro f32(f64)
    return :(Float32($(esc(f64))))
end
export @f32

include("asserts.jl")
include("enums.jl")

include("strings.jl")

end