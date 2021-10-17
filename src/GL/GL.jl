"A light wrapper around OpenGL"
module GL

using ModernGL
using ..Utilities, ..Math

# Define @bp_gl_assert
@make_toggleable_asserts bp_gl_


#TODO: Figure out why this macro is so finnicky. See "temp.jl".
"Short-hand for making enums based on OpenGL constants"
macro bp_gl_enum(name, args...)
    return Utilities.generate_enum(name, :(begin using ModernGL end), args)
end

include("handles.jl")
include("depth_stencil.jl")
include("blending.jl")
include("data.jl")

"""
An OpenGL environment.
You should only interact with it on the main thread.
"""
mutable struct Context

end

end # module