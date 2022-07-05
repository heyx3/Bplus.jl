export Target, TargetUnsupportedException,
       target_activate, target_use_color_slots, target_clear


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

The full list of attached textures is fixed after creation,
    but the specific subset used in rendering can be configured with `target_use_color_slots()`.
"""
mutable struct Target <: Resource
    handle::Ptr_Target
    size::v2u

    attachment_colors::Vector{TargetOutput}
    attachment_depth_stencil::Union{Nothing, TargetOutput, TargetBuffer}

    # Maps each fragment shader output to a color attachment.
    #   e.x. the value [2, nothing, 4] means that shader outputs 1 and 3
    #    go to attachments 2 and 4, while all other outputs get discarded.
    color_render_slots::Vector{Optional{Int}}
    gl_buf_color_render_slots::Vector{GLenum} # Buffer for the slots data that goes to OpenGL

    # Textures which the Target will clean up automatically on close().
    # Some Target constructors will automatically create textures for you, for convenience,
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
                n_tex_mips::Integer = 0,
                ds_tex_sampling_mode::E_DepthStencilSources = DepthStencilSources.depth)
    color = Texture(color, size, @optional(n_tex_mips > 0, n_mips=n_tex_mips))
    depth = ds_is_target_buffer ?
                TargetBuffer(size, depth_stencil) :
                TargetOutput(1,
                             Texture(depth_stencil, size,
                                     @optional(is_depth_and_stencil(depth_stencil),
                                               depth_stencil_sampling = ds_tex_sampling_mode),
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
                ds_is_target_buffer::Bool = true,
                ds_tex_sampling_mode::E_DepthStencilSources = DepthStencilSources.depth)
    size::v2u = output_size(color)
    depth = ds_is_target_buffer ?
                TargetBuffer(size, depth_stencil) :
                TargetOutput(1,
                             Texture(depth_stencil, size,
                                     n_mips=color.tex.n_mips,
                                     @optional(is_depth_and_stencil(depth_stencil),
                                               depth_stencil_sampling = ds_tex_sampling_mode)),
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
        depth_format = depth_stencil.tex.format
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
        attach_output(handle, GLenum(GL_COLOR_ATTACHMENT0 + i-1), color)
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
    attachments = collect(Optional{Int}, 1:length(colors))
    attachments_buf = map(i -> GLenum(GL_COLOR_ATTACHMENT0 + i-1), attachments)
    glNamedFramebufferDrawBuffers(handle, length(colors), Ref(attachments_buf, 1))

    # Check the final state of the framebuffer object.
    status::GLenum = glCheckNamedFramebufferStatus(handle, GL_DRAW_FRAMEBUFFER)
    if status == GL_FRAMEBUFFER_COMPLETE
        # Everything is good!
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
                  copy(colors), depth_stencil,
                  attachments,
                  attachments_buf,
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
    if target.attachment_depth_stencil isa TargetBuffer
        close(target.attachment_depth_stencil)
    elseif target.attachment_depth_stencil isa TargetOutput
        # Already cleared up from the previous 'owned_textures' loop.
    elseif !isnothing(target.attachment_depth_stencil)
        error("Unhandled case: ", typeof(target.attachment_depth_stencil))
    end
    setfield!(target, :attachment_depth_stencil, nothing)

    # Finally, delete the framebuffer object itself.
    glDeleteFramebuffers(1, Ref(target.handle))
    setfield!(target, :handle, Ptr_Target())
end


#######################
#   Pubic interface   #
#######################

"
Sets a Target as the active one for rendering.
Pass `nothing` to render directly to the screen instead.
"
function target_activate(target::Optional{Target};
                         reset_viewport::Bool = true,
                         reset_scissor::Bool = true)
    context = get_context()
    @bp_gl_assert(exists(context), "Activating a Target (or the screen) outside of its context")

    if exists(target)
        glBindFramebuffer(GL_FRAMEBUFFER, get_ogl_handle(target))
        reset_viewport && set_viewport(context, one(v2i), v2i(target.size))
        reset_scissor && set_scissor(context, nothing)
    else
        glBindFramebuffer(GL_FRAMEBUFFER, Ptr_Target())
        reset_viewport && set_viewport(context, one(v2i), get_window_size(context))
        reset_scissor && set_scissor(context, nothing)
    end
end

"
Selects a subset of a Target's color attachments to use for rendering, by their (1-based) index.

For example, passing `[5, nothing, 1]` means that
    the fragment shader's first output goes to color attachment 5,
    the third output goes to color attachment 1,
    and all other outputs (i.e. 2 and 4+) are safely discarded.
"
function target_use_color_slots(target::Target, slots::AbstractVector{<:Optional{Integer}})
    is_valid_slot(i) = isnothing(i) ||
                       ((i > 0) && (i <= length(target.attachment_colors)))
    @bp_check(all(is_valid_slot, slots),
              "One or more color output slots reference a nonexistent attachment!",
              " This Target only has ", length(target.attachment_colors), ".",
              " Slots: ", slots)

    # Update the internal list.
    empty!(target.color_render_slots)
    append!(target.color_render_slots, slots)

    # Tell OpenGL about the new list.
    empty!(target.gl_buf_color_attachments)
    append!(target.gl_buf_color_render_slots, Iterators.map(slots) do slot
        if isnothing(slot)
            GL_NONE
        else
            GLenum(GL_COLOR_ATTACHMENT0 + slot - 1)
        end
    end)
    glNamedFramebufferDrawBuffers(get_ogl_handle(target),
                                  length(target.gl_buf_color_attachments),
                                  Ref(target.gl_buf_color_attachments, 1))
end

"
Clears a Target's color attachment.

If the color texture is `uint`, then you must clear to a `vRGBAu`.
If the color texture is `int`, then you must clear to a `vRGBAi`.
Otherwise, the color texture is `float` or `normalized_[u]int`, and you must clear to a `vRGBAf`.

The index is *not* the color attachment itself but the render slot,
    a.k.a. the fragment shader output.
    For example, if you previously called `target_use_color_slots(t, [ 3, nothing, 1])`,
    and you want to clear color attachment 1, pass the index `3`.
"
function target_clear(target::Target,
                      color::vRGBA{<:Union{Float32, Int32, UInt32}},
                      slot_index::Integer = 1)
    @bp_check(slot_index <= length(target.color_render_slots),
              "The Target only has ", length(target.color_render_slots),
                " active render slots, but you're trying to clear #", slot_index)
    @bp_check(exists(target.color_render_slots[slot_index]),
              "The Target currently has nothing bound to render slot ", slot_index, ",",
              " but you're trying to clear that slot")

    # Get the correct OpenGL function to call.
    # Also double-check that the texture format closely matches the clear data,
    #    as OpenGL requires.
    local gl_func::Function
    format::TexFormat = target.attachment_colors[slot_index].tex.format
    @bp_gl_assert(is_color(format), "Color attachment isn't a color format??? ", format)
    if color isa vRGBAf
        @bp_gl_assert(!is_integer(format),
                    "Color attachment is an integer format (", format, "),",
                        " you can't clear it with float data")
        gl_func = glClearNamedFramebufferfv
    else
        @bp_gl_assert(is_integer(format),
                      "Color attachment isn't an integer format (", format, "),",
                        " you can't clear it with integer data")
        if color isa vRGBAi
            @bp_gl_assert(is_signed(format),
                          "Color attachment is an unsigned format (", format, "),",
                              " you can't clear it with signed data")
            gl_func = glClearNamedFramebufferiv
        elseif color isa vRGBAu
            @bp_gl_assert(!is_signed(format),
                          "Color attachment is a signed format (", format, "),",
                              " you can't clear it with unsigned data")
            gl_func = glClearNamedFramebufferuiv
        else
            error("Unhandled case: ", color)
        end
    end

    gl_func(get_ogl_handle(target), GL_COLOR, slot_index - 1, Ref(color))
end
"
Clears a Target's depth attachment.
Note that this is *legal* on a hybrid depth-stencil buffer; the stencil values will be left alone.

The clear value will be casted to `Float32` and clamped to 0-1.

Be sure that depth writes are turned on in the Context!
Otherwise this would become a no-op, and an error may be thrown.
"
function target_clear(target::Target, depth::AbstractFloat)
    @bp_gl_assert(get_context().state.depth_write,
                  "Can't clear a Target's depth-buffer while depth writes are off")
    glClearNamedFramebufferfv(get_ogl_handle(target), GL_DEPTH, 0, Ref(@f32 depth))
end
"
Clears a Target's stencil attachment.
Note that this is *legal* on a hybrid depth-stencil buffer; the depth values will be left alone.

The clear value will be casted to `UInt8`.

Watch your Context's stencil write mask carefully!
If some bits are disabled from writing, then this clear operation wouldn't affect those bits.

By default, there is a debug check to prevent the stencil write mask from surprising you.
If the masking behavior is desired, you can disable the check by passing `false`.
"
function target_clear(target::Target, stencil::Unsigned,
                      prevent_partial_clears::Bool = true)
    if prevent_partial_clears
        @bp_gl_assert(get_context().state.stencil_write_mask == typemax(GLuint),
                      "The Context has had its `stencil_write_mask` set,",
                        " which can screw up the clearing of stencil buffers!",
                        " To disable this check, pass `false` to this `target_clear()` call.")
    end

    @bp_check(stencil <= typemax(UInt8),
              "Stencil buffer is assumed to have 8 bits, but your clear value (",
                stencil, ") is larger than the max 8-bit value ", typemax(UInt8))

    stencil_ref = Ref(reinterpret(GLint, UInt32(stencil)))
    glClearNamedFramebufferiv(get_ogl_handle(target), GL_STENCIL, 0, stencil_ref)
end
"
Clears a Target's hybrid depth/stencil attachment.
This is more efficient than clearing depth and stencil separately.

See above for important details about clearing depth and clearing stencil.
"
function target_clear(target::Target, depth_stencil::Depth32fStencil8u,
                      prevent_partial_clears::Bool = true)
    @bp_gl_assert(get_context().state.depth_write,
                  "Can't clear a Target's depth/stencli buffer while depth writes are off")
    if prevent_partial_clears
        @bp_gl_assert(get_context().state.stencil_write_mask == typemax(GLuint),
                      "The Context has had its `stencil_write_mask` set,",
                        " which can screw up the clearing of stencil buffers!",
                        " To disable this check, pass `false` to this `target_clear()` call.")
    end

    (depth::Float32, raw_stencil::UInt8) = split(depth_stencil)
    stencil = reinterpret(GLint, UInt32(raw_stencil))

    glClearNamedFramebufferfi(get_ogl_handle(target), GL_DEPTH_STENCIL, 0, depth, stencil)
end


#TODO: Implement copying: http://docs.gl/gl4/glBlitFramebuffer
#TODO: A special singleton Target representing the screen?
