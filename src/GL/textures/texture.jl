"
Some kind of organized color data which can be 'sampled'.
This includes regular images, as well as 1D and 3D textures and 'cubemaps'.
Note that mip levels are counted from 1, not from 0,
   to match Julia's convention.
"
mutable struct Texture <: AbstractResource
    handle::Ptr_Texture
    type::E_TexTypes

    format::TexFormat
    n_mips::Int

    # 3D textures have a full 3-dimensional size.
    # 2D/Cubemap textures have a constant Z size of 1.
    # 1D textures have a contant YZ size of 1.
    size::v3u

    # 3D textures have a full 3D sampler.
    # Other textures ignore the higher dimensions which don't apply to them.
    sampler::TexSampler{3}

    # If this is a hybrid depth/stencil texture,
    #    only one of those two components can be sampled at a time.
    depth_stencil_sampling::Optional{E_DepthStencilSources}

    # The mixing of this texture's channels when it's being sampled by a shader.
    swizzle::SwizzleRGBA

    # This texture's cached view handles.
    known_views::Dict{ViewParams, View}
end

export Texture

function Base.close(t::Texture)
    # Release the texture's Views.
    for view::View in values(t.known_views)
        service_ViewDebugging_remove_view(view.handle)
        view.handle = Ptr_View()
    end

    glDeleteTextures(1, Ref(gl_type(t.handle)))
    setfield!(t, :handle, Ptr_Texture())
end


############################
#       Constructors       #
############################

"Creates a 1D texture"
function Texture( format::TexFormat,
                  width::Union{Integer, Vec{1, <:Integer}}
                  ;

                  sampler::TexSampler{1} = TexSampler{1}(),
                  n_mips::Integer = get_n_mips(width isa Integer ? width : width.x),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )::Texture
    width = (width isa Integer) ? width : width.x
    return generate_texture(
        TexTypes.oneD, format, v3u(width, 1, 1),
        convert(TexSampler{3}, sampler),
        n_mips,
        depth_stencil_sampling,
        swizzling,
        nothing
    )
end
function Texture( format::TexFormat,
                  initial_data::PixelBufferD{1},
                  data_bgr_ordering::Bool = false
                  ;
                  sampler::TexSampler{1} = TexSampler{1}(),
                  n_mips::Integer = get_n_mips(length(initial_data)),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )::Texture
    return generate_texture(
        TexTypes.oneD, format, v3u(length(initial_data), 1, 1),
        convert(TexSampler{3}, sampler),
        n_mips,
        depth_stencil_sampling,
        swizzling,
        (initial_data, data_bgr_ordering)
    )
end

"Creates a 2D texture"
function Texture( format::TexFormat,
                  size::Vec2{<:Integer}
                  ;
                  sampler::TexSampler{2} = TexSampler{2}(),
                  n_mips::Integer = get_n_mips(size),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )::Texture
    return generate_texture(
        TexTypes.twoD, format, v3u(size..., 1),
        convert(TexSampler{3}, sampler),
        n_mips,
        depth_stencil_sampling,
        swizzling,
        nothing
    )
end
function Texture( format::TexFormat,
                  initial_data::PixelBufferD{2},
                  data_bgr_ordering::Bool = false
                  ;
                  sampler::TexSampler{2} = TexSampler{2}(),
                  n_mips::Integer = get_n_mips(Vec(size(initial_data))),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )::Texture
    return generate_texture(
        TexTypes.twoD, format,
        v3u(size(initial_data)..., 1),
        convert(TexSampler{3}, sampler),
        n_mips,
        depth_stencil_sampling,
        swizzling,
        (initial_data, data_bgr_ordering)
    )
end

"Creates a 3D texture"
function Texture( format::TexFormat,
                  size::Vec3{<:Integer}
                  ;
                  sampler::TexSampler{3} = TexSampler{3}(),
                  n_mips::Integer = get_n_mips(size),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )::Texture
    return generate_texture(
        TexTypes.threeD, format, convert(v3u, size),
        sampler,
        n_mips,
        depth_stencil_sampling,
        swizzling,
        nothing
    )
end
function Texture( format::TexFormat,
                  initial_data::PixelBufferD{3},
                  data_bgr_ordering::Bool = false
                  ;
                  sampler::TexSampler{3} = TexSampler{3}(),
                  n_mips::Integer = get_n_mips(Vec(size(initial_data))),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )::Texture
    return generate_texture(
        TexTypes.threeD, format,
        v3u(size(initial_data)),
        sampler,
        n_mips,
        depth_stencil_sampling,
        swizzling,
        (initial_data, data_bgr_ordering)
    )
end

"Creates a Cubemap texture"
function Texture_cube( format::TexFormat,
                       square_length::Integer,
                       initial_value = # Needed to get around an apparent driver bug
                                       #   where non-layered images of a cubemap don't accept writes
                                       #   until the pixels have been cleared.
                           if format isa E_CompressedFormats
                               # Nobody needs images of compressed textures anyway; let them go.
                               nothing
                           elseif is_color(format)
                               if is_signed(format)
                                   zero(vRGBAu)
                               elseif is_integer(format)
                                   zero(vRGBAi)
                               else
                                   zero(vRGBAf)
                               end
                           elseif is_depth_only(format)
                               zero(Float32)
                           elseif is_stencil_only(format)
                               zero(UInt8)
                           elseif is_depth_and_stencil(format)
                               zero(Depth32fStencil8u)
                           else
                               error("Unexpected format: ", format)
                           end
                       ;
                       sampler::TexSampler{1} = TexSampler{1}(),
                       n_mips::Integer = get_n_mips(square_length),
                       depth_stecil_sampling::Optional{E_DepthStencilSources} = nothing,
                       swizzling::SwizzleRGBA = SwizzleRGBA()
                     )::Texture
    tex = generate_texture(
        TexTypes.cube_map, format,
        v3u(square_length, square_length, 1),
        convert(TexSampler{3}, sampler),
        n_mips, depth_stecil_sampling, swizzling,
        nothing
    )
    if exists(initial_value)
        clear_tex_pixels(tex, initial_value)
    end
    return tex
end
function Texture_cube( format::TexFormat,
                       initial_faces_data::PixelBufferD{3},
                       data_bgr_ordering::Bool = false
                       ;
                       sampler::TexSampler{1} = TexSampler{1}(),
                       n_mips::Integer = get_n_mips(Vec(size(initial_faces_data[1:2]))),
                       depth_stecil_sampling::Optional{E_DepthStencilSources} = nothing,
                       swizzling::SwizzleRGBA = SwizzleRGBA()
                     )::Texture
    return generate_texture(
        TexTypes.cube_map, format,
        v3u(size(initial_faces_data)[1:2]..., 1),
        convert(TexSampler{3}, sampler),
        n_mips, depth_stecil_sampling, swizzling,
        (initial_faces_data, data_bgr_ordering)
    )
end
export Texture_cube

"
Creates a texture similar to the given one, but with a different format.
The new texture's data will not be initialized to anything.
"
Texture(template::Texture, format::TexFormat) = generate_texture(
    template.type, format, template.size, template.sampler, template.n_mips,
    (format isa E_DepthStencilFormats) ? template.depth_stencil_sampling : nothing,
    template.swizzle, nothing
)



################################
#    Private Implementation    #
################################

function generate_texture( type::E_TexTypes,
                           format::TexFormat,
                           size::v3u,
                           sampler::TexSampler{3},
                           n_mips::Integer,
                           depth_stencil_sampling::Optional{E_DepthStencilSources},
                           swizzling::SwizzleRGBA,
                           initial_data::Optional{Tuple{PixelBuffer, Bool}}
                         )::Texture
    # Verify the format/sampling settings are valid for this texture.
    @bp_check(exists(get_ogl_enum(format)),
              "Texture format isn't valid: ", sprint(show, format))
    if is_depth_and_stencil(format)
        @bp_check(exists(depth_stencil_sampling),
                  "Using a hybrid depth/stencil texture but didn't provide a depth_stencil_sampling mode")
    else
        @bp_check(isnothing(depth_stencil_sampling),
                  "Can't provide a depth_stencil_sampling mode to a texture that isn't hybrid depth/stencil")
    end
    if !(format isa E_DepthStencilFormats) || is_stencil_only(format)
        @bp_check(isnothing(sampler.depth_comparison_mode),
                    "Can't use a 'shadow sampler' for a non-depth texture")
    end
    @bp_check(is_supported(format, type),
              "Texture type ", type, " doesn't support the format ", sprint(show, format))

    # Make sure the global store of samplers has been initialized.
    if !service_SamplerProvider_exists()
        service_SamplerProvider_init()
    end

    # Construct the Texture instance.
    tex::Texture = Texture(Ptr_Texture(get_from_ogl(gl_type(Ptr_Texture),
                                                    glCreateTextures, GLenum(type), 1)),
                           type, format, n_mips, size,
                           sampler, depth_stencil_sampling, swizzling,
                           Dict{ViewParams, View}())

    # Configure the texture with OpenGL calls.
    set_tex_swizzling(tex, swizzling)
    if exists(depth_stencil_sampling)
        set_tex_depthstencil_source(tex, depth_stencil_sampling)
    end
    apply(sampler, tex.handle)

    # Set up the texture with non-reallocatable storage.
    if tex.type == TexTypes.oneD
        glTextureStorage1D(tex.handle, n_mips, get_ogl_enum(tex.format), size.x)
    elseif (tex.type == TexTypes.twoD) || (tex.type == TexTypes.cube_map)
        glTextureStorage2D(tex.handle, n_mips, get_ogl_enum(tex.format), size.xy...)
    elseif tex.type == TexTypes.threeD
        glTextureStorage3D(tex.handle, n_mips, get_ogl_enum(tex.format), size.xyz...)
    else
        error("Unhandled case: ", tex.type)
    end

    # Upload the initial data, if given.
    if exists(initial_data)
        set_tex_pixels(tex, initial_data[1];
                       color_bgr_ordering = initial_data[2],
                       recompute_mips=true)
    end

    return tex
end

function tex_array_dimensionality(t::E_TexTypes)::Int
    (t == TexTypes.oneD) && return 1
    (t == TexTypes.twoD) && return 2
    (t in (TexTypes.threeD, TexTypes.cube_map)) && return 3
    error("Unhandled: ", t)
end
tex_array_dimensionality(t::Texture) = tex_array_dimensionality(t.type)

"Helper to generate the default 'subset' of a texture"
default_tex_subset(tex::Texture) =
    if tex.type == TexTypes.oneD
        TexSubset{1}()
    elseif (tex.type == TexTypes.twoD) || (tex.type == TexTypes.cube_map)
        TexSubset{2}()
    elseif (tex.type == TexTypes.threeD)
        TexSubset{3}()
    else
        error("Unhandled case: ", tex.type)
    end

"Internal helper to get, set, or clear texture data"
function texture_data( tex::Texture,
                       component_type::GLenum,
                       components::GLenum,
                       subset::TexSubset,
                       value::Ref,
                       recompute_mips::Bool,
                       mode::@ano_enum(Set, Get, Clear)
                       ;
                       cube_face_range::IntervalU,
                       get_buf_pixel_byte_size::Int = -1,  # Only for Get ops
                       check_input_size::VecT{<:Integer} = Vec(-1) # Only for Get/Set ops
                     )
    # Check that the subset is the right number of dimensions.
    if tex.type == TexTypes.oneD
        @bp_check(subset isa TexSubset{1},
                  "Expected a 1D subset for 1D textures; got ", typeof(subset))
    elseif tex.type in (TexTypes.twoD, TexTypes.cube_map)
        @bp_check(subset isa TexSubset{2},
                  "Expected a 2D subset for 2D textures and cubemaps; got ", typeof(subset))
    elseif tex.type == TexTypes.threeD
        @bp_check(subset isa TexSubset{3},
                  "Expected a 3D subset for 3D textures; got ", typeof(subset))
    else
        error("Unimplemented: ", typeof(subset))
    end

    range::Box3D = get_subset_range(change_dimensions(subset, 3)::TexSubset{3},
                                    get_mip_size(tex.size, subset.mip)::v3u)
    if (tex.type == TexTypes.cube_map)
        @bp_check(min_inclusive(cube_face_range) >= 1,
                  "Cubemap face range should be within 1:6, but starts at ", min_inclusive(cube_face_range))
        @bp_check(max_inclusive(cube_face_range) <= 6,
                  "Cubemap face range should be within 1:6, but ends at ", max_inclusive(cube_face_range), " (inclusive)")
        @bp_check(size(cube_face_range) > 0,
                  "Cubemap face range is empty")
        range = typeof(range)((min=v3u(min_inclusive(range).xy...,
                                       min_inclusive(cube_face_range)),
                               size=v3u(size(range).xy...,
                                        size(cube_face_range))))
    end

    # Perform the requested operation.
    if mode isa Val{:Set}
        @bp_check(length(check_input_size) == tex_array_dimensionality(tex.type),
                  "Expected a ", tex_array_dimensionality(tex.type), "D array for ",
                    "the ", tex.type, " texture; got ", length(check_input_size))
        if tex.type == TexTypes.oneD
            @bp_check(size(range).x == check_input_size.x,
                      "Trying to set ", size(range).x, " pixels",
                      " using an array with ", check_input_size.x, " elements")
            glTextureSubImage1D(tex.handle, subset.mip - 1,
                                min_inclusive(range).x - 1,
                                size(range).x,
                                components, component_type,
                                value)
        elseif tex.type == TexTypes.twoD
            @bp_check(all(size(range).xy == check_input_size.xy),
                      "Trying to set ", size(range).xy, " pixels",
                      " using an array with ", check_input_size.xy, " elements")
            glTextureSubImage2D(tex.handle, subset.mip - 1,
                                (min_inclusive(range).xy - 1)...,
                                size(range).xy...,
                                components, component_type,
                                value)
        elseif (tex.type == TexTypes.threeD) || (tex.type == TexTypes.cube_map)
            full_check_size = (tex.type == TexTypes.cube_map) ?
                                  vappend(check_input_size, size(cube_face_range)) :
                                  check_input_size
            @bp_check(all(size(range).xyz == full_check_size.xyz),
                      "Trying to set ", size(range).xyz, " pixels",
                      " using an array with ", full_check_size.xyz, " elements")
            glTextureSubImage3D(tex.handle, subset.mip - 1,
                                (min_inclusive(range) - 1)...,
                                size(range)...,
                                components, component_type,
                                value)
        else
            error("Unhandled case: ", tex.type)
        end
    elseif mode isa Val{:Get}
        @bp_check(length(check_input_size) == tex_array_dimensionality(tex),
                  "Expected a ", tex_array_dimensionality(tex), "D array for ",
                    "the ", tex.type, " texture; got ", length(check_input_size))
        @bp_gl_assert(get_buf_pixel_byte_size > 0,
                      "Internal field 'get_buf_pixel_byte_size' not passed for getting texture data")
        @bp_gl_assert(!recompute_mips,
                      "Asked to recompute mips after getting a texture's pixels, which is pointless")
# println(
#     "Getting a ", tex.type, " texture's pixels.\n",
#     "\tReading pixel range ", range, " into a ", check_input_size, " array\n",
#     "\tComponents: ", ModernGLbp.GLENUM(components), "\n",
#     "\tComponentType: ", ModernGLbp.GLENUM(component_type), "\n",
#     "\tBuf pixel byte size: ", get_buf_pixel_byte_size, "\n"
# )
        if get_component_count(typeof(check_input_size)) == 1
            @bp_check(size(range).x <= check_input_size.x,
                      "Trying to read ", size(range).x, " pixels",
                        " into an array with ", check_input_size.x, " elements")
        elseif get_component_count(typeof(check_input_size)) == 2
            local full_check_size, full_range
            if tex.type == TexTypes.cube_map
                full_check_size = vappend(check_input_size, size(cube_face_range))
                full_range = vappend(size(range).xy, size(cube_face_range))
            else
                full_check_size = check_input_size
                full_range = size(range).xy
            end
            @bp_check(all(full_range <= full_check_size),
                    "Trying to read ", full_range, " pixels",
                        " into an array with ", full_check_size, " elements")
        elseif get_component_count(typeof(check_input_size)) == 3
            @bp_check(all(size(range) <= check_input_size),
                    "Trying to read ", size(range), " pixels",
                        " into an array with ", check_input_size, " elements")
        else
            error("Unknown case: ", typeof(check_input_size))
        end
        glGetTextureSubImage(tex.handle, subset.mip - 1,
                             (min_inclusive(range) - 1)..., size(range)...,
                             components, component_type,
                             prod(size(range)) * get_buf_pixel_byte_size,
                             value)
    elseif mode isa Val{:Clear}
# println(
#     "\n-------------------\nClearing tex #", tex.handle, " to ", value,
#     "\n\tComponents: ", ModernGLbp.GLENUM(components),
#     "\\ntComponentType: ", ModernGLbp.GLENUM(component_type),
#     "\n\t",
#     "\n----------------\n"
# )
        glClearTexSubImage(tex.handle, subset.mip - 1,
                           (min_inclusive(range) - 1)..., size(range)...,
                           components, component_type,
                           value)
    end

    if recompute_mips
        glGenerateTextureMipmap(tex.handle)
    end
end


##########################
#    Public Interface    #
##########################

get_ogl_handle(t::Texture) = t.handle

"
Gets a view of this texture, using the given sampler
    (or the texture's built-in sampler settings).
"
function get_view(t::Texture, sampler::Optional{TexSampler} = nothing)::View
    return get!(() -> View(t, sampler),
                t.known_views, sampler)
end
"
Gets a view of this texture's pixels without sampling,
    which allows for other uses like writing to the pixels.
"
function get_view(t::Texture, view::SimpleViewParams)::View
    return get!(() -> View(t, t.format, view),
                t.known_views, view)
end

# Overload some view functions to work with a texture's default view.
view_activate(tex::Texture) = view_activate(get_view(tex))
view_deactivate(tex::Texture) = view_deactivate(get_view(tex))


"Changes how a texture's pixels are mixed when it's sampled on the GPU"
function set_tex_swizzling(t::Texture, new::SwizzleRGBA)
    new_as_ints = reinterpret(Vec4{GLint}, new)
    glTextureParameteriv(t.handle, GL_TEXTURE_SWIZZLE_RGBA, Ref(new_as_ints.data))
    setfield!(t, :swizzle, new)
end
"Changes the data that can be sampled from this texture, assuming it's a depth/stencil hybrid"
function set_tex_depthstencil_source(t::Texture, source::E_DepthStencilSources)
    @bp_check(is_depth_and_stencil(t.format),
              "Can only set depth/stencil source for a depth/stencil hybrid texture")

    if source != t.depth_stencil_sampling
        glTextureParameteri(t.handle, GL_DEPTH_STENCIL_TEXTURE_MODE, GLint(source))
        setfield!(t, :depth_stencil_sampling, source)
    end
end

"Gets the byte size of a specific texture mip level"
function get_mip_byte_size(t::Texture, level::Integer)
    if t.type == TexTypes.cube
        return get_byte_size(t.format, t.size.xy * Vec(6, 1))
    else
        return get_byte_size(t.format, t.size)
    end
end
"Gets the total byte-size of this texture's pixels, including all mips"
function get_gpu_byte_size(t::Texture)
    return sum(get_mip_byte_size(t, mip) for mip in 1:t.n_mips)
end


"Clears a texture to a given value, without knowing yet what kind of texture it is"
function clear_tex_pixels(t::Texture, values...; kw...)
    if is_color(t.format)
        clear_tex_color(t, values...; kw...)
    elseif is_depth_only(t.format)
        clear_tex_depth(t, values...; kw...)
    elseif is_stencil_only(t.format)
        clear_tex_stencil(t, values...; kw...)
    elseif is_depth_and_stencil(t.format)
        clear_tex_depthstencil(t, values...; kw...)
    else
        error("Unexpected format: ", t.format)
    end
end
"Clears a color texture to a given value"
function clear_tex_color( t::Texture,
                          color::PixelIOValue
                          ;
                          subset::TexSubset = default_tex_subset(t),
                          bgr_ordering::Bool = false,
                          cube_face_range::IntervalU = IntervalU(min=1, max=6),
                          recompute_mips::Bool = true
                        )
    T = get_component_type(typeof(color))
    N = get_component_count(typeof(color))

    @bp_check(!(t.format isa E_CompressedFormats),
              "Can't clear compressed textures")
    @bp_check(is_color(t.format), "Can't clear a non-color texture with a color")
    @bp_check(!is_integer(t.format) || T<:Integer,
              "Can't clear an integer texture with a non-integer value")
    @bp_check(!bgr_ordering || N >= 3,
              "Supplied the 'bgr_ordering' parameter but the data only has 1 or 2 components")

    texture_data(t,
                 GLenum(get_pixel_io_type(T)),
                 get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering), is_integer(t.format)),
                 subset, contiguous_ref(color, T), recompute_mips, Val(:Clear)
                 ;
                 cube_face_range = cube_face_range)
end
"Clears a depth texture to a given value"
function clear_tex_depth( t::Texture,
                          depth::T
                          ;
                          subset::TexSubset = default_tex_subset(t),
                          cube_face_range::IntervalU = IntervalU(min=1, max=6),
                          recompute_mips::Bool = true
                        ) where {T<:PixelIOComponent}
    @bp_check(is_depth_only(t.format),
              "Can't clear depth in a texture of format ", t.format)
    texture_data(t,
                 GLenum(get_pixel_io_type(T)),
                 GL_DEPTH_COMPONENT,
                 subset, Ref(depth), recompute_mips, Val(:Clear)
                 ;
                 cube_face_range = cube_face_range)
end
"Clears a stencil texture to a given value"
function clear_tex_stencil( t::Texture,
                            stencil::UInt8
                            ;
                            cube_face_range::IntervalU = IntervalU(min=1, max=6),
                            subset::TexSubset = default_tex_subset(t)
                          )
    @bp_check(is_stencil_only(t.format),
              "Can't clear stencil in a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(UInt8)), GL_STENCIL_INDEX,
                 subset, Ref(stencil), false, Val(:Clear)
                 ;
                 cube_face_range = cube_face_range)
end
"Clears a depth/stencil hybrid texture to a given value with 24 depth bits and 8 stencil bits"
function clear_tex_depthstencil( t::Texture,
                                 depth::Float32,
                                 stencil::UInt8
                                 ;
                                 subset::TexSubset = default_tex_subset(t),
                                 cube_face_range::IntervalU = IntervalU(min=1, max=6),
                                 recompute_mips::Bool = true
                               )
    if t.format == DepthStencilFormats.depth24u_stencil8
        return clear_tex_depthstencil(t, Depth24uStencil8u(depth, stencil),
                                      subset=subset, recompute_mips=recompute_mips,
                                      cube_face_range=cube_face_range)
    elseif t.format == DepthStencilFormats.depth32f_stencil8
        return clear_tex_depthstencil(t, Depth32fStencil8u(depth, stencil),
                                      subset=subset, recompute_mips=recompute_mips,
                                      cube_face_range=cube_face_range)
    else
        error("Texture doesn't appear to be a depth/stencil hybrid format: ", t.format)
    end
end
function clear_tex_depthstencil( t::Texture,
                                 value::Depth24uStencil8u
                                 ;
                                 subset::TexSubset = default_tex_subset(t),
                                 cube_face_range::IntervalU = IntervalU(min=1, max=6),
                                 recompute_mips::Bool = true
                               )
    @bp_check(t.format == DepthStencilFormats.depth24u_stencil8,
              "Trying to clear a texture with a hybrid 24u-depth/8-stencil value, on a texture of a different format: ", t.format)
    texture_data(t, GL_UNSIGNED_INT_24_8, GL_DEPTH_STENCIL,
                 subset, Ref(value), recompute_mips, Val(:Clear)
                 ;
                 cube_face_range = cube_face_range)
end
function clear_tex_depthstencil( t::Texture,
                                 value::Depth32fStencil8u
                                 ;
                                 subset::TexSubset = default_tex_subset(t),
                                 cube_face_range::IntervalU = IntervalU(min=1, max=6),
                                 recompute_mips::Bool = true
                               )
    @bp_check(t.format == DepthStencilFormats.depth32f_stencil8,
              "Trying to clear a texture with a hybrid 32f-depth/8-stencil value, on a texture of a different format: ", t.format)
    texture_data(t, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH_STENCIL,
                 subset, Ref(value), recompute_mips, Val(:Clear)
                 ;
                 cube_face_range = cube_face_range)
end


"
Sets a texture's pixels, figuring out dynamically whether they're color, depth, etc.
For specific overloads based on texture format, see `set_tex_color()`, `set_tex_depth()`, etc respectively.

The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_pixels( t::Texture,
                         pixels::PixelBuffer
                         ;
                         subset::TexSubset = default_tex_subset(t),
                         color_bgr_ordering::Bool = false,
                         cube_face_range::IntervalU = IntervalU(min=1, max=6),
                         recompute_mips::Bool = true
                       )
    if is_color(t.format)
        set_tex_color(t, pixels; subset=subset,
                      bgr_ordering=color_bgr_ordering,
                      recompute_mips=recompute_mips,
                      cube_face_range=cube_face_range)
    else
        @bp_check(!color_bgr_ordering,
                  "Supplied the 'color_bgr_ordering' parameter for a non-color texture")
        if is_depth_only(t.format)
            set_tex_depth(t, pixels; subset=subset,
                                     recompute_mips=recompute_mips,
                                     cube_face_range=cube_face_range)
        elseif is_stencil_only(t.format)
            set_tex_stencil(t, pixels; subset=subset,
                                       recompute_mips=recompute_mips,
                                       cube_face_range=cube_face_range)
        elseif is_depth_and_stencil(t.format)
            set_tex_depthstencil(t, pixels; subset=subset,
                                            recompute_mips=recompute_mips,
                                            cube_face_range=cube_face_range)
        else
            error("Unhandled case: ", t.format)
        end
    end
end
"
Sets the data for a color texture.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_color( t::Texture,
                        pixels::TBuf,
                        ;
                        subset::TexSubset = default_tex_subset(t),
                        bgr_ordering::Bool = false,
                        cube_face_range::IntervalU = IntervalU(min=1, max=6),
                        recompute_mips::Bool = true
                      ) where {TBuf <: PixelBuffer}
    T = get_component_type(TBuf)
    N = get_component_count(TBuf)

    @bp_check(!(t.format isa E_CompressedFormats),
              "Can't set compressed texture data as if it's normal color data! ",
                "(technically you can, but driver implementations for the compression are usually quite bad)")
    @bp_check(is_color(t.format), "Can't set a non-color texture with a color")
    @bp_check(!is_integer(t.format) || T<:Integer,
              "Can't set an integer texture with a non-integer value")
    @bp_check(!bgr_ordering || N >= 3,
              "Supplied the 'bgr_ordering' parameter but the data only has 1 or 2 components")

    texture_data(t,
                 GLenum(get_pixel_io_type(T)),
                 get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering),
                              is_integer(t.format)),
                 subset, Ref(pixels, 1), recompute_mips,
                 Val(:Set)
                 ;
                 cube_face_range = cube_face_range,
                 check_input_size = vsize(pixels))
end
"
Sets the data for a depth texture.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_depth( t::Texture,
                        pixels::PixelBuffer{T}
                        ;
                        input_dimensions::v3u = convert(v3u, vsize(pixels)),
                        subset::TexSubset = default_tex_subset(t),
                        cube_face_range::IntervalU = IntervalU(min=1, max=6),
                        recompute_mips::Bool = true
                      ) where {T<:Union{Number, Vec{1, <:Number}}}
    @bp_check(is_depth_only(t.format),
              "Can't set depth values in a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(T)), GL_DEPTH_COMPONENT,
                 subset, Ref(pixels, 1), recompute_mips,
                 Val(:Set)
                 ;
                 cube_face_range = cube_face_range,
                 check_input_size = vsize(pixels))
end
"
Sets the data for a stencil texture.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_stencil( t::Texture,
                          pixels::@unionspec(PixelBuffer{_}, UInt8, Vec{1, UInt8})
                          ;
                          subset::TexSubset = default_tex_subset(t),
                          cube_face_range::IntervalU = IntervalU(min=1, max=6),
                          recompute_mips::Bool = true
                        )
    @bp_check(is_stencil_only(t.format),
              "Can't set stencil values in a texture of format ", t.format)
    texture_data(t,
                 GLenum(get_pixel_io_type(UInt8)),
                 GL_STENCIL_INDEX,
                 subset, Ref(pixels, 1), recompute_mips,
                 Val(:Set)
                 ;
                 cube_face_range = cube_face_range,
                 check_input_size = vsize(pixels))
end
"
Sets the data for a depth/stencil hybrid texture.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_depthstencil( t::Texture,
                               pixels::PixelBuffer{T}
                               ;
                               subset::TexSubset = default_tex_subset(t),
                               cube_face_range::IntervalU = IntervalU(min=1, max=6),
                               recompute_mips::Bool = true
                             ) where {T <: Union{Depth24uStencil8u, Depth32fStencil8u}}
    local component_type::GLenum
    if T == Depth24uStencil8u
        @bp_check(t.format == DepthStencilFormats.depth24u_stencil8,
                  "Trying to give hybrid depth24u/stencil8 data to a texture of format ", t.format)
        component_type = GL_UNSIGNED_INT_24_8
    elseif T == Depth32fStencil8u
        @bp_check(t.format == DepthStencilFormats.depth32f_stencil8,
                  "Trying to give hybrid depth32f/stencil8 data to a texture of format ", t.format)
        component_type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV
    else
        error("Unhandled case: ", T)
    end

    texture_data(t, component_type, GL_STENCIL_INDEX,
                 subset, Ref(pixels, 1), recompute_mips,
                 Val(:Set)
                 ;
                 cube_face_range = cube_face_range,
                 check_input_size = vsize(pixels))
end

"
Gets a texture's pixels, figuring out dynamically whether they're color, depth, etc.
For specific overloads based on texture format, see `get_tex_color()`, `get_tex_depth()`, etc respectively.

The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_pixels( t::Texture,
                         out_pixels::PixelBuffer
                         ;
                         subset::TexSubset = default_tex_subset(t),
                         color_bgr_ordering::Bool = false,
                         cube_face_range::IntervalU = IntervalU(min=1, max=6),
                         recompute_mips::Bool = true
                       )
    if is_color(t.format)
        get_tex_color(t, out_pixels; subset=subset,
                                     bgr_ordering=color_bgr_ordering,
                                     recompute_mips=recompute_mips,
                                     cube_face_range=cube_face_range)
    else
        @bp_check(!color_bgr_ordering,
                  "Supplied the 'color_bgr_ordering' parameter for a non-color texture")
        if is_depth_only(t.format)
            get_tex_depth(t, out_pixels; subset=subset,
                                         recompute_mips=recompute_mips,
                                         cube_face_range=cube_face_range)
        elseif is_stencil_only(t.format)
            get_tex_stencil(t, out_pixels; subset=subset,
                                           recompute_mips=recompute_mips,
                                           cube_face_range=cube_face_range)
        elseif is_depth_and_stencil(t.format)
            get_tex_depthstencil(t, out_pixels; subset=subset,
                                                recompute_mips=recompute_mips,
                                                cube_face_range=cube_face_range)
        else
            error("Unhandled case: ", t.format)
        end
    end
end
"
Gets the data for a color texture and writes them into the given array.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_color( t::Texture,
                        out_pixels::TBuf
                        ;
                        subset::TexSubset = default_tex_subset(t),
                        cube_face_range::IntervalU = IntervalU(min=1, max=6),
                        bgr_ordering::Bool = false,
                      ) where {TBuf <: PixelBuffer}
    T = get_component_type(TBuf)
    N = get_component_count(TBuf)

    @bp_check(!(t.format isa E_CompressedFormats),
              "Can't get compressed texture data as if it's normal color data!")
    @bp_check(is_color(t.format),
              "Can't get color data from a texture of format ", t.format)
    @bp_check(!is_integer(t.format) || T<:Integer,
              "Can't set an integer texture with a non-integer value")
    @bp_check(!bgr_ordering || N >= 3,
              "Supplied the 'bgr_ordering' parameter but the data only has 1 or 2 components")

    texture_data(t,
                 GLenum(get_pixel_io_type(T)),
                 get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering),
                              is_integer(t.format)),
                 subset, Ref(out_pixels, 1), false,
                 Val(:Get)
                 ;
                 cube_face_range = cube_face_range,
                 get_buf_pixel_byte_size = N * sizeof(T),
                 check_input_size = vsize(out_pixels))
end
"
Gets the data for a depth texture and writes them into the given array.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_depth( t::Texture,
                        out_pixels::PixelBuffer{T}
                        ;
                        subset::TexSubset = default_tex_subset(t),
                        cube_face_range::IntervalU = IntervalU(min=1, max=6),
                      ) where {T <: Union{Number, Vec{1, <:Number}}}
    @bp_check(is_depth_only(t.format), "Can't get depth data from a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(T)), GL_DEPTH_COMPONENT,
                 subset, Ref(out_pixels, 1), false,
                 Val(:Get)
                 ;
                 cube_face_range = cube_face_range,
                 get_buf_pixel_byte_size = sizeof(T),
                 check_input_size = vsize(out_pixels))
end
"
Gets the data for a stencil texture and writes them into the given array.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_stencil( t::Texture,
                          out_pixels::PixelBuffer{<:Union{UInt8, Vec{1, UInt8}}}
                          ;
                          subset::TexSubset = default_tex_subset(t),
                          cube_face_range::IntervalU = IntervalU(min=1, max=6),
                        )
    @bp_check(is_stencil_only(t.format), "Can't get stencil data from a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(UInt8)), GL_STENCIL_INDEX,
                 subset, Ref(out_pixels, 1), false,
                 Val(:Get)
                 ;
                 cube_face_range = cube_face_range,
                 get_buf_pixel_byte_size = sizeof(UInt8),
                 check_input_size = vsize(out_pixels))
end
"
Gets the data for a depth-stencil hybrid texture and writes them into the given array.
The dimensionality of the input must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_depthstencil( t::Texture,
                               out_pixels::PixelBuffer{T}
                               ;
                               subset::TexSubset = default_tex_subset(t),
                               cube_face_range::IntervalU = IntervalU(min=1, max=6),
                             ) where {T <: Union{Depth24uStencil8u, Depth32fStencil8u}}
    local component_type::GLenum
    if T == Depth24uStencil8u
        @bp_check(t.format == DepthStencilFormats.depth24u_stencil8,
                  "Trying to get hybrid depth24u/stencil8 data from a texture of format ", t.format)
        component_type = GL_UNSIGNED_INT_24_8
    elseif T == Depth32fStencil8u
        @bp_check(t.format == DepthStencilFormats.depth32f_stencil8,
                  "Trying to get hybrid depth32f/stencil8 data from a texture of format ", t.format)
        component_type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV
    else
        error("Unhandled case: ", T)
    end

    texture_data(t, component_type, GL_DEPTH_STENCIL,
                 subset, Ref(out_pixels, 1), false,
                 Val(:Get)
                 ;
                 cube_face_range = cube_face_range,
                 get_buf_pixel_byte_size = sizeof(T),
                 check_input_size = vsize(out_pixels))
end

export get_view,
       set_tex_swizzling, set_tex_depthstencil_source,
       get_mip_byte_size, get_gpu_byte_size,
       clear_tex_pixels, clear_tex_color, clear_tex_depth, clear_tex_stencil, clear_tex_depthstencil,
       set_tex_pixels, set_tex_color, set_tex_depth, set_tex_stencil,
       get_tex_pixels, get_tex_color, get_tex_depth, get_tex_stencil, get_tex_depthstencil