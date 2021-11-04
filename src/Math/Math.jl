module Math

using Setfield
using ..Utilities

# Define @bp_math_assert.
@make_toggleable_asserts bp_math_

include("functions.jl")
include("prng.jl")

include("vec.jl")
include("mat.jl")
include("quat.jl")

include("box.jl")
include("sphere.jl")

#TODO: Port Shapes.hpp
include("ray.jl")

end # module