######################
#      Uniforms      #
######################

# Uniforms are shader parameters, set by the user through OpenGL calls.


const UniformScalar = Union{Scalar32, Scalar64, Bool}
const UniformVector{N} = Vec{N, <:UniformScalar}
const UniformMatrix{C, R, L} = Mat{C, R, <:Union{Float32, Float64}, L}


"
Acceptable types of data for `set_uniform()` and `set_uniforms()`
    (note that buffers are handled separately)
"
const Uniform = Union{UniformScalar,
                      @unionspec(UniformVector{_}, 1, 2, 3, 4),
                      # All Float32 and Float64 matrices from 2x2 to 4x4:
                      (
                          UniformMatrix{size..., prod(size)}
                            for size in Iterators.product(2:4, 2:4)
                      )...,
                      # "Opaque" types:
                      Texture, Ptr_View, View
                     }
export Uniform


"
Information about a specific Program uniform.
A uniform can be an array of its data type; if the uniform is an array of structs,
    each UniformData instance will be about one specific field of one specific element.

The name is not stored because it will be used as the key for an instance of this struct.
"
struct UniformData
    handle::Ptr_Uniform
    type::Type{<:Uniform}
    array_size::Int  # Set to 1 for non-array uniforms.
end

"
Information about a specific Program buffer block (a.k.a. UBO/SSBO).

The name is not stored because it will be used as the key for an instance of this struct.
"
struct ShaderBlockData
    handle::Ptr_ShaderBuffer
    byte_size::Int32 # For SSBO's with a dynamic-length array at the end,
                     #    this assumes an element count of 1 (although 0 elements is legal).
end



"""
`set_uniform(::Program, ::String, ::T[, array_index::Int])`

Sets a program's uniform to the given value.

If the uniform is an array, you must provide a (1-based) index.

If the uniform is a struct or array of structs, you must name the individual fields/elements,
    e.x. `set_uniform(p, "my_lights[0].pos", new_pos)`.
This naming is 0-based, not 1-based, unfortunately.
"""
function set_uniform end
"""
`set_uniforms(::Program, ::String, ::AbstractVector{T}[, dest_offset=0])`
`set_uniforms(::Program, ::String, T, ::Contiguous{T}[, dest_offset=0])`

Sets a program's uniform to the given value.

You must provide an array of *contiguous* values.
If the contiguous array is more nested than a mere `Array{T}`, `Vec{N, T}`, `NTuple{N, T}`, etc
    (e.x. `Vector{NTuple{5, v2f}}`), then you must also pass the `T` value to remove ambiguity.
If you are passing an array of `Texture`/`View` instances, rather than raw `Ptr_View`s,
    then the collection does not need to be contiguous.

If the uniform is a struct or array of structs, you must name the individual fields/elements,
    e.x. `set_uniform(p, "my_lights[0].pos", new_pos)`.
This naming is 0-based, not 1-based, unfortunately.
"""
function set_uniforms end

"""
`set_uniform_block(::Buffer, ::Int; byte_range=[full buffer])`

Sets the given buffer to be used for one of the
    globally-available Uniform Block (a.k.a. "UBO") slots.

`set_uniform_block(::Program, ::String, ::Int)`

Sets a program's Uniform Block (a.k.a. "UBO")
    to use the given global slot, which can be assigned a Buffer with the other overload.

`set_uniform_block(::Int)`

Clears the binding for the given global UBO slot.

**NOTE**: Indices and byte ranges are 1-based!
"""
function set_uniform_block end

"""
`set_storage_block(::Buffer, ::Int; byte_range=[full buffer])`

Sets the given buffer to be used for one of the
    globally-available Shader Storage Block (a.k.a. "SSBO") slots.

`set_storage_block(::Program, ::String, ::Int)`

Sets a program's Shader-Storage Block (a.k.a. "SSBO")
    to use the given global slot, which can be assigned a Buffer with the other overload.

`set_storage_block(::Int)`

Clears the binding for the given global SSBO slot.

**NOTE**: Indices and byte ranges are 1-based!
"""
function set_storage_block end


export Uniform, UniformData, set_uniform, set_uniforms, set_uniform_block, set_storage_block


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

struct RenderProgramSource
    src_vertex::String
    src_fragment::String
    src_geometry::Optional{String}
end

struct ComputeProgramSource
    src::String
end

"A set of data to be compiled into an OpenGL shader program"
mutable struct ProgramCompiler
    source::Union{RenderProgramSource, ComputeProgramSource}

    # A pre-compiled version of this shader which this compiler can attempt to use first.
    # The shader source code is still needed as a fallback,
    #    as shaders may need recompilation after driver changes.
    # After compilation, if the user wants, this field can be updated with the newest binary.
    cached_binary::Optional{PreCompiledProgram}
end

ProgramCompiler( src_vertex::AbstractString, src_fragment::AbstractString
                 ;
                 src_geometry::Optional{AbstractString} = nothing,
                 cached_binary::Optional{PreCompiledProgram} = nothing
               ) = ProgramCompiler(
    RenderProgramSource(string(src_vertex), string(src_fragment),
                        isnothing(src_geometry) ? nothing : string(src_geometry)),
    cached_binary
)
ProgramCompiler( src_compute::AbstractString
                 ;
                 cached_binary::Optional{PreCompiledProgram} = nothing
               ) = ProgramCompiler(
    ComputeProgramSource(string(src_compute)),
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
    if p.source isa RenderProgramSource
        for data in (("vertex", GL_VERTEX_SHADER, p.source.src_vertex),
                    ("fragment", GL_FRAGMENT_SHADER, p.source.src_fragment),
                    ("geometry", GL_GEOMETRY_SHADER, p.source.src_geometry))
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
    elseif p.source isa ComputeProgramSource
        result = compile_stage("compute", GL_COMPUTE_SHADER, p.source.src)
        if result isa String
            # Clean up the shaders/program, then return the error message.
            map(glDeleteShader, compiled_handles)
            glDeleteProgram(out_ptr)
            return result
        else
            push!(compiled_handles, result)
        end
    else
        error("Unhandled case: ", typeof(p.source))
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
        glGetProgramInfoLog(out_ptr, msg_len, Ref(Int32(msg_len)), Ref(msg_data, 1))

        glDeleteProgram(out_ptr)
        return string("Error linking shaders: ", String(msg_data[1:msg_len]))
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
mutable struct Program <: AbstractResource
    handle::Ptr_Program
    compute_work_group_size::Optional{v3u} # Only exists for compute shaders
    uniforms::Dict{String, UniformData}
    uniform_blocks::Dict{String, ShaderBlockData}
    storage_blocks::Dict{String, ShaderBlockData}
    flexible_mode::Bool # Whether to silently ignore invalid uniforms
end

function Program(vert_shader::String, frag_shader::String
                 ;
                 geom_shader::Optional{String} = nothing,
                 flexible_mode::Bool = true)
    compiler = ProgramCompiler(vert_shader, frag_shader; src_geometry = geom_shader)
    result = compile_program(compiler)
    if result isa Ptr_Program
        return Program(result, flexible_mode)
    elseif result isa String
        error(result)
    else
        error("Unhandled case: ", typeof(result), "\n\t: ", result)
    end
end
function Program(compute_shader::String
                 ;
                 flexible_mode::Bool = true)
    compiler = ProgramCompiler(compute_shader)
    result = compile_program(compiler)
    if result isa Ptr_Program
        return Program(result, flexible_mode; is_compute=true)
    elseif result isa String
        error(result)
    else
        error("Unhandled case: ", typeof(result), "\n\t: ", result)
    end
end
function Program(handle::Ptr_Program, flexible_mode::Bool = false; is_compute::Bool = false)
    context = get_context()
    @bp_check(exists(context), "Creating a Program without a valid Context")

    name_c_buffer = Vector{GLchar}(undef, 128)

    # Get the shader storage blocks.
    n_storage_blocks = get_from_ogl(GLint, glGetProgramInterfaceiv,
                                    handle, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES)
    storage_blocks = Dict{String, ShaderBlockData}()
    resize!(name_c_buffer,
            get_from_ogl(GLint, glGetProgramInterfaceiv,
                         handle, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH))
    for block_idx::Int in 1:n_storage_blocks
        block_name_length = Ref(zero(GLsizei))
        glGetProgramResourceiv(handle,
                               # The resource:
                               GL_SHADER_STORAGE_BLOCK, block_idx - 1,
                               # The desired data:
                               1, Ref(GL_NAME_LENGTH),
                               # The output buffer(s):
                               1, C_NULL, block_name_length)

        glGetProgramResourceName(handle,
                                 GL_SHADER_STORAGE_BLOCK, block_idx - 1,
                                 block_name_length[], C_NULL, Ref(name_c_buffer, 1))
        block_name = String(@view name_c_buffer[1 : (block_name_length[] - 1)])
        storage_blocks[block_name] = ShaderBlockData(
            Ptr_ShaderBuffer(block_idx - 1),
            get_from_ogl(GLint, glGetProgramResourceiv,
                         # The resource:
                         handle, GL_SHADER_STORAGE_BLOCK, block_idx - 1,
                         # The desired data:
                         1, Ref(GL_BUFFER_DATA_SIZE),
                         # The output buffer(s):
                         1, C_NULL)
        )
    end

    # Get the uniform buffer blocks.
    n_uniform_blocks = get_from_ogl(GLint, glGetProgramiv, handle, GL_ACTIVE_UNIFORM_BLOCKS)
    uniform_blocks = Dict{String, ShaderBlockData}()
    resize!(name_c_buffer,
            get_from_ogl(GLint, glGetProgramiv, handle, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH))
    for block_idx::Int in 1:n_uniform_blocks
        block_name_length::GLsizei = 0
        @c glGetActiveUniformBlockName(handle, block_idx, length(name_c_buffer),
                                       &block_name_length, &name_c_buffer[0])

        block_name = String(@view name_c_buffer[1 : (block_name_length - 1)])
        uniform_blocks[block_name] = ShaderBlockData(
            Ptr_ShaderBuffer(block_idx - 1),
            get_from_ogl(GLint, glGetActiveUniformBlockiv,
                         handle, block_idx, GL_UNIFORM_BLOCK_DATA_SIZE)
        )
    end

    # Gather handles of uniforms witin the blocks and ignore them below.
    uniform_indices_to_skip = Set{GLint}()
    block_uniform_indices = Vector{GLint}(undef, 128)
    for block_idx::Int in 1:n_uniform_blocks
        resize!(block_uniform_indices,
                get_from_ogl(GLint, glGetActiveUniformBlockiv, handle, block_idx,
                             GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS))
        @c glGetActiveUniformBlockiv(handle, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                                     block_idx, &block_uniform_indices[0])
        append!(uniform_indices_to_skip, block_uniform_indices)
    end

    # Get the uniforms.
    uniforms = Dict{String, UniformData}()
    glu_array_count = Ref{GLint}()  # The length of this uniform array (1 if it's not an array).
    glu_type = Ref{GLenum}()
    glu_name_buf = Vector{GLchar}(
        undef,
        get_from_ogl(GLint, glGetProgramiv,
                     handle, GL_ACTIVE_UNIFORM_MAX_LENGTH)
    )
    glu_name_len = Ref{GLsizei}()
    for i in 1:get_from_ogl(GLint, glGetProgramiv, handle, GL_ACTIVE_UNIFORMS)
        if i in uniform_indices_to_skip
            continue
        end

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

    # Connect to the view-debugger service.
    service_ViewDebugging_add_program(handle)

    return Program(handle,
                   is_compute ?
                      v3u(get_from_ogl(GLint, 3, glGetProgramiv,
                                       handle, GL_COMPUTE_WORK_GROUP_SIZE)...) :
                      nothing,
                   uniforms,
                   uniform_blocks,
                   storage_blocks,
                   flexible_mode)
end

function Base.close(p::Program)
    service_ViewDebugging_remove_program(p.handle)
    @bp_check(p.handle != Ptr_Program(), "Already closed this Program")
    glDeleteProgram(p.handle)
    setfield!(p, :handle, Ptr_Program())
end

"Gets the raw Julia type of a uniform with the given OpenGL type."
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

    GL_INT64_ARB => Int64,
    GL_INT64_VEC2_ARB => Vec{2, Int64},
    GL_INT64_VEC3_ARB => Vec{3, Int64},
    GL_INT64_VEC4_ARB => Vec{4, Int64},
    GL_UNSIGNED_INT64_ARB => UInt64,
    GL_UNSIGNED_INT64_VEC2_ARB => Vec{2, UInt64},
    GL_UNSIGNED_INT64_VEC3_ARB => Vec{3, UInt64},
    GL_UNSIGNED_INT64_VEC4_ARB => Vec{4, UInt64},

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

    #TODO: GL_UNSIGNED_INT_ATOMIC_COUNTER => Ptr_Buffer ??
)


export Program


#################################
#     Setting uniforms Impl     #
#################################

get_uniform(program::Program, name::String) =
    if haskey(program.uniforms, name)
        program.uniforms[name]
    elseif program.flexible_mode
        nothing
    else
        error("Uniform not found (was it optimized out?): '", name, "'")
    end
#


# Setting a single uniform:
function set_uniform(program::Program, name::String, value::T) where {T}
    # If the data is a higher-level object, convert it into its lowered form.
    if T == Texture
        set_uniform(program, name, get_view(value))
    elseif T == View
        set_uniform(program, name, value.handle)
    else
        u_data::Optional{UniformData} = get_uniform(program, name)
        if isnothing(u_data)
            return
        end
        @bp_check(u_data.array_size == 1, "No index given for the array uniform '", name, "'")

        ref = Ref(value)
        GC.@preserve ref begin
            ptr = contiguous_ptr(ref, T)
            set_uniform(u_data.type, T, ptr, program.handle, u_data.handle, 1)
        end
    end
end

# Setting an element of a uniform array:
function set_uniform(program::Program, name::String, value::T, index::Int) where {T<:Uniform}
    # If the data is a higher-level object, convert it into its lowered form.
    if T == Texture
        set_uniform(program, name, get_view(value), index)
    elseif T == View
        set_uniform(program, name, value.handle, index)
    else
        u_data::Optional{UniformData} = get_uniform(program, name)
        if isnothing(u_data)
            return
        end
        @bp_check(index <= u_data.array_size,
                "Array uniform '", name, "' only has ", u_data.array_size, " elements,",
                    " and you're trying to set index ", index)

        handle = Ptr_Uniform(gl_type(u_data.handle) + (index - 1))

        ref = Ref(value)
        GC.@preserve ref begin
            ptr = contiguous_ptr(ref, T)
            set_uniform(u_data.type, T, ptr, program.handle, handle, 1)
        end
    end
end

# Setting an array of uniforms:
@inline set_uniforms( program::Program, name::String,
                      values::TContiguousSimple,
                      dest_offset::Integer = 0
                    ) where {T<:Uniform, TContiguousSimple <: ContiguousSimple{T}} =
    set_uniforms(program, name, T, values, dest_offset)

function set_uniforms( program::Program, name::String,
                       ::Type{T}, values::TCollection,
                       dest_offset::Integer = 0
                     ) where {T<:Uniform, TCollection}
    # If the incoming data is higher-level objects, convert them to their lowered version.
    if T <: Union{Texture, View}
        @bp_check(TCollection <: Union{ConstVector{T}, AbstractArray{T}},
                  "Passing a collection of textures/views to set a uniform array,",
                  " but the collection isn't array-like! It's a ", TCollection)

        handles::Vector{Ptr_View} = map(values) do element::Union{Texture, View}
            if element isa Texture
                return get_view(element).handle
            elseif element isa View
                return element.handle
            else
                error("Unhandled case: ", typeof(element))
            end
        end
        set_uniforms(program, name, Ptr_View, handles, dest_offset)
    else
        # The collection must be contiguous to be uploaded to OpenGL through the C API.
        @bp_check(TCollection <: Contiguous{T},
                 "Passing an array of data to set a uniform array,",
                 " but the data isn't contiguous! It's a ", TCollection)

        u_data::Optional{UniformData} = get_uniform(program, name)
        if isnothing(u_data)
            return
        end

        n_available_uniforms::Int = u_data.array_size - dest_offset
        count::Int = contiguous_length(values, T)
        @bp_check(count <= n_available_uniforms,
                    "Array uniform '", name, "' only has ", u_data.array_size, " elements,",
                    " and you're trying to set elements ",
                    (dest_offset + 1), ":", (dest_offset + count))

        handle = Ptr_Uniform(gl_type(u_data.handle) + dest_offset)
        ref = contiguous_ref(values, T)
        GC.@preserve ref begin
            ptr = contiguous_ptr(ref, T)
            set_uniform(u_data.type, T, ptr, program.handle, handle, count)
        end
    end
end


# The actual implementation, which calls into the proper OpenGL function
#    based on the uniform type:
@generated function set_uniform( ::Type{TOut}, ::Type{TIn},
                                 data_ptr::Ptr,
                                 program::Ptr_Program,
                                 first_param::Ptr_Uniform, n_params::Integer
                               ) where {TOut<:Uniform, TIn<:Contiguous}
    if TOut == Ptr_View
        return quote
            # Update the view-debugger service with these new uniforms.
            # REPL tests indicate that the unsafe_wrap() call always happens,
            #    even if the view-debugger code is compiled into a no-op.
            # This apparently leads to a heap-allocation.
            # Avoid this by manually wrapping in the "debug-only" macro.
            @bp_gl_debug service_ViewDebugging_set_views(
                program, first_param,
                unsafe_wrap(Array, data_ptr, n_params)
            )

            glProgramUniformHandleui64vARB(program, first_param, n_params, data_ptr)
        end
    elseif TOut <: UniformScalar
        gl_func_name = Symbol(:glProgramUniform1, get_uniform_ogl_letter(TOut), :v)
        return :( $gl_func_name(program, first_param, n_params, data_ptr) )
    elseif TOut <: UniformVector
        (N, T) = TOut.parameters
        gl_func_name = Symbol(:glProgramUniform, N, get_uniform_ogl_letter(T), :v)
        return :( $gl_func_name(program, first_param, n_params, data_ptr) )
    elseif TOut <: UniformMatrix
        (C, R, T) = mat_params(TOut)
        gl_func_name = Symbol(:glProgramUniformMatrix,
                              (C == R) ? C : Symbol(C, :x, R),
                              get_uniform_ogl_letter(T),
                              :v)
        return :( $gl_func_name(program, first_param, n_params, GL_FALSE, data_ptr) )
    else
        error("Unexpected uniform type ", TOut, " being set with a ", TIn, " value")
    end
end


# Uniform blocks:
function set_uniform_block(buf::Buffer, idx::Int;
                           context::Context = get_context(),
                           byte_range::Interval{<:Integer} = Interval(
                               min=1,
                               max=buf.byte_size
                           ))
    handle = get_ogl_handle(buf)
    binding = (handle, convert(Interval{Int}, byte_range))
    if context.active_ubos[idx] != binding
        context.active_ubos[idx] = binding
        glBindBufferRange(GL_UNIFORM_BUFFER, idx - 1, handle,
                          min_inclusive(byte_range) - 1,
                          size(byte_range))
    end
    return nothing
end
function set_uniform_block(program::Program, name::AbstractString, idx::Int)
    if !haskey(program.uniform_blocks, name)
        if program.flexible_mode
            return nothing
        else
            error("Uniform block not found (was it optimized out?): '", name, "'")
        end
    end
    glUniformBlockBinding(get_ogl_handle(program),
                          program.uniform_blocks[name].handle,
                          idx - 1)
    return nothing
end
function set_uniform_block(idx::Int; context::Context = get_context())
    cleared_binding = (Ptr_Buffer(), Interval{Int}(min=-1, max=-1))
    if context.active_ubos[idx] != cleared_binding
        context.active_ubos[idx] = cleared_binding
        glBindBufferBase(GL_UNIFORM_BUFFER, idx - 1, Ptr_Buffer())
    end
    return nothing
end

# Shader Storage blocks:
function set_storage_block(buf::Buffer, idx::Int;
                           context::Context = get_context(),
                           byte_range::Interval{<:Integer} = Interval(
                               min=1,
                               max=buf.byte_size
                           ))
    handle = get_ogl_handle(buf)
    binding = (handle, convert(Interval{Int}, byte_range))
    if context.active_ssbos[idx] != binding
        context.active_ssbos[idx] = binding
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, idx - 1, handle,
                          min_inclusive(byte_range) - 1,
                          size(byte_range))
    end
    return nothing
end
function set_storage_block(program::Program, name::AbstractString, idx::Int)
    if !haskey(program.storage_blocks, name)
        if program.flexible_mode
            return nothing
        else
            error("Storage block not found (was it optimized out?): '", name, "'")
        end
    end
    glShaderStorageBlockBinding(get_ogl_handle(program),
                                program.storage_blocks[name].handle,
                                idx - 1)
    return nothing
end
function set_storage_block(idx::Int; context::Context = get_context())
    cleared_binding = (Ptr_Buffer(), Interval{Int}(min=-1, max=-1))
    if context.active_ssbos[idx] != cleared_binding
        context.active_ssbos[idx] = cleared_binding
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, idx - 1, Ptr_Buffer())
    end
    return nothing
end


# Retrieve the correct OpenGL uniform function letter for a given type of data.
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
To compile a non-literal string in the same way, call `bp_glsl_str(shader_string)`.

The code at the top of the shader is shared between all shader stages.
The Vertex Shader starts with the custom command `#START_VERTEX`.
The Fragment Shader starts with the custom command `#START_FRAGMENT`.
The *optional* Geometry Shader starts with the custom command `#START_GEOMETRY`.

For a Compute Shader, use `#START_COMPUTE`.
"
macro bp_glsl_str(src::AbstractString)
    return bp_glsl_str(src, Val(true))
end
"
Compiles a string with special formatting into a `Program`.
For info on how to format it, refer to the docs for the macro version, `@bp_glsl_str`.
"
function bp_glsl_str(src::AbstractString, ::Val{GenerateCode} = Val(false)) where {GenerateCode}
    # Define info about the different pieces of the shader.
    separators = Dict(
        :vert => ("#START_VERTEX", findfirst("#START_VERTEX", src)),
        :frag => ("#START_FRAGMENT", findfirst("#START_FRAGMENT", src)),
        :geom => ("#START_GEOMETRY", findfirst("#START_GEOMETRY", src)),
        :compute => ("#START_COMPUTE", findfirst("#START_COMPUTE", src))
    )

    # Make sure the right shader stages were provided.
    local is_compute::Bool
    if exists(separators[:compute][2])
        is_compute = true
        @bp_check(all(isnothing.(getindex.(getindex.(Ref(separators), (:vert, :frag, :geom)),
                                           Ref(2)))),
                  "Can't provide compute shader with rendering shaders")
    else
        is_compute = false
        if isnothing(separators[:vert][2])
            error("Must provide a vertex shader, with the header `", separators[:vert][1], "`")
        elseif isnothing(separators[:frag][2])
            error("Must provide a fragment shader, with the header `", separators[:frag][1], "`")
        end
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
    compute_range = try_find_range(:compute)

    # Generate the individual shaders.
    src_header = src[1:end_of_common_code]
    # Use the #line command to preserve line numbers in shader compile errors.
    gen_line_command(section_start) = string(
        "\n#line ",
        (1 + count(f -> f=='\n', src[1:section_start])),
        "\n"
    )
    src_vertex = isnothing(vert_range) ?
                     nothing :
                     string("#define IN_VERTEX_SHADER\n", src_header,
                            gen_line_command(first(vert_range)),
                            src[vert_range])
    src_fragment = isnothing(frag_range) ?
                       nothing :
                       string("#define IN_FRAGMENT_SHADER\n", src_header,
                              gen_line_command(first(frag_range)),
                              src[frag_range])
    src_geom = isnothing(geom_range) ?
                   nothing :
                   string("#define IN_GEOMETRY_SHADER\n", src_header,
                          gen_line_command(first(geom_range)),
                          src[geom_range])
    src_compute = isnothing(compute_range) ?
                      nothing :
                      string("#define IN_COMPUTE_SHADER\n", src_header,
                             gen_line_command(first(compute_range)),
                             src[compute_range])

    if GenerateCode
        prog_type = Program
        if is_compute
            return :( $prog_type($src_compute) )
        else
            return :( $prog_type($src_vertex, $src_fragment, geom_shader=$src_geom) )
        end
    else
        if is_compute
            return Program(src_compute)
        else
            return Program(src_vertex, src_fragment, geom_shader=src_geom)
        end
    end
end
export @bp_glsl_str