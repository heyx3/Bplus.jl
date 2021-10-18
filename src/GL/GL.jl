"A light wrapper around OpenGL"
module GL

using Setfield
using ModernGL, GLFW
using ..Utilities, ..Math


# OpenGL/GLSL version info:
const OGL_MAJOR_VERSION = 4
const OGL_MINOR_VERSION = 6
const GLSL_VERSION = "#version 460"
const GLSL_EXTENSIONS = [
    #TODO: How do we load extensions? https://github.com/JuliaGL/ModernGL.jl/issues/70
    # "#extension GL_ARB_bindless_texture : require",
    # "#extension GL_ARB_gpu_shader_int64 : require"
]

include("utils.jl")
include("handles.jl")

include("depth_stencil.jl")
include("blending.jl")
include("data.jl")
include("context.jl")

include("drawing.jl")

#TODO: Drawing functions
#TODO: Give various object names with glObjectLabel
#TODO: Buffers, Textures, Targets, Meshes

end # module