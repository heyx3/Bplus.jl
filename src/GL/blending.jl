#=
When applying a pixel to the screen, a math equation is used
   to combine its color with that of the existing screen pixel.
For example, a transparent pixel (i.e. one with Alpha less than 1)
   should let some of the existing color come through.
On the other hand, an opaque pixel should wipe out the old color completely.

The new incoming color is called "source" or "src",
   and the existing color is called "destination" or "dest".
The blend equation looks like this:
   `new_dest = op((src_factor * src), (dest_factor * dest))`
The configurable parts are src_factor, dest_factor, and op.

Opaque blending would use src_factor=`One`, dest_factor=`Zero`, op=`Add`.
Alpha blending would use src_factor=`SrcAlpha`, dest_factor=`OneMinusSrcAlpha`, op=`Add`.
There are much weirder blend modes (think of the variety available in Photoshop).

Shaders provide unlimited flexibility to combine two textures into a new one,
   but "blending" is built into the hardware and likely to be very fast.
=#


# The multipliers that can be used on 'src' and 'dest' colors.
@bp_gl_enum(BlendFactors::GLenum,
    Zero = GL_ZERO,
    One = GL_ONE,

    SrcColor = GL_SRC_COLOR,
    SrcAlpha = GL_SRC_ALPHA,

    InverseSrcColor = GL_ONE_MINUS_SRC_COLOR,
    InverseSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,

    DestColor = GL_DST_COLOR,
    DestAlpha = GL_DST_ALPHA,

    InverseDestColor = GL_ONE_MINUS_DST_COLOR,
    InverseDestAlpha = GL_ONE_MINUS_DST_ALPHA,

    # The below factors are NOT multipliers on the src/dest value.
    # Instead, they replace the src/dest with a constant.
    ConstantColor = GL_CONSTANT_COLOR,
    ConstantAlpha = GL_CONSTANT_ALPHA,
    InverseConstantColor = GL_ONE_MINUS_CONSTANT_COLOR,
    InverseConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA
)
export BlendFactors, E_BlendFactors

"
Gets whether the given type of blend factor uses a 'Constant' value
  instead of the actual src/dest value.
"
blend_factor_uses_constant(f::E_BlendFactors)::Bool = (
    (f == BlendFactors.ConstantColor) |
    (f == BlendFactors.ConstantAlpha) |
    (f == BlendFactors.InverseConstantColor) |
    (f == BlendFactors.InverseConstantAlpha)
)
export blend_factor_uses_constant


# The operators that can combine source and destination colors
#   (after each is multiplied by their blend factor).
@bp_gl_enum(BlendOps::GLenum,
    Add = GL_FUNC_ADD,

    Subtract = GL_FUNC_SUBTRACT,
    ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,

    Min = GL_MIN,
    Max = GL_MAX
)
export BlendOps, E_BlendOps

"
A way of blending new rendered pixels onto an existing image.
The `T` type parameter determines the type of the blend data,
   which is important when using a 'Constant' blend factor.
"
struct BlendState_{T}
    src::E_BlendFactors
    dest::E_BlendFactors
    op::E_BlendOps

    # Only used for 'constant'-mode blend operations.
    constant::T

    BlendState_{T}(src, dest, op, constant = zero(T)) where {T} = new{T}(src, dest, op, constant)
    BlendState_(src, dest, op, constant::T) where {T} = new{T}(src, dest, op, constant)
end
const BlendStateRGB = BlendState_{vRGBf}
const BlendStateAlpha = BlendState_{Float32}
const BlendStateRGBA = BlendState_{vRGBAf}
export BlendStateRGB, BlendStateAlpha,
       BlendStateRGBA

"
Gets whether the given blend mode uses a 'Constant' value
  instead of an actual src/dest value.
"
@inline blend_uses_constant(b::BlendState_)::Bool = (
    blend_factor_uses_constant(b.src) |
    blend_factor_uses_constant(b.dest)
)
export blend_uses_constant

"Makes an opaque blend mode of the given type"
make_blend_opaque(::Type{BlendState_{T}}) where {T} = BlendState_{T}(
    BlendFactors.One, BlendFactors.Zero,
    BlendOps.Add
)
"Makes an alpha-blend mode of the given type"
make_blend_alpha(::Type{BlendState_{T}}) where {T} = BlendState_{T}(
    BlendFactors.SrcAlpha, BlendFactors.InverseSrcAlpha,
    BlendOps.Add
)
"Makes an additive blend mode"
make_blend_additive(::Type{BlendState_{T}}) where {T} = BlendState_{T}(
    BlendFactors.One, BlendFactors.One,
    BlendOps.Add
)
export make_blend_opaque, make_blend_alpha, make_blend_additive