####################
#       Enums      #
####################

# The types of textures that can be created.
@bp_gl_enum(TexTypes::GLenum,
    oneD = GL_TEXTURE_1D,
    twoD = GL_TEXTURE_2D,
    threeD = GL_TEXTURE_3D,
    cube_map = GL_TEXTURE_CUBE_MAP

    # Array textures are not supported, because they aren't necessary
    #    now that we have bindless textures.
)

@bp_gl_enum(ColorChannels::UInt8,
    red = 1,
    green = 2,
    blue = 3,
    alpha = 4
)
@bp_gl_enum(OtherChannels::UInt8,
    depth = 5,
    stencil = 6
)
const AllChannels = Union{E_ColorChannels, E_OtherChannels}



# The type of data representing each channel of a texture's pixels.
@bp_gl_enum(FormatTypes::UInt8,
    # A floating-point number (allowing "HDR" values outside the traditional 0-1 range)
    float,
    # A value between 0 - 1, stored as an unsigned integer between 0 and its maximum value.
    normalized_uint,
    # A value between -1 and +1, stored as a signed integer between its minimum and maximum value.
    normalized_int,

    # An unsigned integer. Sampling from this texture yields integers, not floats.
    uint,
    # A signed integer. Sampling from this texture yields integers, not floats.
    int
)

export TexTypes, E_TexTypes,
       ColorChannels, E_ColorChannels,
       OtherChannels, E_OtherChannels,
       AllChannels,
       FormatTypes, E_FormatTypes
#


###########################
#   TexFormat interface   #
###########################

# The format types are split into different "archetypes",
#    each of which implement the following queries:

"Is the given format supported for the given TexType?"
function is_supported end

"Is the given format a color type (as opposed to depth and/or stencil)?"
function is_color end

"Is the given format a depth type (no color/stencil)?"
function is_depth_only end
"Is the given format a stencil type (no color/depth)?"
function is_stencil_only end
"Is the given format a hybrid depth/stencil type?"
function is_depth_and_stencil end

"Does the given format use FormatTypes.int or FormatTypes.uint?"
function is_integer end
"
Does the given format have signed components, or unsigned components?
NOTE that for color formats, this is primarily about the RGB components;
    the Alpha may sometimes be in a different format.
"
function is_signed end

"Does the given format store the given channel?"
function stores_channel end

"Gets the number of channels offered by the given format."
function get_n_channels end

"
Gets the size (in bits) of each pixel for the given texture format.
If the format is block-compressed, this will provide an accurate answer
   by dividing the block's bit-size by the number of pixels in that block.
"
function get_pixel_bit_size end

"Gets the total size (in bytes) of a texture of the given format and size"
function get_byte_size end

"
Gets the OpenGL enum value representing this format,
  or 'nothing' if the format isn't valid.
"
function get_ogl_enum end

export get_ogl_enum
#


"
Gets the OpenGL enum value for the actual format that this machine's GPU will use
   to represent the given format, on the given type of texture.
Often-times, the graphics driver will pick a more robust format
   (e.x. r3_g3_b2 is almost always upgraded to r5_g6_b5).
If no texture type is given, then the format will be used in a TargetBuffer.
Returns 'nothing' if the format is invalid for the given texture type.
"
function get_native_ogl_enum(format, tex_type::Optional{E_TexTypes})::GLenum
    gl_tex_type::GLenum = exists(tex_type) ? GLenum(tex_type) : GL_RENDERBUFFER
    gl_format::GLenum = get_ogl_enum(format)

    actual_format::GLint = get_from_ogl(GLint, glGetInternalformativ,
                                        gl_tex_type, gl_format, GL_INTERNALFORMAT_PREFERRED, 1)
    return actual_format
end
export get_native_ogl_enum


# Default behavior for texture byte size: pixel bit size * number of pixels * 8.
get_byte_size(format, size::VecI) = get_pixel_bit_size(format) * 8 * reduce(*, map(Int64, size))


###########################
#       SimpleFormat      #
###########################

# The sets of components that can be used in "simple" texture formats.
@bp_gl_enum(SimpleFormatComponents::UInt8,
    R = 1,
    RG = 2,
    RGB = 3,
    RGBA = 4
)

# The bit-depths that components can have in "simple" texture formats.
# Note that not all combinations of bit depth and components are legal
#   (for example, 2-bit components are only allowed if you use all four channels, RGBA).
@bp_gl_enum(SimpleFormatBitDepths::UInt8,
    B2 = 2,
    B4 = 4,
    B5 = 5,
    B8 = 8,
    B10 = 10,
    B12 = 12,
    B16 = 16,
    B32 = 32
)

"A straight-forward texture format, with each component having the same bit-depth"
struct SimpleFormat
    type::E_FormatTypes
    components::E_SimpleFormatComponents
    bit_size::E_SimpleFormatBitDepths
end
Base.show(io::IO, f::SimpleFormat) = print(io, 
    f.components, Int(f.bit_size), "<", f.type, ">"
)

export SimpleFormatComponents, E_SimpleFormatComponents,
       SimpleFormatBitDepths, E_SimpleFormatBitDepths,
       SimpleFormat
#

is_supported(::SimpleFormat, ::E_TexTypes) = true

is_color(f::SimpleFormat) = true
is_depth_only(f::SimpleFormat) = false
is_stencil_only(f::SimpleFormat) = false
is_depth_and_stencil(f::SimpleFormat) = false

is_integer(f::SimpleFormat) = (f.type == FormatTypes.uint) || (f.type == FormatTypes.int)
is_signed(f::SimpleFormat) = (f.type == FormatTypes.int) || (f.type == FormatTypes.float) || (f.type == FormatTypes.normalized_int)

stores_channel(f::SimpleFormat, c::E_ColorChannels) =
    if f.components == SimpleFormatComponents.R
        return c == ColorChannels.red
    elseif f.components == SimpleFormatComponents.RG
        return c <= ColorChannels.green
    elseif f.components == SimpleFormatComponents.RGB
        return c <= ColorChannels.blue
    elseif f.components == SimpleFormatComponents.RGBA
        return true
    else
        error("Unhandled case: ", f.components)
    end
stores_channel(f::SimpleFormat, c::E_OtherChannels) = false

get_n_channels(f::SimpleFormat) = Int(f.components)

get_pixel_bit_size(f::SimpleFormat) = Int(f.bit_size) * Int(f.components)

function get_ogl_enum(f::SimpleFormat)::Optional{GLenum}
    @inline check_components(if_r, if_rg, if_rgb, if_rgba) =
        if (f.components == SimpleFormatComponents.R)
            if_r
        elseif (f.components == SimpleFormatComponents.RG)
            if_rg
        elseif (f.components == SimpleFormatComponents.RGB)
            if_rgb
        elseif (f.components == SimpleFormatComponents.RGBA)
            if_rgba
        else
            error("Unhandled case: ", f.components)
        end
    @inline check_types(if_float, if_unorm, if_inorm, if_uint, if_int) =
        if (f.type == FormatTypes.float)
            if_float()
        elseif (f.type == FormatTypes.normalized_uint)
            if_unorm()
        elseif (f.type == FormatTypes.normalized_int)
            if_inorm()
        elseif (f.type == FormatTypes.uint)
            if_uint()
        elseif (f.type == FormatTypes.int)
            if_int()
        else
            error("Unhandled case: ", f.type)
        end
    if (f.bit_size == SimpleFormatBitDepths.B2)
        return ((f.type == FormatTypes.normalized_uint) && (f.components == SimpleFormatComponents.RGBA)) ?
                   GL_RGBA2 :
                   nothing
    elseif (f.bit_size == SimpleFormatBitDepths.B4)
        return (f.type == FormatTypes.normalized_uint) ?
                   check_components(nothing, nothing, GL_RGB4, GL_RGBA4) :
                   nothing
    elseif (f.bit_size == SimpleFormatBitDepths.B5)
        return ((f.type == FormatTypes.normalized_uint) && (f.components == SimpleFormatComponents.RGB)) ?
                   GL_RGB5 :
                   nothing
    elseif (f.bit_size == SimpleFormatBitDepths.B8)
        return check_types(() -> nothing,
                           () -> check_components(GL_R8, GL_RG8, GL_RGB8, GL_RGBA8),
                           () -> check_components(GL_R8_SNORM, GL_RG8_SNORM, GL_RGB8_SNORM, GL_RGBA8_SNORM),
                           () -> check_components(GL_R8UI, GL_RG8UI, GL_RGB8UI, GL_RGBA8UI),
                           () -> check_components(GL_R8I, GL_RG8I, GL_RGB8I, GL_RGBA8I))
    elseif (f.bit_size == SimpleFormatBitDepths.B10)
        return ((f.type == FormatTypes.normalized_uint) && (f.components == SimpleFormatComponents.RGB)) ?
                   GL_RGB10 :
                   nothing
    elseif (f.bit_size == SimpleFormatBitDepths.B12)
        return (f.type == FormatTypes.normalized_uint) ?
                   check_components(nothing, nothing, GL_RGB12, GL_RGBA12) :
                   nothing
    elseif (f.bit_size == SimpleFormatBitDepths.B16)
        return check_types(() -> check_components(GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F),
                           () -> check_components(GL_R16, GL_RG16, GL_RGB16, GL_RGBA16),
                           () -> check_components(GL_R16_SNORM, GL_RG16_SNORM, GL_RGB16_SNORM, GL_RGBA16_SNORM),
                           () -> check_components(GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI),
                           () -> check_components(GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I))
    elseif (f.bit_size == SimpleFormatBitDepths.B32)
        return check_types(() -> check_components(GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F),
                           () -> nothing, () -> nothing,
                           () -> check_components(GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI),
                           () -> check_components(GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I))
    else
        error("Unhandled case: ", f.bit_size)
    end
end


###########################
#      SpecialFormat      #
###########################

# Special one-off texture formats.
@bp_gl_enum(SpecialFormats::GLenum,
    # normalized_uint texture packing each pixel into 2 bytes:
    #     5 bits for Red, 6 for Green, and 5 for Blue (no alpha).
    r5_g6_b5 = GL_RGB565,

    # normalized_uint texture packing each pixel into 4 bytes:
    #     10 bits each for Red, Green, and Blue, and 2 bits for Alpha.
    rgb10_a2 = GL_RGB10_A2,

    # UInt texture (meaning it outputs integer values, not floats!)
    #     that packs each pixel into 4 bytes:
    #     10 bits each for Red, Green, and Blue, and 2 bits for Alpha.
    rgb10_a2_uint = GL_RGB10_A2UI,

    # Floating-point texture using special unsigned 11-bit floats for Red and Green,
    #     and unsigned 10-bit float for Blue. No Alpha.
    # Floats of this size can represent values from .0000610 to 65500,
    #     with ~2 digits of precision.
    rgb_tiny_ufloats = GL_R11F_G11F_B10F,

    # Floating-point texture using unsigned 14-bit floats for RGB (no alpha), with a catch:
    # They share the same 5-bit exponent, to fit into 32 bits per pixel.
    rgb_shared_exp_ufloats = GL_RGB9_E5,


    # Each pixel is a 24-bit sRGB colorspace image (no alpha).
    # Each channel is 8 bits, and the texture data is treated as non-linear,
    #     which means it's converted into linear values on the fly when sampled.
    sRGB = GL_SRGB8,

    # Same as sRGB, but with the addition of a linear (meaning non-sRGB) 8-bit Alpha value.
    sRGB_linear_alpha = GL_SRGB8_ALPHA8,


    # normalized_uint texture packing each pixel into a single byte:
    #     3 bits for Red, 3 for Green, and 2 for Blue (no alpha).
    # Note that, from reading on the Internet,
    #     it seems most hardware just converts to r5_g6_b5 under the hood.
    r3_g3_b2 = GL_R3_G3_B2,

    # normalized_uint texture packing each pixel into 2 bytes:
    #     5 bits each for Red, Green, and Blue, and 1 bit for Alpha.
    # It is highly recommended to use a compressed format instead of this one.
    rgb5_a1 = GL_RGB5_A1
)

export SpecialFormats, E_SpecialFormats

is_supported(::E_SpecialFormats, ::E_TexTypes) = true

is_color(f::E_SpecialFormats) = true
is_depth_only(f::E_SpecialFormats) = false
is_stencil_only(f::E_SpecialFormats) = false
is_depth_and_stencil(f::E_SpecialFormats) = false

function is_integer(f::E_SpecialFormats)
    if (f == SpecialFormats.r3_g3_b2) ||
       (f == SpecialFormats.r5_g6_b5) ||
       (f == SpecialFormats.rgb10_a2) ||
       (f == SpecialFormats.rgb5_a1) ||
       (f == SpecialFormats.rgb_shared_exp_ufloats) ||
       (f == SpecialFormats.rgb_tiny_ufloats) ||
       (f == SpecialFormats.sRGB) ||
       (f == SpecialFormats.sRGB_linear_alpha)
    #begin
        return false
    elseif (f == SpecialFormats.rgb10_a2_uint)
        return true
    else
        error("Unhandled case: ", f)
    end
end
function is_signed(f::E_SpecialFormats)
    if (f == SpecialFormats.r3_g3_b2) ||
       (f == SpecialFormats.r5_g6_b5) ||
       (f == SpecialFormats.rgb10_a2) ||
       (f == SpecialFormats.rgb5_a1) ||
       (f == SpecialFormats.rgb_shared_exp_ufloats) ||
       (f == SpecialFormats.rgb_tiny_ufloats) ||
       (f == SpecialFormats.sRGB) ||
       (f == SpecialFormats.sRGB_linear_alpha) ||
       (f == SpecialFormats.rgb10_a2_uint)
    #begin
        return false
    else
        error("Unhandled case: ", f)
    end
end

function stores_channel(f::E_SpecialFormats, c::E_ColorChannels)
    if (f == SpecialFormats.r3_g3_b2) ||
       (f == SpecialFormats.r5_g6_b5) ||
       (f == SpecialFormats.rgb_shared_exp_ufloats) ||
       (f == SpecialFormats.rgb_tiny_ufloats) ||
       (f == SpecialFormats.sRGB)
    #begin
        return stores_channel(SimpleFormat(FormatTypes.normalized_uint,
                                           SimpleFormatComponents.RGB,
                                           SimpleFormatBitDepths.B8),
                              c)
    elseif (f == SpecialFormats.rgb10_a2) ||
           (f == SpecialFormats.rgb10_a2_uint) ||
           (f == SpecialFormats.rgb5_a1) ||
           (f == SpecialFormats.sRGB_linear_alpha)
    #begin
        return stores_channel(SimpleFormat(FormatTypes.normalized_uint,
                                           SimpleFormatComponents.RGBA,
                                           SimpleFormatBitDepths.B8),
                              c)
    else
        error("Unhandled case: ", f)
    end
end
stores_channel(f::E_SpecialFormats, c::E_OtherChannels) = false

function get_n_channels(f::E_SpecialFormats)
    if (f == SpecialFormats.r3_g3_b2) ||
       (f == SpecialFormats.r5_g6_b5) ||
       (f == SpecialFormats.rgb_shared_exp_ufloats) ||
       (f == SpecialFormats.rgb_tiny_ufloats) ||
       (f == SpecialFormats.sRGB)
    #begin
        return 3
    elseif (f == SpecialFormats.rgb10_a2) ||
           (f == SpecialFormats.rgb10_a2_uint) ||
           (f == SpecialFormats.rgb5_a1) ||
           (f == SpecialFormats.sRGB_linear_alpha)
    #begin
        return 4
    else
        error("Unhandled case: ", f)
    end
end

function get_pixel_bit_size(f::E_SpecialFormats)
    if (f == SpecialFormats.r3_g3_b2)
        return 8
    elseif (f == SpecialFormats.r5_g6_b5)
        return 16
    elseif (f == SpecialFormats.rgb_shared_exp_ufloats)
        return 32
    elseif (f == SpecialFormats.rgb_tiny_ufloats)
        return 32
    elseif (f == SpecialFormats.sRGB)
        return 24
    elseif (f == SpecialFormats.rgb10_a2)
        return 32
    elseif (f == SpecialFormats.rgb10_a2_uint)
        return 32
    elseif (f == SpecialFormats.rgb5_a1)
        return 16
    elseif (f == SpecialFormats.sRGB_linear_alpha)
        return 32
    else
        error("Unhandled case: ", f)
    end
end

get_ogl_enum(f::E_SpecialFormats) = GLenum(f)


##############################
#      CompressedFormat      #
##############################

# All below formats are based on "block compression", where 4x4 blocks of pixels
#     are intelligently compressed together.
@bp_gl_enum(CompressedFormats::GLenum,
    # BC4 compression, with one color channel and a value range from 0 - 1.
    greyscale_normalized_uint = GL_COMPRESSED_RED_RGTC1,
    # BC4 compression, with one color channel and a value range from -1 to 1.
    greyscale_normalized_int = GL_COMPRESSED_SIGNED_RED_RGTC1,

    # BC5 compression, with two color channels and values range from 0 - 1.
    rg_normalized_uint = GL_COMPRESSED_RG_RGTC2,
    # BC5 compression, with two color channels and values range from -1 - 1.
    rg_normalized_int = GL_COMPRESSED_SIGNED_RG_RGTC2,

    # BC6 compression, with RGB color channels and floating-point values.
    rgb_float = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
    # BC6 compression, with RGB color channels and *unsigned* floating-point values.
    rgb_ufloat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
    # TODO: DXT1 for low-quality compression, with or without 1-bit alpha.

    # BC7 compression, with RGBA channels and values range from 0 - 1.
    rgba_normalized_uint = GL_COMPRESSED_RGBA_BPTC_UNORM,
    # BC7 compression, with RGBA channels and sRGB values ranging from 0 - 1.
    # "sRGB" meaning that the values get converted from sRGB space to linear space when sampled.
    rgba_sRGB_normalized_uint = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
);

"Gets the width/height/depth of each block of pixels in a block-compressed texture of the given format"
function get_block_size(format::E_CompressedFormats)
    # Currently, all Bplus-supported compressed formats
    #   use a block size of 4.
    return 4
end

"Gets the number of blocks along each axis for a block-compressed texture of the given size and format"
function get_block_count(format::E_CompressedFormats, tex_size::Vec{N, I}) where {N, I<:Integer}
    block_size::Int = get_block_size(format)
    # Round up to the next full block.
    return (size + (block_size - 1)) ÷ block_size
end

export CompressedFormats, E_CompressedFormats,
       get_block_size, get_block_count
#

function is_supported(::E_CompressedFormats, tex_type::E_TexTypes)::Bool
    # Currently, all Bplus-supported compressed formats
    #   only work with 2D textures.
    return (tex_type != TexTypes.twoD) && (tex_type != TexTypes.cube_map)
end

is_color(f::E_CompressedFormats) = true
is_depth_only(f::E_CompressedFormats) = false
is_stencil_only(f::E_CompressedFormats) = false
is_depth_and_stencil(f::E_CompressedFormats) = false

is_integer(f::E_CompressedFormats) = false
is_signed(f::E_CompressedFormats) = (
    if (f == CompressedFormats.greyscale_normalized_uint) ||
       (f == CompressedFormats.rg_normalized_uint) ||
       (f == CompressedFormats.rgb_ufloat) ||
       (f == CompressedFormats.rgba_normalized_uint) ||
       (f == CompressedFormats.rgba_sRGB_normalized_uint)
    #begin
        false
    elseif (f == CompressedFormats.greyscale_normalized_int) ||
           (f == CompressedFormats.rg_normalized_int) ||
           (f == CompressedFormats.rgb_float)
        true
    else
        error("Unhandled case: ", f)
    end
)

function stores_channel(f::E_CompressedFormats, c::E_ColorChannels)
    if (f == CompressedFormats.greyscale_normalized_uint) ||
       (f == CompressedFormats.greyscale_normalized_int)
    #begin
       return c == ColorChannels.red
    elseif (f == CompressedFormats.rg_normalized_uint) ||
           (f == CompressedFormats.rg_normalized_int)
    #begin
        return c <= ColorChannels.green
    elseif (f == CompressedFormats.rgb_float) ||
           (f == CompressedFormats.rgb_ufloat)
    #begin
        return c <= ColorChannels.blue
    elseif (f == CompressedFormats.rgba_normalized_uint) ||
           (f == CompressedFormats.rgba_sRGB_normalized_uint)
        return true
    else
        error("Unhandled case: ", f)
    end
end
stores_channel(f::E_CompressedFormats, c::E_OtherChannels) = false

function get_n_channels(f::E_CompressedFormats)
    if (f == CompressedFormats.greyscale_normalized_uint) ||
       (f == CompressedFormats.greyscale_normalized_int)
    #begin
       return 1
    elseif (f == CompressedFormats.rg_normalized_uint) ||
           (f == CompressedFormats.rg_normalized_int)
    #begin
        return 2
    elseif (f == CompressedFormats.rgb_float) ||
           (f == CompressedFormats.rgb_ufloat)
    #begin
        return 3
    elseif (f == CompressedFormats.rgba_normalized_uint) ||
           (f == CompressedFormats.rgba_sRGB_normalized_uint)
        return 4
    else
        error("Unhandled case: ", f)
    end
end

function get_pixel_bit_size(f::E_CompressedFormats)
    block_len::Int = get_block_size(f)
    n_block_bytes::Int =
        if (f == CompressedFormats.greyscale_normalized_uint) ||
           (f == CompressedFormats.greyscale_normalized_int)
            8
        elseif (f == CompressedFormats.rg_normalized_uint) ||
               (f == CompressedFormats.rg_normalized_int) ||
               (f == CompressedFormats.rgb_float) ||
               (f == CompressedFormats.rgb_ufloat) ||
               (f == CompressedFormats.rgba_normalized_uint) ||
               (f == CompressedFormats.rgba_sRGB_normalized_uint)
            16
        else
            error("Unhandled case: ", f)
        end
    return (n_block_bytes * 8) ÷ (block_len * block_len)
end

get_ogl_enum(f::E_CompressedFormats) = GLenum(f)


###################################
#      Depth/Stencil Formats      #
###################################

# Formats for depth and/or stencil textures.
@bp_gl_enum(DepthStencilFormats::GLenum,
    # Depth texture with unsigned 16-bit data.
    depth_16u = GL_DEPTH_COMPONENT16,
    # Depth texture with unsigned 24-bit data.
    depth_24u = GL_DEPTH_COMPONENT24,
    # Depth texture with unsigned 32-bit data.
    depth_32u = GL_DEPTH_COMPONENT32,
    # Depth texture with floating-point 32-bit data.
    depth_32f = GL_DEPTH_COMPONENT32F,

    # Stencil texture with unsigned 8-bit data.
    # Note that other sizes exist for stencil textures,
    #     but the OpenGL wiki strongly advises against using them.
    stencil_8 = GL_STENCIL_INDEX8,

    # Hybrid Depth/Stencil texture with unsigned 24-bit depth
    #     and unsigned 8-bit stencil.
    depth24u_stencil8 = GL_DEPTH24_STENCIL8,
    # Hybrid Depth/Stencil texture with floating-point 32-bit depth
    #     and unsigned 8-bit stencil (and 24 bits of padding in between them).
    depth32f_stencil8 = GL_DEPTH32F_STENCIL8
);

primitive type Depth24uStencil8u 32 end
"The data in each pixel of a `depth24u_stencil8` texture"
Depth24uStencil8u(u::UInt32) = reinterpret(Depth24uStencil8u, u)
Depth24uStencil8u(depth::UInt32, stencil::UInt8) = Depth24uStencil8u(
    (depth << 8) | stencil
)
Depth24uStencil8u(depth::UInt64, stencil::UInt8) = Depth24uStencil8u(UInt32(depth), stencil)
function Depth24uStencil8u(depth01::AbstractFloat, stencil::UInt8)
    max_uint24 = UInt32((2^24) - 1)
    return Depth24uStencil8u(
        UInt32(clamp(floor(depth01 * Float32(max_uint24)),
                           0, max_uint24)),
        stencil
    )
end
Base.zero(::Type{Depth24uStencil8u}) = Depth24uStencil8u(0x000000, 0x0)
export Depth24uStencil8u

primitive type Depth32fStencil8u 64 end
"The data in each pixel of a `depth32f_stencil8` texture"
Depth32fStencil8u(u::UInt64) = reinterpret(Depth32fStencil8u, u)
Depth32fStencil8u(depth::Float32, stencil::UInt8) = Depth32fStencil8u(
    (UInt64(reinterpret(UInt32, depth)) << 32) |
    stencil
)
Base.zero(::Type{Depth32fStencil8u}) = Depth32fStencil8u(Float32(0), 0x0)
export Depth32fStencil8u

"Pulls the depth and stencil values out of a packed texture pixel"
function Base.split(pixel::Depth24uStencil8u)::Tuple{UInt32, UInt8}
    u = reinterpret(UInt32, pixel)
    return (
        (u & 0xffffff00) >> 8,
        UInt8(u & 0xff)
    )
end
function Base.split(pixel::Depth32fStencil8u)::Tuple{Float32, UInt8}
    u = reinterpret(UInt64, pixel)
    return (
        Float32((u & 0xffffffff00000000) >> 32),
        UInt8(u & 0x00000000000000ff)
    )
end


export DepthStencilFormats, E_DepthStencilFormats,
       Depth24uStencil8u, Depth32fStencil8u
#

function is_supported(f::E_DepthStencilFormats, t::E_TexTypes)
    # Depth/stencil formats are not supported on 3D textures.
    return t != TexTypes.threeD
end

is_color(f::E_DepthStencilFormats) = false
is_depth_only(f::E_DepthStencilFormats) =
    if (f == DepthStencilFormats.depth_16u) ||
       (f == DepthStencilFormats.depth_24u) ||
       (f == DepthStencilFormats.depth_32u) ||
       (f == DepthStencilFormats.depth_32f)
        true
    elseif (f == DepthStencilFormats.depth24u_stencil8) ||
           (f == DepthStencilFormats.depth32f_stencil8) ||
           (f == DepthStencilFormats.stencil_8)
        false
    else
        error("Unhandled case: ", f)
    end
is_stencil_only(f::E_DepthStencilFormats) =
    if (f == DepthStencilFormats.stencil_8)
       true
    elseif (f == DepthStencilFormats.depth24u_stencil8) ||
           (f == DepthStencilFormats.depth32f_stencil8) ||
           (f == DepthStencilFormats.depth_16u) ||
           (f == DepthStencilFormats.depth_24u) ||
           (f == DepthStencilFormats.depth_32u) ||
           (f == DepthStencilFormats.depth_32f)
        false
    else
        error("Unhandled case: ", f)
    end
is_depth_and_stencil(f::E_DepthStencilFormats) =
    if (f == DepthStencilFormats.depth_16u) ||
       (f == DepthStencilFormats.depth_24u) ||
       (f == DepthStencilFormats.depth_32u) ||
       (f == DepthStencilFormats.depth_32f) ||
       (f == DepthStencilFormats.stencil_8)
        false
    elseif (f == DepthStencilFormats.depth24u_stencil8) ||
           (f == DepthStencilFormats.depth32f_stencil8)
        true
    else
        error("Unhandled case: ", f)
    end

is_integer(f::E_DepthStencilFormats) =
    if (f == DepthStencilFormats.stencil_8)
       true
    elseif (f == DepthStencilFormats.depth24u_stencil8) ||
           (f == DepthStencilFormats.depth32f_stencil8) ||
           (f == DepthStencilFormats.depth_16u) ||
           (f == DepthStencilFormats.depth_24u) ||
           (f == DepthStencilFormats.depth_32u) ||
           (f == DepthStencilFormats.depth_32f)
        false
    else
        error("Unhandled case: ", f)
    end

stores_channel(f::E_DepthStencilFormats, c::E_ColorChannels) = false
stores_channel(f::E_DepthStencilFormats, c::E_OtherChannels) =
    if (f == DepthStencilFormats.depth_16u) ||
       (f == DepthStencilFormats.depth_24u) ||
       (f == DepthStencilFormats.depth_32u) ||
       (f == DepthStencilFormats.depth_32f)
    #begin
        (c == OtherChannels.depth)
    elseif (f == DepthStencilFormats.stencil_8)
        (c == OtherChannels.stencil)
    elseif (f == DepthStencilFormats.depth24u_stencil8) ||
           (f == DepthStencilFormats.depth32f_stencil8)
        true
    else
        error("Unhandled case: ", f)
    end

get_n_channels(f::E_DepthStencilFormats) =
    if (f == DepthStencilFormats.depth_16u) ||
       (f == DepthStencilFormats.depth_24u) ||
       (f == DepthStencilFormats.depth_32u) ||
       (f == DepthStencilFormats.depth_32f) ||
       (f == DepthStencilFormats.stencil_8)
        1
    elseif (f == DepthStencilFormats.depth24u_stencil8) ||
           (f == DepthStencilFormats.depth32f_stencil8)
        2
    else
        error("Unhandled case: ", f)
    end

get_pixel_bit_size(f::E_DepthStencilFormats) =
    if (f == DepthStencilFormats.depth_16u)
        16
    elseif (f == DepthStencilFormats.depth_24u)
        24
    elseif (f == DepthStencilFormats.depth_32u) ||
           (f == DepthStencilFormats.depth_32f)
        32
    elseif (f == DepthStencilFormats.depth24u_stencil8) ||
           (f == DepthStencilFormats.depth32f_stencil8)
        64
    else
        error("Unhandled case: ", f)
    end

get_ogl_enum(f::E_DepthStencilFormats) = GLenum(f)


#######################
#      TexFormat      #
#######################

"Describes a pixel format for any kind of texture"
const TexFormat = Union{SimpleFormat, E_SpecialFormats, E_CompressedFormats, E_DepthStencilFormats}
export TexFormat