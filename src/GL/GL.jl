"A light wrapper around OpenGL"
module GL

using Setfield
using ModernGL, GLFW
using ..Utilities, ..Math

# @bp_gl_assert
@make_toggleable_asserts bp_gl_

# A set of callbacks that can run on this module's __init__().
const RUN_ON_INIT = [ ]
function __init__()
    for f::Function in RUN_ON_INIT
        f()
    end
end

#TODO: Base type for all GL resource objects (i.e. no getproperty(), and other things)


# OpenGL info:
const OGL_MAJOR_VERSION = 4
const OGL_MINOR_VERSION = 6
const OGL_REQUIRED_EXTENSIONS = [
    "GL_ARB_bindless_texture",
    "GL_ARB_gpu_shader_int64"
]

const GLSL_VERSION = "#version 460"
const GLSL_EXTENSIONS = [
    #TODO: How do we load extensions? https://github.com/JuliaGL/ModernGL.jl/issues/70
    "#extension GL_ARB_bindless_texture : require",
    "#extension GL_ARB_gpu_shader_int64 : require"  #TODO: Build in conditional support for extensions like this one
]

include("utils.jl")
include("handles.jl")

include("depth_stencil.jl")
include("blending.jl")
include("data.jl")
include("context.jl")

include("buffer.jl")
include("mesh.jl")

include("textures/format.jl")
include("textures/sampling.jl")
include("textures/data.jl")
include("textures/texture.jl")

include("drawing.jl")

#TODO: Give various object names with glObjectLabel
#TODO: Buffers, Textures, Targets, Meshes

end # module