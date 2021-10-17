struct StencilState
    test::StencilTest
    result::StencilResult
    mask::GLuint

    StencilState(test, result, mask = ~GLuint(0)) = new(test, result, mask)
    StencilResult() = StencilState(StencilTest(), StencilResult())
end

"Global OpenGL state, mostly related to the fixed-function stuff like blending"
struct RenderState
    color_write_mask::vRGBA{Bool}
    depth_write::Bool

    cull_mode::E_FaceCullModes
    #TODO: Port the "Box" from C++ and use it for viewoprt and scissor.
    viewport_min::v2i
    viewport_max::v2i
    scissor_min::v2i
    scissor_max::v2i
    # TODO: Changing viewport Y axis and depth (how can GLM support this depth mode?): https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glClipControl.xhtml

    # There can be different blend modes for Color vs Alpha.
    blend_mode::Union{BlendStateRGBA,
                      Tuple{BlendStateRGB, BlendStateAlpha}}

    depth_test::E_ValueTests

    # There can be different stencil behaviors for the front-faces and back-faces.
    stencil::Union{StencilState,
                   @NamedTuple(front::StencilState, back::StencilState)}
end


"
The OpenGL context, which owns all OpenGL state and data.
This type is very special, in a lot of ways:
    * It is a per-thread singleton, because OpenGL only allows one context per thread
    * You create and register it simply by calling the constructor
    * You can get the fields, but you cannot set them manually
It's recommended to surround all OpenGL work in a `bp_gl_context()` block.
"
mutable struct Context
    state::RenderState
    
    #TODO: Vsync mode
    #TODO: The active RenderTarget
    #TODO: The GLFW window

    #TODO: Constructor that registers and starts the context
end
export Context

get_context()::Optional{Context} = CONTEXTS_PER_THREAD[Threads.threadid()]
export get_context

Base.setproperty!(c::Context, name::Symbol, new_val) =
    error("Cannot set the fields of `Bplus.GL.Context`")

#TODO: Setters for render state/viewport/scissor.

#TODO: Set up callbacks for 'destroyed' and 'refresh state'.

#TODO: A 'close()' overload for closing the context

# Set up the thread-local storage.
const CONTEXTS_PER_THREAD = Vector{Optional{Context}}()
module _Init_Contexts
    import ..CONTEXTS_PER_THREAD
    function __init__()
        global CONTEXTS_PER_THREAD
        empty!(CONTEXTS_PER_THREAD)
        for i in 1:Threads.nthreads()
            push!(CONTEXTS_PER_THREAD, nothing)
        end
    end
end