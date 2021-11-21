# Defines other aspects of texture interaction
#    which don't fit into "sampling" and "format".


#######################################
#       Upload/download channels      #
#######################################

# Subsets of color channels that can be uploaded to a texture.
@bp_gl_enum(PixelIOChannels::GLenum,
    red = GL_RED,
    green = GL_GREEN,
    blue = GL_BLUE,

    RG = GL_RG,
    RGB = GL_RGB,
    RGBA = GL_RGBA,

    BGR = GL_BGR,
    BGRA = GL_BGRA
)

"Gets the number of channels that will get uploaded with the given upload format"
get_n_channels(c::E_PixelIOChannels) =
    if (c == PixelIOChannels.red) ||
       (c == PixelIOChannels.green) ||
       (c == PixelIOChannels.blue)
        1
    elseif (c == PixelIOChannels.RG)
        2
    elseif (c == PixelIOChannels.RGB) ||
           (c == PixelIOChannels.BGR)
        3
    elseif (c == PixelIOChannels.RGBA) ||
           (c == PixelIOChannels.BGRA)
        4
    else
        error("Unhandled case: ", c)
    end
# get_n_channels() is already defined an exported by another GL function

"Gets the OpenGL enum to use when uploading an integer version of the given components"
get_int_ogl_enum(c::E_PixelIOChannels) =
    if c == PixelIOChannels.red
        GL_RED_INTEGER
    elseif c == PixelIOChannels.green
        GL_GREEN_INTEGER
    elseif c == PixelIOChannels.blue
        GL_BLUE_INTEGER
    elseif c == PixelIOChannels.RG
        GL_RG_INTEGER
    elseif c == PixelIOChannels.RGB
        GL_RGB_INTEGER
    elseif c == PixelIOChannels.RGBA
        GL_RGBA_INTEGER
    elseif c == PixelIOChannels.BGR
        GL_BGR_INTEGER
    elseif c == PixelIOChannels.BGRA
        GL_BGRA_INTEGER
    else
        error("Unhandled case: ", c)
    end

"Gets the set of channels needed to handle data with the given number of components"
function get_pixel_io_channels( ::Val{N},
                                use_bgr_ordering::Bool,
                                color_for_1D::E_PixelIOChannels = PixelIOChannels.red
                              )::E_PixelIOChannels where {N}
    if N == 1
        return color_for_1D
    elseif N == 2
        return PixelIOChannels.RG
    elseif N == 3
        return use_bgr_ordering ? PixelIOChannels.BGR : PixelIOChannels.RGB
    elseif N == 4
        return use_bgr_ordering ? PixelIOChannels.BGRA : PixelIOChannels.RGBA
    else
        error("Unhandled case: ", N)
    end
end

"
Gets the OpenGL enum for a set of channels being uploaded to a texture.
Needs to know whether the texture has an int/uint format.
"
function get_ogl_enum(channels::E_PixelIOChannels, is_integer_format::Bool)
    if is_integer_format
        if channels == PixelIOChannels.red
            return GL_RED_INTEGER
        elseif channels == PixelIOChannels.green
            return GL_GREEN_INTEGER
        elseif channels == PixelIOChannels.blue
            return GL_BLUE_INTEGER
        elseif channels == PixelIOChannels.RG
            return GL_RG_INTEGER
        elseif channels == PixelIOChannels.RGB
            return GL_RGB_INTEGER
        elseif channels == PixelIOChannels.BGR
            return GL_BGR_INTEGER
        elseif channels == PixelIOChannels.RGBA
            return GL_RGBA_INTEGER
        elseif channels == PixelIOChannels.BGRA
            return GL_BGRA_INTEGER
        else
            error("Unhandled case: ", channels)
        end
    else
        return GLenum(channels)
    end
end


export PixelIOChannels, E_PixelIOChannels


#########################################
#       Upload/download data types      #
#########################################

# Data types that pixel data can be uploaded/downloaded as.
# This does not always have to match the texture's format or sampler.
@bp_gl_enum(PixelIOTypes::GLenum,
    uint8 = GL_UNSIGNED_BYTE,
    uint16 = GL_UNSIGNED_SHORT,
    uint32 = GL_UNSIGNED_INT,
    int8 = GL_BYTE,
    int16 = GL_SHORT,
    int32 = GL_INT,
    float32 = GL_FLOAT
)

"Gets the byte-size of the given pixel component type"
get_byte_size(t::E_PixelIOTypes) =
    if (t == PixelIOTypes.uint8) || (t == PixelIOTypes.int8)
        1
    elseif (t == PixelIOTypes.uint16) || (t == PixelIOTypes.int16)
        2
    elseif (t == PixelIOTypes.uint32) || (t == PixelIOTypes.int32) || (t == PixelIOTypes.float32)
        4
    else
        error("Unhandled case: ", t)
    end

"Converts from a type to the corresponding PixelIOTypes"
get_pixel_io_type(::Type{UInt8}) = PixelIOTypes.uint8
get_pixel_io_type(::Type{UInt16}) = PixelIOTypes.uint16
get_pixel_io_type(::Type{UInt32}) = PixelIOTypes.uint32
get_pixel_io_type(::Type{Int8}) = PixelIOTypes.int8
get_pixel_io_type(::Type{Int16}) = PixelIOTypes.int16
get_pixel_io_type(::Type{Int32}) = PixelIOTypes.int32
get_pixel_io_type(::Type{Float32}) = PixelIOTypes.float32

"
The set of all number types which can be used for pixel upload/download.
Does not include special packed types like Depth24uStencil8.
"
@inline supported_pixel_io_types() = (
    UInt8, UInt16, UInt32,
    Int8, Int16, Int32,
    Float32
    #TODO: I think Float16 is supported as well! GL_HALF_FLOAT
)


"Types which can be used as pixel components for texture upload/download"
const PixelIOComponent = Union{supported_pixel_io_types()..., Depth24uStencil8u, Depth32fStencil8u}

"
Types which can be uploaded to/downloaded from a texture.
One-component texture data can be a number or a Vec1;
   higher-component data must be a Vec.
"
const PixelIOValue = Union{PixelIOComponent, @unionspec(Vec{_, <:PixelIOComponent}, 1, 2, 3, 4)}


"An array of pixels for texture upload/download"
const PixelBufferD{N, T<:PixelIOValue} = Array{T, N}

"The total set of valid pixel arrays for texture upload/download"
const PixelBuffer{T} = Union{PixelBufferD{1, T}, PixelBufferD{2, T}, PixelBufferD{3, T}}

"Gets the type of pixel data in the given buffer"
get_pixel_type(::PixelBufferD{N, T}) where {N, T} = T
"Gets the type of the individual components in some pixel buffer data"
get_component_type(T::Type{<:Number}) = T
get_component_type(V::Type{<:Vec}) = V.parameters[2]
get_component_type(::Type{PixelBufferD{N, T}}) where {N, T} = get_component_type(T)
"Gets the number of components in some pixel buffer data"
get_component_count(T::Type{<:Number}) = 1
get_component_count(V::Type{<:Vec}) = V.parameters[1]
get_component_count(::Type{PixelBufferD{N, T}}) where {N, T} = get_component_count(T)


export PixelIOTypes, E_PixelIOTypes,
       get_pixel_io_type, supported_pixel_io_types, get_component_type, get_component_count,
       PixelIOComponent, PixelIOValue, PixelBufferD, PixelBuffer


#########################################
#       Upload/download data types      #
#########################################

#TODO: Provide a helper alias for "number or vector based on type parameter N".

"
Parameters for specifying a subset of data in an N-dimensional texture.
Note that mips and pixel ranges follow the 1-based Julia convention,
   not the 0-based convention of OpenGL.
"
struct TexSubset{N}
    # Rectangular subset of the texture (or 'nothing' to use all the pixels).
    # The coordinates are numbers for 1D textures, and vectors for 2D+.
    pixels::Optional{Box{N == 1 ? UInt : VecU{N}}}
    # The mip level of the texture to focus on.
    # 1 means the original (full-size) texture.
    # Higher values are smaller mips.
    mip::UInt

    TexSubset{N}(pixels = nothing, mip = 1) where {N} = new(pixels, convert(UInt, mip))
end

"
Calculates the subset of a texture to use.
The 'full_size' should match the dimensionality of the texture.
"
@inline get_subset_range(subset::TexSubset, full_size::Union{Integer, Vec}) =
    isnothing(subset.pixels) ?
        convert(Box{typeof(full_size)}, Box_minmax(1, full_size)) :
        subset.pixels

"
Converts a subset to a lower- or higher-dimensional one.
"
@inline change_dimensions(subset::TexSubset{N}, N2::Int) where {N} = TexSubset{N2}(
    isnothing(subset.pixels) ?
        nothing :
        reshape(subset.pixels, 3),
    subset.mip
)


export TexSubset


########################
#       Swizzling      #
########################

# Various sources a color texture can pull from during sampling.
# Note that swizzling is set per-texture, not per-sampler.
@bp_gl_enum(SwizzleSources::GLenum,
    red = GL_RED,
    green = GL_GREEN,
    blue = GL_BLUE,
    alpha = GL_ALPHA,

    # The below sources output a constant value instead of reading from the texture.
    zero = GL_ZERO,
    one = GL_ONE
)

const SwizzleRGBA = Vec4{E_SwizzleSources}
SwizzleRGBA() = Vec(SwizzleSources.red, SwizzleSources.green, SwizzleSources.blue, SwizzleSources.alpha)

export SwizzleSources, E_SwizzleSources


######################
#       General      #
######################

"Gets the number of mips needed for a texture of the given size"
@inline function get_n_mips(tex_size::Union{Integer, VecT{<:Integer}})
    largest_axis = max(tex_size)
    return 1 + Int(floor(log2(largest_axis)))
end
get_n_mips(length::Integer) = get_n_mips(Vec(length))

"Gets the size of a mip level, given the original size of the texture"
@inline function get_mip_size(original_size::Union{Integer, VecT{<:Integer}}, mip_level::Integer)
    size = original_size
    for _ in 2:mip_level
        size = max(size ÷ 2, 1)
    end
    return size
end

# When using this texture through an ImageView, these are the modes it is available in.
@bp_gl_enum(ImageAccessModes::GLenum,
    read = GL_READ_ONLY,
    write = GL_WRITE_ONLY,
    read_write = GL_READ_WRITE
)

export get_n_mips, get_mip_size,
       ImageAccessModes, E_ImageAccessModes