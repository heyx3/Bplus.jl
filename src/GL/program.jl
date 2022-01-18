#TODO: Port in the "ShaderIncludeFromFiles" stuff?


######################
#      Uniforms      #
######################

# Uniforms are shader parameters, set by the user through OpenGL calls.


const UniformScalar = Union{Int32, UInt32, Bool, Float32, Float64}
const UniformVector{N} = @unionspec Vec{N, _} Int32 UInt32 Bool Float32 Float64
const UniformMatrix{C, R, L} = @unionspec Mat{C, R, _, L} Float32 Float64


"Acceptable types of uniforms"
const Uniform = Union{UniformScalar,
                      @unionspec(UniformVector{_}, 1, 2, 3, 4),
                      # All Float32 and Float64 matrices from 2x2 to 4x4:
                      map(size -> UniformMatrix{size..., prod(size)},
                           tuple(Iterators.product(2:4, 2:4)...)
                      )...,
                      # "Opaque" types:
                      Ptr_Buffer,  #TODO: Use Buffer instead of the raw pointer, and decorate them with SSBO-vs-UBO and binding-index.
                      Texture, Ptr_View, View #TODO: Also support the old-school texture, decorated with the texture-unit index to put it in.
                     }


export Uniform


"
Information about a specific Program uniform.
A uniform can be an array of its data type; if the uniform is an array of structs,
    each UniformData instance will be about one specific field of one specific element.
"
struct UniformData
    handle::Ptr_Uniform
    type::Type{<:Uniform}
    array_size::Int  # Set to 1 for non-array uniforms.
end

"""
`set_uniform(::Program, ::String, ::T[, array_index::Int])`
`set_uniforms(::Program, ::String, ::Contiguous{T}[, dest_offset=0])`

Sets a program's uniform to the given value.

If the uniform is an array, you must provide a contiguous array of values, or a (1-based) index.

If the uniform is a struct or array of structs, you must name the individual fields/elements,
    e.x. `set_uniform(p, "my_lights[0].pos", new_pos)`
"""
function set_uniform end
"""
`set_uniform(::Program, ::String, ::T[, array_index::Int])`
`set_uniforms(::Program, ::String, ::AbstractVector{T}[, offset=0])`

Sets a program's uniform to the given value.

If the uniform is an array, you must provide an array of values, or a (1-based) index.

If the uniform is a struct or array of structs, you must name the individual fields/elements,
    e.x. `set_uniform(p, "my_lights[0].pos", new_pos)`
"""
function set_uniforms end


export Uniform, UniformData, set_uniform, set_uniforms


################################
#      PreCompiledProgram      #
################################

# A binary blob representing a previously-compiled shader.
# You can usually cache these to avoid recompiling shaders.
mutable struct PreCompiledProgram
    header::GLenum
    data::Vector{UInt8}
end

export PreCompiledProgram


#############################
#      compile_stage()      #
#############################

"
Internal helper that compiles a single stage of a shader program.
Returns the program's handle, or an error message.
"
function compile_stage( name::String,
                        type::GLenum,
                        source::String
                      )::Union{GLuint, String}
    source = string(GLSL_HEADER, "\n", source)
    handle::GLuint = glCreateShader(type)

    # The OpenGL call wants an array of strings, a.k.a. a pointer to a pointer to a string.
    # ModernGL provides an example of how to do this with Ptr,
    #   but ideally I'd like to know how to do it with Ref.
    source_array = [ convert(Ptr{GLchar}, pointer(source)) ]
    source_array_ptr = convert(Ptr{UInt8}, pointer(source_array))

    glShaderSource(handle, 1, source_array_ptr, C_NULL)
    glCompileShader(handle)

    if get_from_ogl(GLint, glGetShaderiv, handle, GL_COMPILE_STATUS) == GL_TRUE
        return handle
    else
        err_msg_len = get_from_ogl(GLint, glGetShaderiv, handle, GL_INFO_LOG_LENGTH)
        err_msg_data = Vector{UInt8}(undef, err_msg_len)
        glGetShaderInfoLog(handle, err_msg_len, Ref(GLsizei(err_msg_len)), Ref(err_msg_data, 1))

        return string("Error compiling ", name, ": ",
                      String(view(err_msg_data, 1:err_msg_len)))
    end
end


#############################
#      ProgramCompiler      #
#############################

"A set of data to be compiled into an OpenGL shader program"
mutable struct ProgramCompiler
    src_vertex::String
    src_fragment::String
    src_geometry::Optional{String}
    #TODO: also support compute shaders

    # A pre-compiled version of this shader which this compiler can attempt to use first.
    # The shader source code is still needed as a fallback,
    #    as shaders may need recompilation after driver changes.
    # After compilation, if the user wants, this field can be updated with the newest binary.
    cached_binary::Optional{PreCompiledProgram}
end

ProgramCompiler( src_vertex, src_fragment
                 ;
                 src_geometry = nothing,
                 cached_binary = nothing
               ) = ProgramCompiler(
    src_vertex, src_fragment,
    src_geometry,
    cached_binary
)

"
Run the given compile job.
Returns the new program's handle, or a compile error message.
Also optionally updates the compiler 'cached_binary' field to contain
    the program's up-to-date binary blob.
"
function compile_program( p::ProgramCompiler,
                          update_cache::Bool = false
                        )::Union{Ptr_Program, String}
    @bp_check(exists(get_context()), "Can't create a Program outside a Bplus.GL.Context")

    out_ptr = Ptr_Program(glCreateProgram())

    # Try to use the pre-compiled binary blob.
    if exists(p.cached_binary)
        glProgramBinary(out_ptr, p.cached_binary.header,
                        Ref(p.cached_binary.data, 1),
                        length(p.cached_binary.data))

        if get_from_ogl(GLint, glGetProgramiv, GL_LINK_STATUS) == GL_TRUE
            return out_ptr
        end
    end

    # Compile the individual shaders.
    compiled_handles::Vector{GLuint} = [ ]
    for data in (("vertex", GL_VERTEX_SHADER, p.src_vertex),
                 ("fragment", GL_FRAGMENT_SHADER, p.src_fragment),
                 ("geometry", GL_GEOMETRY_SHADER, p.src_geometry))
        if exists(data[3])
            result = compile_stage(data...)
            if result isa String
                # Clean up the shaders/program, then return the error message.
                map(glDeleteShader, compiled_handles)
                glDeleteProgram(out_ptr)
                return result
            else
                push!(compiled_handles, result)
            end
        end
    end

    # Link the shader program together.
    for shad_ptr in compiled_handles
        glAttachShader(out_ptr, shad_ptr)
    end
    glLinkProgram(out_ptr)
    map(glDeleteShader, compiled_handles)

    # Check for link errors.
    if get_from_ogl(GLint, glGetProgramiv, out_ptr, GL_LINK_STATUS) == GL_FALSE
        msg_len = get_from_ogl(GLint, glGetProgramiv, out_ptr, GL_INFO_LOG_LENGTH)
        msg_data = Vector{UInt8}(undef, msg_len)
        glGetProgramInfoLog(out_ptr, msg_len, Ref(Int(msg_len)), Ref(msg_data, 1))

        glDeleteProgram(out_ptr)
        return string("Error combining shaders: ", String(msg_data[1:msg_len]))
    end

    # We need to "detach" the shader objects
    #    from the main program object, so that they can be cleaned up.
    for handle in compiled_handles
        glDetachShader(out_ptr, handle)
    end

    # Update the cached compiled program.
    if update_cache
        byte_size = get_from_ogl(GLint, glGetProgramiv, GL_PROGRAM_BINARY_LENGTH)
        @bp_gl_assert(byte_size > 0,
                      "Didn't return an error message earlier, yet compilation failed?")

        # Allocate space in the cache to hold the program binary.
        if exists(p.cached_binary)
            resize!(p.cached_binary, byte_size)
        else
            p.cached_binary = PreCompiledProgram(zero(GLenum), Vector{UInt8}(undef, byte_size))
        end

        glGetProgramBinary(program, byte_size,
                           Ref(C_NULL),
                           Ref(p.cached_binary.header),
                           Ref(p.cached_binary.data, 1))

    end

    return out_ptr
end

export ProgramCompiler, compile_program


######################
#       Program      #
######################

"A compiled group of OpenGL shaders (vertex, fragment, etc.)"
mutable struct Program <: Resource
    handle::Ptr_Program
    uniforms::Dict{String, UniformData}
end

function Program(vert_shader::String, frag_shader::String
                 ;
                 geom_shader::Optional{String} = nothing)
    compiler = ProgramCompiler(vert_shader, frag_shader; src_geometry = geom_shader)
    result = compile_program(compiler)
    if result isa Ptr_Program
        return Program(result)
    else
        error(result)
    end
end
function Program(handle::Ptr_Program)
    # Get the uniforms.
    uniforms = Dict{String, UniformData}()
    glu_array_count = Ref{GLint}()  # The length of this uniform array (1 if it's not an array).
    glu_type = Ref{GLenum}()
    glu_name_buf = Vector{UInt8}(
                           undef,
                           get_from_ogl(GLint, glGetProgramiv,
                                        handle, GL_ACTIVE_UNIFORM_MAX_LENGTH)
                   )
    glu_name_len = Ref{GLsizei}()
    for i in 1:get_from_ogl(GLint, glGetProgramiv, handle, GL_ACTIVE_UNIFORMS)
        glGetActiveUniform(handle, i - 1,
                           length(glu_name_buf), glu_name_len,
                           glu_array_count,
                           glu_type,
                           Ref(glu_name_buf, 1))
        u_name_buf = glu_name_buf[1:glu_name_len[]]
        u_name = String(u_name_buf)

        # Array uniforms come in with the suffix "[0]".
        # Remove that to keep things simple to the user.
        if endswith(u_name, "[0]")
            u_name = u_name[1:(end-3)]
        end

        @bp_gl_assert(!haskey(uniforms, u_name))
        uniforms[u_name] = UniformData(
            Ptr_Uniform(glGetUniformLocation(handle, Ref(glu_name_buf, 1))),
            UNIFORM_TYPE_FROM_GL_ENUM[glu_type[]],
            glu_array_count[]
        )
    end

    return Program(handle, uniforms)
end

function Base.close(p::Program)
    @bp_check(p.handle != Ptr_Program(), "Already closed this Program")
    glDeleteProgram(p.handle)
    setfield!(p, :handle, Ptr_Program())
end

"Gets the Julia type from a GLenum value representing a uniform type"
const UNIFORM_TYPE_FROM_GL_ENUM = Dict{GLenum, Type{<:Uniform}}(
    GL_FLOAT => Float32,
    GL_FLOAT_VEC2 => v2f,
    GL_FLOAT_VEC3 => v3f,
    GL_FLOAT_VEC4 => v4f,
    GL_DOUBLE => Float64,
    GL_DOUBLE_VEC2 => v2d,
    GL_DOUBLE_VEC3 => v3d,
    GL_DOUBLE_VEC4 => v4d,

    GL_INT => Int32,
    GL_INT_VEC2 => Vec{2, Int32},
    GL_INT_VEC3 => Vec{3, Int32},
    GL_INT_VEC4 => Vec{4, Int32},
    GL_UNSIGNED_INT => UInt32,
    GL_UNSIGNED_INT_VEC2 => Vec{2, UInt32},
    GL_UNSIGNED_INT_VEC3 => Vec{3, UInt32},
    GL_UNSIGNED_INT_VEC4 => Vec{4, UInt32},

    GL_BOOL => Bool,
    GL_BOOL_VEC2 => v2b,
    GL_BOOL_VEC3 => v3b,
    GL_BOOL_VEC4 => v4b,

    GL_FLOAT_MAT2 => fmat2,
    GL_FLOAT_MAT3 => fmat3,
    GL_FLOAT_MAT4 => fmat4,
    GL_FLOAT_MAT2x3 => fmat2x3,
    GL_FLOAT_MAT2x4 => fmat2x4,
    GL_FLOAT_MAT3x2 => fmat3x2,
    GL_FLOAT_MAT3x4 => fmat3x4,
    GL_FLOAT_MAT4x2 => fmat4x2,
    GL_FLOAT_MAT4x3 => fmat4x3,
    GL_DOUBLE_MAT2 => dmat2,
    GL_DOUBLE_MAT3 => dmat3,
    GL_DOUBLE_MAT4 => dmat4,
    GL_DOUBLE_MAT2x3 => dmat2x3,
    GL_DOUBLE_MAT2x4 => dmat2x4,
    GL_DOUBLE_MAT3x2 => dmat3x2,
    GL_DOUBLE_MAT3x4 => dmat3x4,
    GL_DOUBLE_MAT4x2 => dmat4x2,
    GL_DOUBLE_MAT4x3 => dmat4x3,

    #TODO: support 'atomic counter' uniforms.  #GL_UNSIGNED_INT_ATOMIC_COUNTER => atomic_uint,

    GL_SAMPLER_1D => Ptr_View,
    GL_SAMPLER_2D => Ptr_View,
    GL_SAMPLER_3D => Ptr_View,
    GL_SAMPLER_CUBE => Ptr_View,
    GL_SAMPLER_1D_SHADOW => Ptr_View,
    GL_SAMPLER_2D_SHADOW => Ptr_View,
    GL_SAMPLER_1D_ARRAY => Ptr_View,
    GL_SAMPLER_2D_ARRAY => Ptr_View,
    GL_SAMPLER_1D_ARRAY_SHADOW => Ptr_View,
    GL_SAMPLER_2D_ARRAY_SHADOW => Ptr_View,
    GL_SAMPLER_2D_MULTISAMPLE => Ptr_View,
    GL_SAMPLER_2D_MULTISAMPLE_ARRAY => Ptr_View,
    GL_SAMPLER_CUBE_SHADOW => Ptr_View,
    GL_SAMPLER_BUFFER => Ptr_View,
    GL_SAMPLER_2D_RECT => Ptr_View,
    GL_SAMPLER_2D_RECT_SHADOW => Ptr_View,
    GL_INT_SAMPLER_1D => Ptr_View,
    GL_INT_SAMPLER_2D => Ptr_View,
    GL_INT_SAMPLER_3D => Ptr_View,
    GL_INT_SAMPLER_CUBE => Ptr_View,
    GL_INT_SAMPLER_1D_ARRAY => Ptr_View,
    GL_INT_SAMPLER_2D_ARRAY => Ptr_View,
    GL_INT_SAMPLER_2D_MULTISAMPLE => Ptr_View,
    GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY => Ptr_View,
    GL_INT_SAMPLER_BUFFER => Ptr_View,
    GL_INT_SAMPLER_2D_RECT => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_1D => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_2D => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_3D => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_CUBE => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_1D_ARRAY => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_2D_ARRAY => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_BUFFER => Ptr_View,
    GL_UNSIGNED_INT_SAMPLER_2D_RECT => Ptr_View,
    GL_IMAGE_1D => Ptr_View,
    GL_IMAGE_2D => Ptr_View,
    GL_IMAGE_3D => Ptr_View,
    GL_IMAGE_2D_RECT => Ptr_View,
    GL_IMAGE_CUBE => Ptr_View,
    GL_IMAGE_BUFFER => Ptr_View,
    GL_IMAGE_1D_ARRAY => Ptr_View,
    GL_IMAGE_2D_ARRAY => Ptr_View,
    GL_IMAGE_2D_MULTISAMPLE => Ptr_View,
    GL_IMAGE_2D_MULTISAMPLE_ARRAY => Ptr_View,
    GL_INT_IMAGE_1D => Ptr_View,
    GL_INT_IMAGE_2D => Ptr_View,
    GL_INT_IMAGE_3D => Ptr_View,
    GL_INT_IMAGE_2D_RECT => Ptr_View,
    GL_INT_IMAGE_CUBE => Ptr_View,
    GL_INT_IMAGE_BUFFER => Ptr_View,
    GL_INT_IMAGE_1D_ARRAY => Ptr_View,
    GL_INT_IMAGE_2D_ARRAY => Ptr_View,
    GL_INT_IMAGE_2D_MULTISAMPLE => Ptr_View,
    GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_1D => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_2D => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_3D => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_2D_RECT => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_CUBE => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_BUFFER => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_1D_ARRAY => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_2D_ARRAY => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE => Ptr_View,
    GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY => Ptr_View,
)


export Program


#################################
#     Setting uniforms Impl     #
#################################

get_uniform(program::Program, name::String) =
    if haskey(program.uniforms, name)
        program.uniforms[name]
    else
        error("Uniform not found (was it optimized out?): '", name, "'")
    end
#


function set_uniform(program::Program, name::String, value::T) where {T}
    u_data::UniformData = get_uniform(program, name)
    @bp_check(u_data.array_size == 1, "No index given for the array uniform '", name, "'")
    if T <: Vec
        set_uniform(u_data.type, T, Ref(value.data), program.handle, u_data.handle, 1)
    elseif T <: Union{Texture, View}
        set_uniform(u_data.type, T, value, program.handle, u_data.handle, 1)
    else
        set_uniform(u_data.type, T, Ref(value), program.handle, u_data.handle, 1)
    end
end

function set_uniform( program::Program, name::String,
                      value::T, index::Int
                    ) where {T<:Uniform}
    u_data::UniformData = get_uniform(program, name)

    @bp_check(index <= u_data.array_size,
              "Array uniform '", name, "' only has ", u_data.array_size, " elements,",
                " and you're trying to set index ", index)
    handle = Ptr_Uniform(gl_type(u_data.handle) + (index - 1))

    if T <: Vec
        set_uniform(u_data.type, T, Ref(value.data), program.handle, handle, 1)
    elseif T <: Union{Texture, View}
        set_uniform(u_data.type, T, value, program.handle, handle, 1)
    else
        set_uniform(u_data.type, T, Ref(value), program.handle, handle, 1)
    end
end
function set_uniforms( program::Program, name::String,
                       values::TVector,
                       offset::Integer = 0
                     ) where {T<:Uniform, TVector<:Contiguous{T}}
    u_data::UniformData = get_uniform(program, name)

    @bp_check(length(values) + offset <= u_data.array_size,
                "Array uniform '", name, "' only has ", u_data.array_size, " elements,",
                " and you're trying to set elements ", (offset+1), ":", (offset+length(values)))
    handle = Ptr_Uniform(gl_type(u_data.handle) + offset)

    count::Int = min(length(values), u_data.array_size - offset)

    if T <: Union{Texture, View}
        set_uniform(u_data.type, T, values, program.handle, handle, count)
    else
        set_uniform(u_data.type, T, Ref(values, 1), program.handle, handle, count)
    end
end


# Below are the actual implementations, calling through to OpenGL functions.

function set_uniform( ::Type{Ptr_View}, ::Type{<:Union{Texture, View}},
                      data::TexData,
                      program::Ptr_Program,
                      first_param::Ptr_Uniform,
                      n_params::Integer
                    ) where {TexData<:Union{Texture, View, AbstractVector{<:Union{Texture, View}}}}
    # We need to convert each texture/view into a handle.
    if TexData isa AbstractVector
        #TODO: Use some kind of pooling for the temp array.
        handles::Vector{gl_type(Ptr_View)} = map(data) do element::Union{Texture, View}
            if element isa Texture
                return gl_type(get_view(element).handle)
            elseif element isa View
                return gl_type(element.handle)
            else
                error("Unhandled case: ", typeof(element))
            end
        end
        glProgramUniformHandleui64vARB(program, first_param, n_params, Ref(handles, 1))
    elseif TexData <: Union{Texture, View}
        @bp_gl_assert(n_params == 1, "Trying to set ", n_params, " params with a single texture")
        view::View = (TexData isa View) ? data : get_view(data)
        glProgramUniformHandleui64ARB(program, first_param, gl_type(view.handle))
    else
        error("Unhandled case: ", TexData)
    end
end

@generated function set_uniform( ::Type{OutT}, ::Type{InT},
                                 data_ptr::TRef, program::Ptr_Program,
                                 first_param::Ptr_Uniform, n_params::Integer
                               ) where {InT, OutT<:Uniform, TRef<:Ref}
    if OutT == Ptr_View
        # Any input type other than raw Ptr_View's
        #    should have been handled by other function overloads.
        @bp_check(InT <: Union{Ptr_View, Contiguous{Ptr_View}},
                  "Can't set a texture uniform with a ", InT,
                     " using ref-type ", TRef)
        return :( glProgramUniformHandleui64vARB(program, first_param, n_params, data_ptr) )
    elseif OutT <: UniformScalar
        @bp_check(InT <: Union{OutT, Vec{1, OutT}},
                  "Uniform type is a scalar ", OutT, ", but you're trying to set a ", InT)
        gl_func_name = Symbol(:glProgramUniform1, get_uniform_ogl_letter(OutT), :v)
        return :( $gl_func_name(program, first_param, n_params, data_ptr) )
    elseif OutT <: UniformVector
        (N, T) = OutT.parameters
        @bp_check(InT <: Union{OutT, Contiguous{T}},
                  "Uniform type is ", OutT, ", which isn't compatible with your input type ", InT)
        gl_func_name = Symbol(:glProgramUniform, N, get_uniform_ogl_letter(T), :v)
        return :( $gl_func_name(program, first_param, n_params, data_ptr) )
    elseif OutT <: UniformMatrix
        (C, R, T) = mat_params(OutT)
        @bp_check(InT <: Union{OutT, Contiguous{T}},
                  "Uniform type is ", OutT, ", which isn't compatible with your input type ", InT)
        gl_func_name = Symbol(:glProgramUniformMatrix,
                              (C == R) ? C : Symbol(C, :x, R),
                              get_uniform_ogl_letter(T),
                              :v)
        return :(
            $gl_func_name(program, first_param, n_params, GL_FALSE,
                          bp_unsafe_convert(Ptr{$T}, data_ptr))
        )
    else
        error("Unexpected uniform type ", OutT, " being set with a ", InT, " value")
    end
end

# Gets the correct OpenGL uniform function letter for a given type of data.
get_uniform_ogl_letter(::Type{Float32}) = :f
get_uniform_ogl_letter(::Type{Float64}) = :d
get_uniform_ogl_letter(::Type{Int32}) = :i
get_uniform_ogl_letter(::Type{UInt32}) = :ui
get_uniform_ogl_letter(::Type{UInt64}) = :ui64


####################################
#       Shader string literal      #
####################################

"
Compiles an OpenGL Program from a string literal containing the various shader stages.
The code at the top of the shader is shared between all shader stages.
The Vertex Shader starts with the custom command `#START_VERTEX`.
The Fragment Shader starts with the custom command `#START_FRAGMENT`.
The *optional* Geometry Shader starts with the custom command `#START_GEOMETRY`.
"
macro bp_glsl_str(src::AbstractString)
    # Define info about the different pieces of the shader.
    separators = Dict(
        :vert => ("#START_VERTEX", findfirst("#START_VERTEX", src)),
        :frag => ("#START_FRAGMENT", findfirst("#START_FRAGMENT", src)),
        :geom => ("#START_GEOMETRY", findfirst("#START_GEOMETRY", src))
    )

    # Make sure enough shader stages were provided.
    if isnothing(separators[:vert][2])
        error("Must provide a vertex shader, with the header `", separators[:vert][1], "`")
    elseif isnothing(separators[:frag][2])
        error("Must provide a fragment shader, with the header `", separators[:frag][1], "`")
    end

    # Find the end of the common section of code at the top.
    end_of_common_code::Int = -1 + minimum(first(data[2]) for data in values(separators) if exists(data[2]))

    # Get the boundaries of each section of code, in order.
    section_bounds = [ data[2] for data in values(separators) if exists(data[2]) ]
    sort!(section_bounds, by=first)

    # Get the sections of code for each specific stage.
    function try_find_range(stage::Symbol)
        if isnothing(separators[stage][2])
            return nothing
        end

        my_range_start = last(separators[stage][2]) + 1

        next_bounds_idx = findfirst(bounds -> bounds[1] > my_range_start,
                                    section_bounds)
        my_range_end = isnothing(next_bounds_idx) ?
                           length(src) :
                           first(section_bounds[next_bounds_idx]) - 1

        return my_range_start:my_range_end
    end
    vert_range = try_find_range(:vert)
    frag_range = try_find_range(:frag)
    geom_range = try_find_range(:geom)

    # Generate the individual shaders.
    src_header = src[1:end_of_common_code]
    # Use the #line command to preserve line numbers in shader compile errors.
    gen_line_command(section_start) = string(
        "\n#line ",
        (1 + count(f -> f=='\n', src[1:section_start])),
        "\n"
    )
    src_vertex = string(src_header, gen_line_command(first(vert_range)), src[vert_range])
    src_fragment = string(src_header, gen_line_command(first(frag_range)), src[frag_range])
    src_geom = isnothing(geom_range) ?
                   nothing :
                   string(src_header, gen_line_command(first(geom_range)), src[geom_range])

    # Reference the Program type from anywhere this macro is invoked
    prog_type = Program

    return :(
        $prog_type($src_vertex, $src_fragment, geom_shader=$src_geom)
    )
end
export @bp_glsl_str