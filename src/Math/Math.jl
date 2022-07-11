module Math

using Setfield, StaticArrays, StructTypes
using ..Utilities

# Define @bp_math_assert.
@make_toggleable_asserts bp_math_

include("vec.jl")
include("functions.jl")
include("contiguous.jl")

include("mat.jl")
include("quat.jl")
include("transformations.jl")

include("box.jl")
include("sphere.jl")

#TODO: Port Shapes.hpp
include("ray.jl")

include("noise.jl")

end # module