# Define @bp_gl_assert:
@make_toggleable_asserts bp_gl_


"Short-hand for making enums based on OpenGL constants"
macro bp_gl_enum(name, args...)
    expr = Utilities.generate_enum(name, :(begin using ModernGLbp end), args, false)
    return expr
end
"Short-hand for making bitfields based on OpenGL constants"
macro bp_gl_bitfield(name, args...)
  expr = Utilities.generate_enum(name, :(begin using ModernGLbp end), args, true)
end

"
Helper function that calls OpenGL, reads N elements of data into a buffer,
  then returns that buffer as an NTuple.
"
function get_from_ogl( output_type::DataType,
                       output_count::Int,
                       gl_func::Function,
                       func_args...
                     )::NTuple{output_count, output_type}
    ref = Ref(ntuple(i->zero(output_type), output_count))
    gl_func(func_args..., ref)
    return ref[]
end
"Helper function that calls OpenGL, reads some kind of data, then returns that data"
get_from_ogl(output_type::DataType, gl_func::Function, func_args...)::output_type = get_from_ogl(output_type, 1, gl_func, func_args...)[1]