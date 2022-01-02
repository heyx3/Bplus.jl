module Math

using Setfield
using ..Utilities

# Define @bp_math_assert.
@make_toggleable_asserts bp_math_

include("vec.jl")
include("functions.jl")

include("mat.jl")
include("quat.jl")

include("box.jl")
include("sphere.jl")

#TODO: Port Shapes.hpp
include("ray.jl")

include("noise.jl")

end # module