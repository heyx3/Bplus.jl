# Define the handles to various OpenGL objects.
# Most define an "emmpty" constructor which creates a null-pointer,
#    a.k.a. a value which OpenGL promises will not be a valid handle.
# They also provide a `gl_type(::Type)` that retrieves their underlying integer data type.

"""
Defines a primitive type representing an OpenGL handle.
Think of this like a stronger version of type aliasing,
   which provides more type safety.
The type will also have an empty constructor which creates a "null" value
   (usually 0 or -1).
"""
macro ogl_handle(name::Symbol, gl_type_name,
                 display_name = lowercase(string(name)),
                 null_val = zero(eval(gl_type_name))) #TODO: Eliminate eval by using `:( zero($gl_type_name) )`
    gl_type = eval(gl_type_name)
    @bp_gl_assert(gl_type <: Integer,
                  "Unexpected GL type: ", gl_type, ".",
                  " I only really planned for integers in this macro")

    type_name = esc(Symbol(:Ptr_, name))
    null_val = esc(null_val)
    gl_type_name = esc(gl_type_name)
    hash_extra = hash(name)
    return quote
        Base.@__doc__(
            primitive type $type_name (8*sizeof($gl_type)) end
        )
        $type_name() = reinterpret($type_name, $gl_type($null_val))
        $type_name(i::$gl_type) = reinterpret($type_name, i)
        $type_name(i::Integer) = $type_name(convert($gl_type, i))
        $gl_type_name(i::$type_name) = reinterpret($gl_type_name, i)
        $(esc(:gl_type))(x::$type_name) = $gl_type_name(x)
        $(esc(:gl_type))(::Type{$type_name}) = $gl_type_name
        Base.show(io::IO, x::$type_name) = print(io, $display_name, '<', Int($gl_type_name(x)), '>')
        Base.print(io::IO, x::$type_name) = print(io, Int($gl_type_name(x)))
        Base.convert(::Type{$gl_type_name}, i::$type_name) = reinterpret($gl_type, i)
        Base.unsafe_convert(::Type{Ptr{$gl_type}}, r::Base.RefValue{$type_name}) =
            Base.unsafe_convert(Ptr{$gl_type}, Base.unsafe_convert(Ptr{Nothing}, r))
        Base.hash(h::$type_name, u::UInt) = hash(tuple(gl_type(u), $hash_extra), u)
        Base.:(==)(a::$type_name, b::$type_name) = (gl_type(a) == gl_type(b))
    end
end

@ogl_handle Program GLuint
@ogl_handle Uniform GLint "uniform" -1
@ogl_handle ShaderBuffer GLuint "shaderBuffer" GL_INVALID_INDEX

@ogl_handle Texture GLuint
@ogl_handle Image GLuint
@ogl_handle View GLuint64
@ogl_handle Sampler GLuint

"Equivalent to an OpenGL 'Framebuffer'"
@ogl_handle Target GLuint
"Equivalent to an OpenGL 'RenderBuffer'"
@ogl_handle TargetBuffer GLuint

@ogl_handle Buffer GLuint
@ogl_handle Mesh GLuint


# Some handle types benefit from supporting pointer arithmetic.
const POINTER_ARITHMETIC_HANDLES = tuple(
    Ptr_Uniform, Ptr_ShaderBuffer
)
for TP in POINTER_ARITHMETIC_HANDLES
    for op in (:+, :-, :*, :÷)
        @eval begin
            Base.$op(p::$TP, i::Integer) = $TP(
                gl_type($TP)(
                    $op(gl_type(p), gl_type($TP)(i))
                )
            )
            Base.$op(i::Integer, p::$TP) = $TP(
                gl_type($TP)(
                    $op(gl_type($TP)(i), gl_type(p))
                )
            )
        end
    end
end