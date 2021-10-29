"""
A contiguous block of memory on the GPU,
   for storing any kind of data.
Most commonly used to store mesh vertices/indices, or compute shader data.
Instances can be "mapped" to the CPU, allowing you to write/read them directly
   as if they were a plain C array.
This is often more efficient than setting the buffer data the usual way,
   e.x. you could read the mesh data from disk directly into this mapped memory.
Like other GL resources, you cannot set the fields of this object directly.
Instead, you should use its interface.
"""
mutable struct Buffer
    handle::Ptr_Buffer
    byte_size::UInt64
    is_mutable::Bool

    # Creates a buffer of the given byte-size, optionally with
    #    support for mapping its memory onto the CPU.
    # If no initial data is given, the buffer will contain garbage.
    function Buffer( byte_size::Integer, can_change_data::Bool,
                     initial_byte_data::Optional{Vector{UInt8}} = nothing,
                     recommend_storage_on_cpu::Bool = false
                   )::Buffer
        b = new(Ptr_Buffer(), 0, false)
        set_up_buffer(
            byte_size, can_change_data,
            Ref(initial_byte_data),
            recommend_storage_on_cpu,
            b)
        return b
    end
    function Buffer( n_elements::I, can_change_data::Bool,
                     initial_elements::Vector{T},
                     recommend_storage_on_cpu::Bool = false
                   )::Buffer where {I<:Integer, T}
        b = new(Ptr_Buffer(), 0, false)
        set_up_buffer(
            n_elements * sizeof(T), can_change_data,
            Ref(initial_elements),
            recommend_storage_on_cpu
        )
        return b
    end
end

@inline function set_up_buffer( byte_size::I, can_change_data::Bool,
                                initial_byte_data::Ref,
                                recommend_storage_on_cpu::Bool,
                                output::Buffer
                              ) where {I<:Integer}
    @bp_check(exists(get_context()), "No Bplus context to create this buffer in")
    @bp_check(isnothing(initial_byte_data) || (length(initial_byte_data) == byte_size),
                "Buffer is $byte_size, but initial data array ",
                "is $(length(initial_byte_data))")

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
    setfield(output, :byte_size, UInt64(byte_size))
    setfield(output, :is_mutable, can_change_data)
end

Base.show(io::IO, b::Buffer) = print(io,
    "Buffer<",
    Base.format_bytes(b.byte_size),
    (b.is_mutable ? " Mutable" : ""),
    " ", b.handle,
    ">"
)

function Base.close(b::Buffer)
    glDeleteBuffers(1, Ref(b.handle))
end

Base.setproperty!(b::Buffer, name::Symbol, value) = error("Can't set the fields of a Buffer! Use the functions instead")

export Buffer


################################
#       Buffer Operations      #
################################

"
Uploads the given data into the buffer.
Note that counts are per-element, not per-byte
   (unless the elements you're uploading are 1 byte each).
"
function set_buffer_data( b::Buffer,
                          new_elements::Vector{T}
                          ;
                          # Which part of the input array to read from
                          src_element_range::IntervalU = IntervalU(1, length(new_elements)),
                          # Shifts the first element of the buffer's array to write to
                          dest_element_offset::UInt = 0x0,
                          # A byte offset, to be combinend wth 'dest_element_start'
                          dest_byte_offset::UInt = 0x0
                        ) where {T}
    @bp_check(b.is_mutable, "Can't change this Buffer's data after creation; it's immutable")
    @bp_check(inclusive_max(src_element_range) <= length(new_elements),
              "Trying to upload a range of data beyond the input buffer")

    first_byte::UInt = dest_byte_offset + (dest_element_offset * sizeof(T))
    byte_size::UInt = sizeof(T) * src_element_range.size
    last_byte::UInt = first_byte + byte_size - 1
    @bp_check(last_byte <= b.byte_size,
              "Trying to write past the end of the buffer: ",
                 "bytes ", first_byte, " => ", last_byte,
                 ", when there's only ", b.byte_size, " bytes")

    if byte_size >= 1
        glNamedBufferSubData(b.handle, first_byte, byte_size,
                             Ref(new_elements, src_element_range.min))
    end
end

"
Loads the buffer's data into the given array.
If given a type instead of an array,
   then a new array of that type is allocated and returned.
Note that counts are per-element, not per-byte
   (unless the elements you're reading are 1 byte each).
"
function get_buffer_data( b::Buffer,
                          # The array which will contain the results,
                          #    or the type of the new array to make
                          output::Union{Vector{T}, Type{T}}
                          ;
                          # Shifts the first element to write to in the output array
                          dest_offset::UInt = 0x0,
                          # The elements to read from the buffer (defaults to as much as possible)
                          src_elements::IntervalU = IntervalU(1, (output isa Vector{T}) ?
                                                                   (length(output) - dest_offset) :
                                                                   b.byte_size ÷ sizeof(T)),
                          # The start of the buffer's array data
                          src_byte_offset::UInt = 0x0
                        )::Optional{Vector{T}} where {T}
    src_first_byte::UInt = src_byte_offset + (src_elements.min * sizeof(T))
    src_last_byte::UInt = src_first_byte + (src_elements.size * sizeof(T))
    n_bytes::UInt = src_last_byte - src_first_byte + 1

    if output isa Vector{T}
        @bp_check(dest_offset + src_elements.size <= length(output),
                  "Trying to read Buffer into an array, but the array isn't big enough.",
                    " Trying to write to elements ", (dest_offset + 1),
                    " - ", (dest_offset + src_elements.size), ", but there are only ",
                    length(output))
    else
        @bp_check(dest_offset == 0x0, "You provided 'dest_offset' but not an output array")
    end
    output_array::Vector{T} = (output isa Vector{T}) ?
                                  output :
                                  Vector{T}(undef, src_elements.size)

    glGetNamedBufferSubData(b.handle, src_first_byte, n_bytes, Ref(output_array))
    if !(output isa Vector{T})
        return output_array
    else
        return nothing
    end
end

"
Copies data from one buffer to another.
By default, copies as much data as possible.
"
function copy_buffer( src::Buffer, dest::Buffer
                      ;
                      src_byte_offset::UInt = 0x0,
                      dest_byte_offset::UInt = 0x0,
                      byte_size::UInt = min(src.byte_size - src.byte_offset,
                                            dest.byte_size - dest.byte_offset)
                    )
    @bp_check(src_byte_offset + byte_size <= src.byte_size,
              "Going outside the bounds of the 'src' buffer in a copy:",
                " from ", src_byte_offset, " to ", src_byte_offset + byte_size)
    @bp_check(dest_byte_offset + byte_size <= dest.byte_size,
              "Going outside the bounds of the 'dest' buffer in a copy:",
                " from ", dest_byte_offset, " to ", dest_byte_offset + byte_size)
    glCopyNamedBufferSubData(src.handle, dest.handle,
                             src_byte_offset,
                             dest_byte_offset,
                             byte_size)
end

export set_buffer_data, get_buffer_data, copy_buffer

#TODO: Rest of the operations (mapping)
#TODO: Unit tests