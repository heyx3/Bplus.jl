#######################
#   Creation Errors   #
#######################

"
Thrown if the graphics driver doesn't support a particular combination of texture attachments.
This is the only Target-related error that can't be foreseen, so it's given special representation;
    the others are thrown with `error()`.
"
struct TargetUnsupportedException <: Exception
    color_formats::Vector{TexFormat}
    depth_format::Optional{E_DepthStencilFormats}
end
Base.showerror(io::IO, e::TargetUnsupportedException) = print(io,
    "Your graphics driver doesn't support this particular combination of attachment formats;",
    " try simpler ones.\n\tThe color attachments: ", e.color_formats,
    "\n\tThe depth attachment: ", (exists(e.depth_format) ? e.depth_format : "[none]")
)
#=
        string("At least one of the attachments is a compressed-format texture,",
               " which generally can't be rendered into.")
    elseif e.reason == TargetErrors.layer_mixup
        string("Some attachments are layered (i.e. full 3D textures or cubemaps),",
               " and some aren't.")
    elseif e.reason == TargetErrors.driver_unsupported
        string("Your graphics driver doesn't support this particular combination",
               " of texture formats; try simpler ones.")
    elseif e.reason == TargetErrors.unknown
        string("Some unknown error has occurred. You may get more info",
               " by turning on Bplus.GL asserts, and starting the Context in \"debug mode\".")
    else
        string("ERROR_UNHANDLED(", e.reason, ")")
    end
=#


######################################
#   Struct/constructors/destructor   #
######################################

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
    attachment_depth_stencil::Union{Nothing, TargetOutput, TargetBuffer}

    active_color_attachments::ReadOnlyVector{Int}  # Stored by index
    gl_buf_color_attachments::ReadOnlyVector{GLenum} # Array data to pass to the OpenGL C API.

    # Textures which the Target will clean up automatically when it's destroyed.
    # Some Target constructors automatically create textures for you, for convenience,
    #    and those get added to this set.
    # Feel free to add/remove Textures as desired.
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


"
Creates a target with no outputs,
    which acts like it has the given size and number of layers.
"
Target(size::v2u, n_layers::Int = 1) = make_target(size, n_layers, TexOutput[ ], nothing, Texture[ ])

"
Creates a target with the given output size, format, and mip levels,
    using a single 2D color Texture and a Texture *or* TargetBuffer for depth/stencil.
"
function Target(size::v2u, color::TexFormat, depth_stencil::E_DepthStencilFormats
                ;
                ds_is_target_buffer::Bool = true,
                n_tex_mips::Integer = 0)
    color = Texture(color, size, @optional(n_tex_mips > 0, n_mips=n_tex_mips))
    depth = ds_is_target_buffer ?
                TargetBuffer(size, depth_stencil) :
                TargetOutput(1,
                             Texture(depth_stencil, size,
                                     @optional(n_tex_mips > 0, n_mips=n_tex_mips)),
                             nothing)

    return make_target(size, 1,
                       [ TargetOutput(1, color, nothing) ],
                       depth,
                       [ color, @optional(!ds_is_target_buffer, depth.tex) ])
end

"
Creates a Target which uses already-existing textures.
The Target is *not* responsible for cleaning up these textures.
"
Target(color::TargetOutput, depth::TargetOutput) = make_target(
    min(output_size(color), output_size(depth)),
    min(output_layer_count(color), output_layer_count(depth)),
    [ color ], depth, Texture[ ]
)

"
Creates a target with the given color output, and generates a corresponding depth/stencil buffer.
By default, the depth/stencil will be a TargetBuffer and not a Texture.
The Target is *not* responsible for cleaning up the color texture, only the new depth/stencil buffer.
"
function Target(color::TargetOutput, depth_stencil::E_DepthStencilFormats,
                ds_is_target_buffer::Bool = true)
    size::v2u = output_size(color)
    depth = ds_is_target_buffer ?
                TargetBuffer(size, depth_stencil) :
                TargetOutput(1,
                             Texture(depth_stencil, size, n_mips=color.tex.n_mips),
                             nothing)

    return make_target(size, 1,
                       [ TargetOutput(1, color, nothing) ],
                       depth,
                       [ @optional(!ds_is_target_buffer, depth.tex) ])
end

println("#TODO: The other Target constructors")


function make_target( size::v2u, n_layers::Int,
                      colors::Vector{TargetOutput},
                      depth_stencil::Union{Nothing, TargetOutput, TargetBuffer},
                      textures_to_manage::Vector{Texture}
                    )::Target
    @bp_check(all(size > 0), "Target's size can't be 0")
    @bp_check(exists(get_context()), "Trying to create a Target outside a valid context")

    # Check the depth attachment's format.
    depth_format::Optional{TexFormat} = nothing
    if depth_stencil isa TargetOutput
        depth_format = depth_stencil.format
    elseif depth_stencil isa TargetBuffer
        depth_format = depth_stencil.format
    end
    if exists(depth_format)
        @bp_check(
            is_depth_only(depth_format) || is_depth_and_stencil(depth_format),
            "Depth/stencil attachment for a Target must be",
              " a depth texture or depth-stencil hybrid texture, but it was ",
              depth_format
        )
    end

    # Create and configure the Framebuffer handle.
    handle = Ptr_Target(get_from_ogl(gl_type(Ptr_Target), glCreateFramebuffers, 1))
    if isempty(colors) && isnothing(depth_stencil)
        glNamedFramebufferParameteri(handle, GL_FRAMEBUFFER_DEFAULT_WIDTH, (GLint)size.x)
        glNamedFramebufferParameteri(handle, GL_FRAMEBUFFER_DEFAULT_HEIGHT, (GLint)size.y)
        glNamedFramebufferParameteri(handle, GL_FRAMEBUFFER_DEFAULT_LAYERS, (GLint)n_layers)
    end

    # Attach the color textures.
    for (i::Int, color::TargetOutput) in enumerate(colors)
        @bp_check(is_color(color.tex.format),
                  "Color attachment for a Target must be a color format, not ", color.tex.format)
        @bp_check(!(color.tex.format isa E_CompressedFormats),
                  "Color attachment can't be a compressed format: ", color.tex.format)
        attach_output(handle, GL_COLOR_ATTACHMENT0 + i, color)
    end

    # Attach the depth texture/buffer.
    if exists(depth_format)
        attachment::GLenum = if is_depth_only(depth_format)
                                 GL_DEPTH_ATTACHMENT
                             elseif is_depth_and_stencil(depth_format)
                                 GL_DEPTH_STENCIL_ATTACHMENT
                             else
                                 error("Unhandled case: ", depth_format)
                             end
        attach_output(handle, attachment, depth_stencil)
    end

    # Make sure all color attachments start out active.
    attachments = map(identity, 1:length(colors))
    attachments_buf = map(i -> GLenum(GL_COLOR_ATTACHMENT0 + i), attachments)
    glNamedFramebufferDrawBuffers(handle, length(colors), Ref(attachments_buf, 1))

    # Check the final state of the framebuffer object.
    status::GLenum = glCheckNamedFramebufferStatus(handle, GL_DRAW_FRAMEBUFFER)
    if status == GL_FRAMEBUFFER_COMPLETE
        # Things are good!
    elseif status == GL_FRAMEBUFFER_UNSUPPORTED
        throw(TargetUnsupportedException(map(c -> c.tex.format, colors),
                                         depth_format))
    elseif status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
        error("Some attachments are layered (i.e. full 3D textures or cubemaps),",
              " and some aren't.")
    else
        error("Unknown framebuffer error: ", GLENUM(status))
    end

    # Assemble the final instance.
    return Target(handle, size,
                  ReadOnlyVector(copy(colors)), depth_stencil,
                  ReadOnlyVector(attachments),
                  ReadOnlyVector(attachments_buf),
                  Set(textures_to_manage))
end
function attach_output(handle::Ptr_Target, slot::GLenum, output::TargetOutput)
    output_validate(output)

    # There are two variants of the OpenGL function to attach a texture to a framebuffer.
    # The only difference is that one of them has an extra argument at the end.
    main_args = (handle, slot, get_ogl_handle(output.tex), output.mip_level - 1)

    if output_is_layered(output) || !output_can_be_layered(output)
        glNamedFramebufferTexture(main_args...)
    else
        glNamedFramebufferTextureLayer(main_args..., GLint(Int(output.layer) - 1))
    end
end
function attach_output(handle::Ptr_Target, slot::GLenum, buffer::TargetBuffer)
    glNamedFramebufferRenderbuffer(handle, slot, GL_RENDERBUFFER, get_ogl_handle(buffer))
end


function Base.close(target::Target)
    # Clean up managed Textures.
    for tex in target.owned_textures
        close(tex)
    end
    empty!(target.owned_textures)

    # Clean up the depth/stencil TargetBuffer.
    if exists(target.depth_stencil)
        close(target.depth_stencil)
        setfield!(target, :depth_stencil, nothing)
    end

    # Finally, delete the framebuffer object itself.
    glDeleteFramebuffers(1, Ref(target.handle))
    setfield!(target, :handle, Ptr_Target())
end


#######################
#   Pubic interface   #
#######################


println("#TODO: Implement Activate()")
println("#TODO: Implement SetDrawBuffers()")
println("#TODO: Implement Clear functions")

#TODO: Implement copying: http://docs.gl/gl4/glBlitFramebuffer
#TODO: A special singleton Target representing the screen?
