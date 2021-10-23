"""
A contiguous block of memory on the GPU,
   for storing any kind of data.
Most commonly used to store mesh vertices/indices, or compute shader data.
Instances can be "mapped" to the CPU, allowing you to write/read them directly
   as if they were a plain C array.
This is often more efficient than setting the buffer data the usual way,
   e.x. you could read the mesh data from disk directly into this mapped memory.
Like other GL objects, you cannot set the fields of this object directly.
Instead, you should use its interface.
"""
mutable struct Buffer
    handle::Ptr_Buffer

    # Creates a buffer of the given byte-size, optionally with
    #    support for mapping its memory onto the CPU.
    # If no initial data is given, the buffer will contain garbage.
    Buffer( byte_size::Integer, can_change_data::Bool,
            initial_byte_data::Optional{Vector{UInt8}} = nothing,
            recommend_storage_on_cpu::Bool = false
          )::Buffer = set_up_buffer(
        byte_size, can_change_data,
        initial_byte_data,
        recommend_storage_on_cpu,
        new()
    )
    function Buffer( n_elements::I, can_change_data::Bool,
                     initial_elements::Vector{T},
                     recommend_storage_on_cpu::Bool = false
                   )::Buffer where {I<:Integer}
        
    end
end

@inline function set_up_buffer( byte_size::I, can_change_data::Bool,
                                initial_byte_data::Ref{UInt8} = nothing,
                                recommend_storage_on_cpu::Bool = false,
                                output::Buffer
                              )::Buffer where {I<:Integer}
    @bp_check(exists(get_context()), "No Bplus context to create this buffer in")
    @bp_check(isnothing(initial_byte_data) || (length(initial_byte_data) == byte_size),
                "Buffer is $(Base.format_bytes(byte_size)), but initial data array ",
                "is $(Base.format_bytes(length(initial_byte_data)))")

    handle::Ptr_Buffer = get_from_ogl(gl_type(Ptr_Buffer), glCreateBuffers, 1)

    flags::GLbitfield = 0
    if recommend_storage_on_cpu
        flags |= GL_CLIENT_STORAGE_BIT
    end
    if can_change_data
        flags |= GL_DYNAMIC_STORAGE_BIT
    end

    data_ptr = exists(initial_byte_ata) ?
                    Ref(initial_byte_data) :
                    Ref{UInt8}(C_NULL)
    glNamedBufferStorage(handle, byte_size, data_ptr, flags)

    setfield(output, :handle, handle)
end


################################
#       Buffer Operations      #
################################

#TODO: Implement