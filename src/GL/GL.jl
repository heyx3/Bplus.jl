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

include("context.jl")
#TODO: Drawing functions
# TODO: Give various object names with glObjectLabel

#TODO: Buffers, Textures, Targets, Meshes


# OpenGL/GLSL version info:
const OGL_MAJOR_VERSION = 4
const OGL_MINOR_VERSION = 6
const GLSL_VERSION = "#version 460"
const GLSL_EXTENSIONS = [
    #TODO: Ho do we load extensions? https://github.com/JuliaGL/ModernGL.jl/issues/70
    # "#extension GL_ARB_bindless_texture : require",
    # "#extension GL_ARB_gpu_shader_int64 : require"
]

end # module