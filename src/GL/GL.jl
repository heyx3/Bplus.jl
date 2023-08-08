"A light wrapper around OpenGL"
module GL

using Setfield, TupleTools, StructTypes
using ModernGLbp, GLFW, CSyntax
using ..Utilities, ..Math

@decentralized_module_init

# OpenGL info:
const OGL_MAJOR_VERSION = 4
const OGL_MINOR_VERSION = 6
const OGL_REQUIRED_EXTENSIONS = (
    "GL_ARB_bindless_texture",
    "GL_ARB_seamless_cubemap_per_texture", # Needed to get seamless cubemaps with bindless textures.
    "GL_ARB_gpu_shader_int64"
)

"A snippet that should be placed at the top of all shaders to ensure B+ compatibility"
const GLSL_HEADER = string(
    "#version ", OGL_MAJOR_VERSION, OGL_MINOR_VERSION, 0, "\n",
    "#extension GL_ARB_bindless_texture : require\n",
    "#extension GL_ARB_gpu_shader_int64 : require\n",
    # Don't reference ARB_seamless_cubemap_per_texture in GLSL, for some reason it's rejected
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

include("targets/target_buffer.jl")
include("targets/target_output.jl")
include("targets/target.jl")

include("program.jl")
include("drawing.jl")

end # module