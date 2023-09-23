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
    zero = GL_ZERO,
    one = GL_ONE,

    src_color = GL_SRC_COLOR,
    src_alpha = GL_SRC_ALPHA,

    inverse_src_color = GL_ONE_MINUS_SRC_COLOR,
    inverse_src_alpha = GL_ONE_MINUS_SRC_ALPHA,

    dest_color = GL_DST_COLOR,
    dest_alpha = GL_DST_ALPHA,

    inverse_dest_color = GL_ONE_MINUS_DST_COLOR,
    inverse_dest_alpha = GL_ONE_MINUS_DST_ALPHA,

    # The below factors are NOT multipliers on the src/dest value.
    # Instead, they replace the src/dest with a constant.
    constant_color = GL_CONSTANT_COLOR,
    constant_alpha = GL_CONSTANT_ALPHA,
    inverse_constant_color = GL_ONE_MINUS_CONSTANT_COLOR,
    inverse_constant_alpha = GL_ONE_MINUS_CONSTANT_ALPHA
)
export BlendFactors, E_BlendFactors

"
Gets whether the given type of blend factor uses a 'Constant' value
  instead of the actual src/dest value.
"
blend_factor_uses_constant(f::E_BlendFactors)::Bool = (
    (f == BlendFactors.constant_color) |
    (f == BlendFactors.constant_alpha) |
    (f == BlendFactors.inverse_constant_color) |
    (f == BlendFactors.inverse_constant_alpha)
)
export blend_factor_uses_constant


# The operators that can combine source and destination colors
#   (after each is multiplied by their blend factor).
@bp_gl_enum(BlendOps::GLenum,
    add = GL_FUNC_ADD,

    subtract = GL_FUNC_SUBTRACT,
    reverse_subtract = GL_FUNC_REVERSE_SUBTRACT,

    min = GL_MIN,
    max = GL_MAX
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
make_blend_opaque(T::Type{<:BlendState_}) = T(
    BlendFactors.one, BlendFactors.zero,
    BlendOps.add
)
"Makes an alpha-blend mode of the given type"
make_blend_alpha(T::Type{<:BlendState_}) = T(
    BlendFactors.src_alpha, BlendFactors.inverse_src_alpha,
    BlendOps.add
)
"Makes an additive blend mode"
make_blend_additive(T::Type{<:BlendState_}) = T(
    BlendFactors.one, BlendFactors.one,
    BlendOps.add
)
export make_blend_opaque, make_blend_alpha, make_blend_additive