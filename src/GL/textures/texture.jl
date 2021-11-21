#TODO: Texture2DMSAA class.
#TODO: Copying from one texture to another (and from framebuffer into texture? it's redundant though).
#TODO: Memory Barriers. https://www.khronos.org/opengl/wiki/Memory_Model#Texture_barrier

"
Some kind of organized color data which can be 'sampled'.
This includes typical 2D textures, as well as 3D textures and 'cubemaps'.
Note that mip levels are counted from 1, not from 0,
   to match Julia's convention.
"
mutable struct Texture <: Resource
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
    sampler::Sampler{3}

    # If this is a hybrid depth/stencil texture,
    #    only one of those two components can be sampled at a time.
    depth_stencil_sampling::Optional{E_DepthStencilSources}

    # The mixing of this texture's channels when it's being sampled by a shader.
    swizzle::SwizzleRGBA
end
export Texture

function Base.close(t::Texture)
    glDeleteTextures(1, Ref(t.handle))
    setfield!(t, :handle, Ptr_Texture())
end


############################
#       Constructors       #
############################


"Creates a 1D texture"
function Texture( format::TexFormat,
                  width::Integer
                  ;
                  sampler::Sampler{1} = Sampler{1}(),
                  n_mips::Integer = get_n_mips(width),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )
    return generate_texture(
        TexTypes.oneD, format, v3u(width, 1, 1),
        convert(Sampler{3}, sampler),
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
                  sampler::Sampler{1} = Sampler{1}(),
                  n_mips::Integer = get_n_mips(width),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )
    return generate_texture(
        TexTypes.oneD, format, v3u(length(initial_data), 1, 1),
        convert(Sampler{3}, sampler),
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
                  sampler::Sampler{2} = Sampler{2}(),
                  n_mips::Integer = get_n_mips(width),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )
    return generate_texture(
        TexTypes.twoD, format, v3u(size, 1),
        convert(Sampler{3}, sampler),
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
                  sampler::Sampler{2} = Sampler{2}(),
                  n_mips::Integer = get_n_mips(width),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )
    return generate_texture(
        TexTypes.twoD, format,
        v3u(Vec(size(initial_data)), 1),
        convert(Sampler{3}, sampler),
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
                  sampler::Sampler{3} = Sampler{3}(),
                  n_mips::Integer = get_n_mips(width),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )
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
                  sampler::Sampler{3} = Sampler{3}(),
                  n_mips::Integer = get_n_mips(width),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing,
                  swizzling::SwizzleRGBA = SwizzleRGBA()
                )
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


################################
#    Private Implementation    #
################################

#TODO: Handle cubemap (constructor, and new behavior in generate_texture()), after implementing cube.jl

function generate_texture( type::E_TexTypes,
                           format::TexFormat,
                           size::v3u,
                           sampler::Sampler{3},
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

    tex::Texture = Texture(get_from_ogl(gl_type(Ptr_Texture), glCreateTextures, 1),
                           type, format, n_mips, size, sampler, depth_stencil_sampling)

    # Configure the texture with OpenGL calls.
    set_tex_swizzling(tex, swizzling)
    set_tex_depthstencil_source(tex, depth_stencil_sampling)
    apply(sampler, tex)

    # Set up the texture with non-reallocatable storage.
    if tex.type == TexTypes.oneD
        glTextureStorage1D(tex.handle, n_mips, get_ogl_enum(tex.format), size.x)
    elseif tex.type == TexTypes.twoD
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
default_tex_subset(tex::Texture) =
    if tex.type == TexTypes.oneD
        TexSubset{1}()
    elseif (tex.type == TexTypes.twoD) || (tex.type == TexTypes.cube)
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
                       mode::TMode,
                       get_buf_pixel_byte_size::Int = -1  # Only for Get ops
                     ) where {TMode <: @ano_enum(Set, Get, Clear)}
    #TODO: Check that the subset is the right number of dimensions

    full_size::v3u = get_mip_size(t.size, subset.mip)
    full_subset::TexSubset{3} = change_dimensions(subset, 3)
    range::Box{v3u} = get_subset_range(full_subset, full_size)

    # Perform the requested operation.
    if mode == Val(:Set)
        if tex.type == TexTypes.oneD
            glTextureSubImage1D(tex.handle, subset.mip - 1,
                                range.min.x - 1,
                                range.size.x,
                                components, component_type,
                                value)
        elseif tex.type == TexTypes.twoD
            glTextureSubImage2D(tex.handle, subset.mip - 1,
                                (range.min.xy - 1)...,
                                range.size.xy...,
                                components, component_type,
                                value)
        elseif tex.type == TexTypes.threeD
            glTextureSubImage3D(tex.handle, subset.mip - 1,
                                (range.min - 1)...,
                                range.size...,
                                components, component_type,
                                value)
        else
            error("Unhandled case: ", tex.type)
        end
    elseif mode == Val(:Get)
        @bp_gl_assert(get_buf_pixel_byte_size > 0,
                      "Internal field 'get_buf_pixel_byte_size' not passed for getting texture data")
        @bp_gl_assert(!recompute_mips, "Asked to recompute mips after getting a texture's pixels, which is pointless")
        glGetTextureSubImage(tex.handle, subset.mip - 1,
                             (range.min - 1)..., range.size...,
                             components, component_type,
                             prod(range.size) * get_buf_pixel_byte_size,
                             value)
    elseif mode == Val(:Clear)
        glClearTexSubImage(tex.handle, subset.mip - 1,
                           (range.min - 1)..., range.size...,
                           components, component_type,
                           value)
    end

    if recompute_mips
        println("#TODO: Recompute mips")
    end
end


##########################
#    Public Interface    #
##########################

get_ogl_handle(t::Texture) = t.handle

"Changes how a texture's pixels are mixed when it's sampled on the GPU"
function set_tex_swizzling(t::Texture, new::SwizzleRGBA)
    glTextureParameteri(t.handle, GL_TEXTURE_SWIZZLE_RGBA, Ref(new))
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


#TODO: 'clear_tex_pixels()' infers the format type automatically. Do the same for getting
"Clears a color texture to a given value"
function clear_tex_color( t::Texture,
                          color::PixelIOValue
                          ;
                          subset::TexSubset = default_tex_subset(t),
                          bgr_ordering::Bool = false,
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
                 get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering),
                              is_integer(t.format)),
                 subset, Ref(color), recompute_mips, Val(:Clear))
end
"Clears a depth texture to a given value"
function clear_tex_depth( t::Texture,
                          depth::T
                          ;
                          subset::TexSubset = default_tex_subset(t),
                          recompute_mips::Bool = true
                        ) where {T<:PixelIOComponent}
    @bp_check(is_depth_only(t.format),
              "Can't clear depth in a texture of format ", t.format)
    texture_data(t,
                 GLenum(get_pixel_io_type(T)),
                 GL_DEPTH_COMPONENT,
                 subset, Ref(depth), recompute_mips, Val(:Clear))
end
"Clears a stencil texture to a given value"
function clear_tex_stencil( t::Texture,
                            stencil::UInt8
                            ;
                            subset::TexSubset = default_tex_subset(t)
                          )
    @bp_check(is_stencil_only(t.format),
              "Can't clear stencil in a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(UInt8)), GL_STENCIL_INDEX,
                 subset, Ref(stencil), false, Val(:Clear))
end
"Clears a depth/stencil hybrid texture to a given value with 24 depth bits and 8 stencil bits"
function clear_tex_depthstencil( t::Texture,
                                 depth::Float32,
                                 stencil::UInt8
                                 ;
                                 subset::TexSubset = default_tex_subset(t),
                                 recompute_mips::Bool = true
                               )
    if t.format == DepthStencilFormats.depth24u_stencil8
        return clear_tex_depthstencil(t, Depth24uStencil8u(depth, stencil),
                                      subset=subset, recompute_mips=recompute_mips)
    elseif t.format == DepthStencilFormats.depth32f_stencil8
        return clear_tex_depthstencil(t, Depth32fStencil8u(depth, stencil),
                                      subset=subset, recompute_mips=recompute_mips)
    else
        error("Texture doesn't appear to be a depth/stencil hybrid format: ", t.format)
    end
end
function clear_tex_depthstencil( t::Texture,
                                 value::Depth24uStencil8u
                                 ;
                                 subset::TexSubset = default_tex_subset(t),
                                 recompute_mips::Bool = true
                               )
    @bp_check(t.format == DepthStencilFormats.depth24u_stencil8,
              "Trying to clear a texture with a hybrid 24u-depth/8-stencil value, on a texture of a different format: ", t.format)
    texture_data(t, GL_UNSIGNED_INT_24_8, GL_DEPTH_STENCIL,
                 subset, Ref(value), recompute_mips, Val(:Clear))
end
function clear_tex_depthstencil( t::Texture,
                                 value::Depth32fStencil8u
                                 ;
                                 subset::TexSubset = default_tex_subset(t),
                                 recompute_mips::Bool = true
                               )
    @bp_check(t.format == DepthStencilFormats.depth32f_stencil8,
              "Trying to clear a texture with a hybrid 32f-depth/8-stencil value, on a texture of a different format: ", t.format)
    texture_data(t, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH_STENCIL,
                 subset, Ref(value), recompute_mips, Val(:Clear))
end


"
Sets a texture's pixels.
For specific overloads based on texture format, see `set_tex_color()`, `set_tex_depth()`, etc.
"
function set_tex_pixels( t::Texture,
                         pixels::PixelBuffer
                         ;
                         subset::TexSubset = default_tex_subset(t),
                         color_bgr_ordering::Bool = false,
                         recompute_mips::Bool = true
                       )
    if is_color(t.format)
        set_tex_color(t, pixels; subset=subset,
                      bgr_ordering=color_bgr_ordering,
                      recompute_mips=recompute_mips)
    else
        @bp_check(!color_bgr_ordering,
                  "Supplied the 'color_bgr_ordering' parameter for a non-color texture")
        if is_depth_only(t.format)
            set_tex_depth(t, pixels; subset=subset, recompute_mips=recompute_mips)
        elseif is_stencil_only(t.format)
            set_tex_stencil(t, pixels; subset=subset, recompute_mips=recompute_mips)
        elseif is_depth_and_stencil(t.format)
            set_tex_depthstencil(t, pixels; subset=subset, recompute_mips=recompute_mips)
        else
            error("Unhandled case: ", t.format)
        end
    end
end
"Sets the data for a color texture"
function set_tex_color( t::Texture,
                        pixels::TBuf
                        ;
                        subset::TexSubset = default_tex_subset(t),
                        bgr_ordering::Bool = false,
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
                 subset, Ref(pixels), recompute_mips,
                 Val(:Set))
end
"Sets the data for a depth texture"
function set_tex_depth( t::Texture,
                        pixels::PixelBuffer{T}
                        ;
                        subset::TexSubset = default_tex_subset(t),
                        recompute_mips::Bool = true
                      ) where {T<:Union{Number, Vec{1, <:Number}}}
    @bp_check(is_depth_only(t.format),
              "Can't set depth values in a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(T)), GL_DEPTH_COMPONENT,
                 subset, Ref(pixels), recompute_mips,
                 Val(:Set))
end
"Sets the data for a stencil texture"
function set_tex_stencil( t::Texture,
                          pixels::@unionspec(PixelBuffer{_}, UInt8, Vec{1, UInt8})
                          ;
                          subset::TexSubset = default_tex_subset(t),
                          recompute_mips::Bool = true
                        )
    @bp_check(is_stencil_only(t.format),
              "Can't set stencil values in a texture of format ", t.format)
    texture_data(t,
                 GLenum(get_pixel_io_type(UInt8)),
                 GL_STENCIL_INDEX,
                 subset, Ref(pixels), recompute_mips,
                 Val(:Set))
end
"Sets the data for a depth/stencil hybrid texture"
function set_tex_depthstencil( t::Texture,
                               pixels::PixelBuffer{T}
                               ;
                               subset::TexSubset = default_tex_subset(t),
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
                 subset, Ref(pixels), recompute_mips,
                 Val(:Set))
end
#TODO: set_tex_compressed() for compressed-format textures

"Gets the pixel data for a color texture"
function get_tex_color( t::Texture,
                        out_pixels::TBuf
                        ;
                        subset::TexSubset = default_tex_subset(t),
                        bgr_ordering::Bool = false,
                      ) where {TBuf <: PixelBuffer}
    T = get_component_type(out_pixels)
    N = get_component_count(out_pixels)

    @bp_check(!(t.format isa E_CompressedFormats),
              "Can't get compressed texture data as if it's normal color data!")
    @bp_check(is_color(t.format), "Can't get color data from a texture of format ", t.format)
    @bp_check(!is_integer(t.format) || T<:Integer,
              "Can't set an integer texture with a non-integer value")
    @bp_check(!bgr_ordering || N >= 3,
              "Supplied the 'bgr_ordering' parameter but the data only has 1 or 2 components")

    texture_data(t,
                 GLenum(get_pixel_io_type(T)),
                 get_ogl_enum(get_pixel_io_channels(Val(N), bgr_ordering)),
                 subset, Ref(out_pixels), recompute_mips,
                 Val(:Get), N * sizeof(T))
end
"Gets the pixel data for a depth texture"
function get_tex_depth( t::Texture,
                        out_pixels::PixelBuffer{T}
                        ;
                        subset::TexSubset = default_tex_subset(t)
                      ) where {T <: Union{Number, Vec{1, <:Number}}}
    @bp_check(is_depth_only(t.format), "Can't get depth data from a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(T)), GL_DEPTH_COMPONENT,
                 subset, Ref(out_pixels), false,
                 Val(:Get), sizeof(T))
end
"Gets the pixel data for a stencil texture"
function get_tex_stencil( t::Texture,
                          out_pixels::TBuf
                          ;
                          subset::TexSubset = default_tex_subset(t)
                        ) where {TBuf <: @unionspec(PixelBuffer{_}, UInt8, Vec{1, UInt8})}
    @bp_check(is_stencil_only(t.format), "Can't get stencil data from a texture of format ", t.format)
    texture_data(t, GLenum(get_pixel_io_type(UInt8)), GL_STENCIL_INDEX,
                 subset, Ref(out_pixels), false,
                 Val(:Get), sizeof(UInt8))
end
"Get the data for a depth/stencil hybrid texture"
function get_tex_depthstencil( t::Texture,
                               pixels::PixelBuffer{T}
                               ;
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

    texture_data(t, component_type, GL_DEPTH_STENCIL,
                 subset, Ref(pixels), false,
                 Val(:Get), sizeof(T))
end
#TODO: get_tex_compressed() for compressed-format textures

export set_tex_swizzling, set_tex_depthstencil_source,
       get_mip_byte_size, get_gpu_byte_size,
       clear_tex_color, clear_tex_depth, clear_tex_stencil, clear_tex_depthstencil,
       set_tex_pixels, set_tex_color, set_tex_depth, set_tex_stencil,
       get_tex_color, get_tex_depth, get_tex_stencil, get_tex_depthstencil


##########################
#    TexView/ImgView     #
##########################

#TODO: Implement
