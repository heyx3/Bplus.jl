"A light wrapper around OpenGL"
module GL

using Setfield, TupleTools
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

# OpenGL info:
const OGL_MAJOR_VERSION = 4
const OGL_MINOR_VERSION = 6
const OGL_REQUIRED_EXTENSIONS = (
    "GL_ARB_bindless_texture",
    "GL_ARB_gpu_shader_int64" #TODO: Build in conditional support for extensions like this one
)
#TODO: How do we load extensions? https://github.com/JuliaGL/ModernGL.jl/issues/70

"A snippet that should be placed at the top of all shaders to ensure B+ compatibility"
const GLSL_HEADER = string(
    "#version ", OGL_MAJOR_VERSION, OGL_MINOR_VERSION, 0, "\n",
    (string("#extension ", ex, " : require\n") for ex in OGL_REQUIRED_EXTENSIONS)...,
    "#line 1\n"
)

include("utils.jl")
include("debugging.jl")

include("handles.jl")
include("depth_stencil.jl")
include("blending.jl")
include("data.jl")

include("context.jl")
include("resource.jl")

include("buffers/buffer.jl")
include("buffers/vertices.jl")
include("buffers/mesh.jl")

include("textures/format.jl")
include("textures/sampling.jl")
include("textures/data.jl")
include("textures/cube.jl")
include("textures/views.jl")
include("textures/view_debugging.jl")
include("textures/texture.jl")

include("program.jl")
include("drawing.jl")

#TODO: Give various object names with glObjectLabel
#TODO: Targets

end # module