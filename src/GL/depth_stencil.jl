#=
The depth buffer is used to keep track of how close each rendered pixel is to the camera.
As new pixels are rendered, any that are behind the depth buffer are discarded,
  and any that are in front of the depth buffer will be applied and update the buffer's value.
This default behavior can be configured in a number of ways.

The stencil buffer is a similar kind of filter, but with integer data and bitwise operations
   instead of floating-point depth values.
=#


#=
Types of comparisons that can be used during depth/stencil testing.
The depth/stencil value of a newly-rendered fragment is tested against the existing value
   using one of these comparisons.
=#
@bp_gl_enum(ValueTests::GLenum,
    # The test always passes. Note that this does NOT disable depth writes.
    Off      = GL_ALWAYS,
    # The test always fails.
    Never    = GL_NEVER,

    # The test passes if the new value is less than the "test" value.
    LessThan = GL_LESS,
    # The test passes if the new value is less than or equal to the "test" value.
    LessThanOrEqual = GL_LEQUAL,

    # The test passes if the new value is greater than the "test" value.
    GreaterThan = GL_GREATER,
    # The test passes if the new value is greater than or equal to the "test" value.
    GreaterThanOrEqual = GL_GEQUAL,

    # The test passes if the new value is equal to the "test" value.
    Equal = GL_EQUAL,
    # The test passes if the new value is not equal to the "test" value.
    NotEqual = GL_NOTEQUAL
)

#=
The various actions that can be performed on a stencil buffer pixel,
   based on a new fragment that was just drawn over it.
=#
@bp_gl_enum(StencilOps::GLenum,
    # Don't do anything.
    Nothing = GL_KEEP,

    # Set the value to 0.
    Zero = GL_ZERO,
    # Replace the stencil buffer's value with the incoming fragment's value.
    Replace = GL_REPLACE,
    # Flip all bits in the buffer (a.k.a. bitwise NOT).
    Invert = GL_INVERT,

    # Increment the stencil buffer's value, clamping it to stay inside its range.
    IncrementClamp = GL_INCR,
    # Increment the stencil buffer's value, wrapping around to 0 if it passes the max value.
    IncrementWrap = GL_INCR_WRAP,

    # Decrement the stencil buffer's value, clamping it to stay inside its range.
    DecrementClamp = GL_DECR,
    # Decrement the stencil buffer's value, wrapping around to the max value if it passes below 0.
    DecrementWrap = GL_DECR_WRAP,
)

