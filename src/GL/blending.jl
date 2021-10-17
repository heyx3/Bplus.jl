#=
When applying a pixel to the screen, a math equation is used
   to combine its color with that of the existing screen pixel.
For example, a transparent pixel (i.e. one with Alpha less than 1)
   should let some of the existing color come through.
On the other hand, an opaque pixel should wipe out the old color completely.

The new incoming color is called "source" or "src",
   and the existing color is called "destination" or "dest".
The blend equation looks like this:
   `new_dest = (src_factor * src) [op] (dest_factor * dest)`
The configurable parts are src_factor, dest_factor, and the op that combines the values.

Opaque blending would use src_factor=`One`, dest_factor=`Zero`, op=`Add`.
Alpha blending would use src_factor=`SrcAlpha`, dest_factor=`OneMinusSrcAlpha`, op=`Add`.
There are much weirder blend modes (think of the variety available in Photoshop).

Shaders provide unlimited flexibility to combine two textures into a new one,
   but "blending" is built into the GPU hardware and therefore likely to be very fast.
=#

# The multipliers that can be used on 'src' and 'dest' colors.
@bp_gl_enum(BlendFactors::ModernGL.GLenum,
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