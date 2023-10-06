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
                       depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                       swizzling::SwizzleRGBA = SwizzleRGBA()
                     )::Texture
    tex = generate_texture(
        TexTypes.cube_map, format,
        v3u(square_length, square_length, 1),
        convert(TexSampler{3}, sampler),
        n_mips, depth_stencil_sampling, swizzling,
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
                       depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                       swizzling::SwizzleRGBA = SwizzleRGBA()
                     )::Texture
    return generate_texture(
        TexTypes.cube_map, format,
        v3u(size(initial_faces_data)[1:2]..., 1),
        convert(TexSampler{3}, sampler),
        n_mips, depth_stencil_sampling, swizzling,
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

"
Processes data representing a subset of a texture,
  checking it for errors and returning the final pixel range and mip-level.

For simplicity, the returned range is always 3D.
Cubemap textures use Z to represent the cube faces.
"
function process_tex_subset(tex::Texture, subset::TexSubset)::Tuple{Box3Du, UInt}
    # Check the dimensionality of the subset.
    if tex.type == TexTypes.oneD
        @bp_check(subset isa TexSubset{1},
                  "Expected a 1D subset for 1D textures; got ", typeof(subset))
    elseif tex.type == TexTypes.twoD
        @bp_check(subset isa TexSubset{2},
                  "Expected a 2D subset for 2D textures and cubemaps; got ", typeof(subset))
    elseif tex.type in (TexTypes.threeD, TexTypes.cube_map)
        @bp_check(subset isa TexSubset{3},
                  "Expected a 3D subset for Cubemap and 3D textures; got ", typeof(subset))
    else
        error("Unimplemented: ", tex.type)
    end

    # Check the mip index.
    legal_mips = IntervalU(min=1, max=tex.n_mips)
    @bp_check(is_touching(legal_mips, subset.mip),
              "Texture's mip range is ", legal_mips, " but you requested mip ", subset.mip)

    # Get the 3D size of the texture.
    # 1D tex has Y=Z=1, 2D tex has Z=1, and Cubemap tex has Z=6.
    tex_size::v3u = get_mip_size(tex.size, subset.mip)
    if tex.type == TexTypes.oneD
        @set! tex_size.y = 1
        @set! tex_size.z = 1
    elseif tex.type == TexTypes.twoD
        @set! tex_size.z = 1
    elseif tex.type == TexTypes.cube_map
        @set! tex_size.z = 6
    end

    # Make sure the subset is fully inside the texture.
    tex_pixel_range = Box3Du(min=one(v3u), size=tex_size)
    subset_pixel_range::Box3Du = reshape(get_subset_range(subset, tex_size),
                                         3,
                                         new_dims_size=UInt32(1),
                                         new_dims_min=UInt32(1))
    @bp_check(contains(tex_pixel_range, subset_pixel_range),
              "Texture at mip level ", subset.mip, " is size ", tex_size,
                ", and your subset is out of those bounds: ", subset_pixel_range)

    return subset_pixel_range, subset.mip
end

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

"Helper to generate the default 'subset' of a texture"
default_tex_subset(tex::Texture)::@unionspec(TexSubset{_}, 1, 2, 3) =
    if tex.type == TexTypes.oneD
        TexSubset{1}()
    elseif tex.type == TexTypes.twoD
        TexSubset{2}()
    elseif tex.type in (TexTypes.cube_map, TexTypes.threeD)
        TexSubset{3}()
    else
        error("Unhandled case: ", tex.type)
    end
#

"Internal helper to get, set, or clear texture data"
function texture_op_impl( tex::Texture,
                          component_type::GLenum,
                          components::GLenum,
                          subset::TexSubset,
                          value::Ref,
                          recompute_mips::Bool,
                          mode::@ano_enum(Set, Get, Clear),
                          # Get/Set ops would like to ensure that the array passed in
                          #    matches the size of the texture subset.
                          known_subset_data_size::Optional{VecT{<:Integer}} = nothing
                          ;
                          get_buf_pixel_byte_size::Int = -1  # Only for Get ops
                        )
    (subset_pixels_3D::Box3Du, mip::UInt) = process_tex_subset(tex, subset)

    # If requested, check the size of the actual array data
    #    against the size we are telling OpenGL.
    if exists(known_subset_data_size)
        @bp_check(prod(known_subset_data_size) >= prod(size(subset_pixels_3D)),
                  "Your pixel data array is size ", known_subset_data_size,
                    ", but the texture (or subset/mip) you are working with is larger: ", subset_pixels_3D)
    end

    # Perform the requested operation.
    if mode isa Val{:Set}
        @bp_gl_assert(get_buf_pixel_byte_size == -1,
                      "Internal field 'get_buf_pixel_byte_size' shouldn't be passed for a Set op")
        if tex.type == TexTypes.oneD
            glTextureSubImage1D(tex.handle, mip - 1,
                                min_inclusive(subset_pixels_3D).x - 1,
                                size(subset_pixels_3D).x,
                                components, component_type,
                                value)
        elseif tex.type == TexTypes.twoD
            glTextureSubImage2D(tex.handle, mip - 1,
                                (min_inclusive(subset_pixels_3D).xy - 1)...,
                                size(subset_pixels_3D).xy...,
                                components, component_type,
                                value)
        elseif (tex.type == TexTypes.threeD) || (tex.type == TexTypes.cube_map)
            glTextureSubImage3D(tex.handle, mip - 1,
                                (min_inclusive(subset_pixels_3D) - 1)...,
                                size(subset_pixels_3D)...,
                                components, component_type,
                                value)
        else
            error("Unhandled case: ", tex.type)
        end
    elseif mode isa Val{:Get}
        @bp_gl_assert(get_buf_pixel_byte_size > 0,
                      "Internal field 'get_buf_pixel_byte_size' not passed for a Get operation like it should have been")
        @bp_gl_assert(!recompute_mips,
                      "Asked to recompute mips after getting a texture's pixels, which is pointless")
        glGetTextureSubImage(tex.handle, mip - 1,
                             (min_inclusive(subset_pixels_3D) - 1)...,
                             size(subset_pixels_3D)...,
                             components, component_type,
                             prod(size(subset_pixels_3D)) * get_buf_pixel_byte_size,
                             value)
    elseif mode isa Val{:Clear}
        @bp_gl_assert(get_buf_pixel_byte_size == -1,
                      "Internal field 'get_buf_pixel_byte_size' shouldn't be passed for a Clear op")
        glClearTexSubImage(tex.handle, mip - 1,
                           (min_inclusive(subset_pixels_3D) - 1)...,
                           size(subset_pixels_3D)...,
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


"
Clears a texture to a given value, without knowing yet what kind of texture it is.

The dimensionality of the `subset` parameter must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
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
"Clears a color texture to a given value.

The dimensionality of the `subset` parameter must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function clear_tex_color( t::Texture,
                          color::PixelIOValue,
                          subset::TexSubset = default_tex_subset(t)
                          ;
                          bgr_ordering::Bool = false,
                          single_component::E_PixelIOChannels = PixelIOChannels.red,
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

    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(T)),
        get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering, single_component),
                    is_integer(t.format)),
        subset,
        contiguous_ref(color, T),
        recompute_mips, Val(:Clear)
    )
end
"Clears a depth texture to a given value.

The dimensionality of the `subset` parameter must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function clear_tex_depth( t::Texture,
                          depth::T,
                          subset::TexSubset = default_tex_subset(t)
                          ;
                          recompute_mips::Bool = true
                        ) where {T<:PixelIOComponent}
    @bp_check(is_depth_only(t.format),
              "Can't clear depth in a texture of format ", t.format)
    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(T)),
        GL_DEPTH_COMPONENT,
        subset,
        Ref(depth),
        recompute_mips, Val(:Clear)
    )
end
"Clears a stencil texture to a given value.

The dimensionality of the `subset` parameter must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function clear_tex_stencil( t::Texture,
                            stencil::UInt8,
                            subset::TexSubset = default_tex_subset(t)
                          )
    @bp_check(is_stencil_only(t.format),
              "Can't clear stencil in a texture of format ", t.format)
    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(UInt8)),
        GL_STENCIL_INDEX,
        subset,
        Ref(stencil),
        false,
        Val(:Clear)
    )
end
"
Clears a depth/stencil hybrid texture to a given value with 24 depth bits and 8 stencil bits.

The dimensionality of the `subset` parameter must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function clear_tex_depthstencil( t::Texture,
                                 depth::Float32,
                                 stencil::UInt8,
                                 subset::TexSubset = default_tex_subset(t)
                                 ;
                                 recompute_mips::Bool = true
                               )
    if t.format == DepthStencilFormats.depth24u_stencil8
        return clear_tex_depthstencil(t, Depth24uStencil8u(depth, stencil),
                                      subset,
                                      recompute_mips=recompute_mips)
    elseif t.format == DepthStencilFormats.depth32f_stencil8
        return clear_tex_depthstencil(t, Depth32fStencil8u(depth, stencil),
                                      subset,
                                      recompute_mips=recompute_mips)
    else
        error("Texture doesn't appear to be a depth/stencil hybrid format: ", t.format)
    end
end
function clear_tex_depthstencil( t::Texture,
                                 value::Depth24uStencil8u,
                                 subset::TexSubset = default_tex_subset(t)
                                 ;
                                 recompute_mips::Bool = true
                               )
    @bp_check(t.format == DepthStencilFormats.depth24u_stencil8,
              "Trying to clear a texture with a hybrid 24u-depth/8-stencil value, on a texture of a different format: ", t.format)
    texture_op_impl(
        t,
        GL_UNSIGNED_INT_24_8,
        GL_DEPTH_STENCIL,
        subset,
        Ref(value),
        recompute_mips,
        Val(:Clear)
    )
end
function clear_tex_depthstencil( t::Texture,
                                 value::Depth32fStencil8u,
                                 subset::TexSubset = default_tex_subset(t)
                                 ;
                                 recompute_mips::Bool = true
                               )
    @bp_check(t.format == DepthStencilFormats.depth32f_stencil8,
              "Trying to clear a texture with a hybrid 32f-depth/8-stencil value, on a texture of a different format: ", t.format)
    texture_op_impl(
        t,
        GL_FLOAT_32_UNSIGNED_INT_24_8_REV,
        GL_DEPTH_STENCIL,
        subset,
        Ref(value),
        recompute_mips,
        Val(:Clear)
    )
end


"
Sets a texture's pixels, figuring out dynamically whether they're color, depth, etc.
For specific overloads based on texture format, see `set_tex_color()`, `set_tex_depth()`, etc respectively.

The dimensionality of the input array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_pixels(t::Texture, args...; kw_args...)
    if is_color(t.format)
        set_tex_color(t, args...; kw_args...)
    else
        if is_depth_only(t.format)
            set_tex_depth(t, args...; kw_args...)
        elseif is_stencil_only(t.format)
            set_tex_stencil(t, args...; kw_args...)
        elseif is_depth_and_stencil(t.format)
            set_tex_depthstencil(t, args...; kw_args...)
        else
            error("Unhandled case: ", t.format)
        end
    end
end
"
Sets the data for a color texture.
The dimensionality of the input array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_color( t::Texture,
                        pixels::TBuf,
                        subset::TexSubset = default_tex_subset(t)
                        ;
                        bgr_ordering::Bool = false,
                        single_component::E_PixelIOChannels = PixelIOChannels.red,
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

    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(T)),
        get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering, single_component),
                     is_integer(t.format)),
        subset,
        Ref(pixels, 1),
        recompute_mips,
        Val(:Set), vsize(pixels)
    )
end
"
Sets the data for a depth texture.
The dimensionality of the input array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_depth( t::Texture,
                        pixels::PixelBuffer{T},
                        subset::TexSubset = default_tex_subset(t)
                        ;
                        recompute_mips::Bool = true
                      ) where {T<:Union{Number, Vec{1, <:Number}}}
    @bp_check(is_depth_only(t.format),
              "Can't set depth values in a texture of format ", t.format)
    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(T)),
        GL_DEPTH_COMPONENT,
        subset,
        Ref(pixels, 1),
        recompute_mips,
        Val(:Set),
        vsize(pixels)
    )
end
"
Sets the data for a stencil texture.
The dimensionality of the input array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_stencil( t::Texture,
                          pixels::PixelBuffer{<:Union{UInt8, Vec{1, UInt8}}},
                          subset::TexSubset = default_tex_subset(t)
                          ;
                          recompute_mips::Bool = true
                        )
    @bp_check(is_stencil_only(t.format),
              "Can't set stencil values in a texture of format ", t.format)
    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(UInt8)),
        GL_STENCIL_INDEX,
        subset,
        Ref(pixels, 1),
        recompute_mips,
        Val(:Set),
        vsize(pixels)
    )
end
"
Sets the data for a depth/stencil hybrid texture.
The dimensionality of the input array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function set_tex_depthstencil( t::Texture,
                               pixels::PixelBuffer{T},
                               subset::TexSubset = default_tex_subset(t)
                               ;
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

    texture_op_impl(
        t,
        component_type,
        GL_STENCIL_INDEX,
        subset,
        Ref(pixels, 1),
        recompute_mips,
        Val(:Set),
        vsize(pixels)
    )
end

"
Gets a texture's pixels, figuring out dynamically whether they're color, depth, etc.
For specific overloads based on texture format, see `get_tex_color()`, `get_tex_depth()`, etc, respectively.

The dimensionality of the array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_pixels(t::Texture, args...; kw_args...)
    if is_color(t.format)
        get_tex_color(t, args...; kw_args...)
    else
        if is_depth_only(t.format)
            get_tex_depth(t, args...; kw_args...)
        elseif is_stencil_only(t.format)
            get_tex_stencil(t, args...; kw_args...)
        elseif is_depth_and_stencil(t.format)
            get_tex_depthstencil(t, args...; kw_args...)
        else
            error("Unhandled case: ", t.format)
        end
    end
end
"
Gets the data for a color texture and writes them into the given array.
The dimensionality of the array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_color( t::Texture,
                        out_pixels::TBuf,
                        subset::TexSubset = default_tex_subset(t)
                        ;
                        bgr_ordering::Bool = false,
                        single_component::E_PixelIOChannels = PixelIOChannels.red
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

    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(T)),
        get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering, single_component),
                    is_integer(t.format)),
        subset,
        Ref(out_pixels, 1),
        false,
        Val(:Get),
        vsize(out_pixels),
        ;
        get_buf_pixel_byte_size = N * sizeof(T)
    )
end
"
Gets the data for a depth texture and writes them into the given array.
The dimensionality of the array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_depth( t::Texture,
                        out_pixels::PixelBuffer{T},
                        subset::TexSubset = default_tex_subset(t)
                      ) where {T <: Union{Number, Vec{1, <:Number}}}
    @bp_check(is_depth_only(t.format), "Can't get depth data from a texture of format ", t.format)
    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(T)),
        GL_DEPTH_COMPONENT,
        subset,
        Ref(out_pixels, 1),
        false,
        Val(:Get),
        vsize(out_pixels),
        ;
        get_buf_pixel_byte_size = sizeof(T)
    )
end
"
Gets the data for a stencil texture and writes them into the given array.
The dimensionality of the array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_stencil( t::Texture,
                          out_pixels::PixelBuffer{<:Union{UInt8, Vec{1, UInt8}}},
                          subset::TexSubset = default_tex_subset(t)
                        )
    @bp_check(is_stencil_only(t.format), "Can't get stencil data from a texture of format ", t.format)
    texture_op_impl(
        t,
        GLenum(get_pixel_io_type(UInt8)),
        GL_STENCIL_INDEX,
        subset,
        Ref(out_pixels, 1),
        false,
        Val(:Get),
        vsize(out_pixels),
        ;
        get_buf_pixel_byte_size = sizeof(UInt8)
    )
end
"
Gets the data for a depth-stencil hybrid texture and writes them into the given array.
The dimensionality of the array and `subset` parameter
    must match the dimensionality of the texture.
Cubemaps textures are 3D, where Z spans the 6 faces.
"
function get_tex_depthstencil( t::Texture,
                               out_pixels::PixelBuffer{T},
                               subset::TexSubset = default_tex_subset(t)
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

    texture_op_impl(
        t,
        component_type,
        GL_DEPTH_STENCIL,
        subset, Ref(out_pixels, 1),
        false,
        Val(:Get),
        vsize(out_pixels)
        ;
        get_buf_pixel_byte_size = sizeof(T)
    )
end

println("#TODO: support glCopyImageSubData()")

export get_view,
       set_tex_swizzling, set_tex_depthstencil_source,
       get_mip_byte_size, get_gpu_byte_size,
       clear_tex_pixels, clear_tex_color, clear_tex_depth, clear_tex_stencil, clear_tex_depthstencil,
       set_tex_pixels, set_tex_color, set_tex_depth, set_tex_stencil,
       get_tex_pixels, get_tex_color, get_tex_depth, get_tex_stencil, get_tex_depthstencil