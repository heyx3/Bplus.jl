
# Describes the various states of readiness a Target can be in.
# It's only usable for rendering if the state is `TargetStates.ready`.
@bp_enum(TargetStates,
    # Everything is fine and the Target can be used.
    ready,

    # One of the attachments uses a "compressed" format,
    #    which generally can't be rendered into.
    compressed_format,
    # Some attachments are layered (i.e. full 3D textures or cubemaps), and some aren't.
    layer_mixup,
    # Your graphics driver doesn't support this particular combination of texture formats;
    #    try simpler ones.
    driver_unsupported,

    # Some other unknown error has occurred.
    # Make sure asserts are on, and the Context is in "debug mode", to get more info.
    unknown
)
export E_TargetStates, TargetStates


"""
A destination for rendering, as opposed to rendering into the screen itself.
Has a set of Textures attached to it, described as `TargetOutput` instances,
    which receive the rendered output.

OpenGL calls these "framebuffers".

The set of attached textures is fixed after creation,
    but the Target can limit itself to any subset of the color attachments as desired.
"""
mutable struct Target <: Resource
    handle::Ptr_Target
    size::v2u

    attachment_colors::ReadOnlyVector{TargetOutput}
    attachment_depth::Optional{TargetOutput}
    attachment_stencil::Optional{TargetOutput}

    active_color_attachments::Vector{Int}  # Stored by index
    gl_buf_color_attachments::Vector{GLenum} # Array data to pass to the OpenGL C API.

    # Depth-and/or-stencil texture can be a TargetBuffer, instead of a Texture.
    is_depth_a_target_buffer::Bool
    is_stencil_a_target_buffer::Bool
    target_buffer_depth_stencil::Optional{TargetBuffer}

    # Some textures were created by this Target on construction, for convenience,
    #    and so they should be destroyed by this Target on destruction.
    owned_textures::Set{Texture}
end
#=
    For reference, here is the process for OpenGL framebuffers:
    1. Create it
    2. Attach textures/images to it, possibly layered,
         with `glNamedFramebufferTexture[Layer]()`.
    3. Set which subset of color attachments to use,
         with `glNamedFramebufferDrawBuffers()`
    4. Depth/stencil attachments are always used
    5. Draw!
=#

println("#TODO: Implement Target functions")

#TODO: Implement copying: http://docs.gl/gl4/glBlitFramebuffer
#TODO: A special singleton Target representing the screen?
