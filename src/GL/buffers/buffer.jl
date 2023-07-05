"""
A contiguous block of memory on the GPU,
   for storing any kind of data.
Most commonly used to store mesh vertices/indices, or other arrays of things.
Instances can be "mapped" to the CPU, allowing you to write/read them directly
   as if they were a plain C array.
This is often more efficient than setting the buffer data the usual way,
   e.x. you could read the mesh data from disk directly into this mapped memory.
"""
mutable struct Buffer <: Resource
    handle::Ptr_Buffer
    byte_size::UInt64
    is_mutable::Bool

    # Creates a buffer of the given byte-size, optionally with
    #    support for mapping its memory onto the CPU.
    # The buffer will initially contain garbage.
    function Buffer( byte_size::Integer, can_change_data::Bool,
                     recommend_storage_on_cpu::Bool = false
                   )::Buffer
        b = new(Ptr_Buffer(), 0, false)
        set_up_buffer(
            byte_size, can_change_data,
            nothing,
            recommend_storage_on_cpu,
            b
        )
        return b
    end
    function Buffer( can_change_data::Bool,
                     initial_elements::Contiguous{T},
                     ::Type{T} = eltype(initial_elements)
                     ;
                     recommend_storage_on_cpu::Bool = false,
                     contiguous_element_range::Interval{<:Integer} = Interval((
                        min=1,
                        size=contiguous_length(initial_elements, T)
                    ))
                   )::Buffer where {I<:Integer, T}
        @bp_check(isbitstype(T), "Can't make a GPU buffer of ", T)
        b = new(Ptr_Buffer(), 0, false)
        set_up_buffer(
            size(contiguous_element_range) * sizeof(T),
            can_change_data,
            contiguous_ref(initial_elements, T,
                           min_inclusive(contiguous_element_range)),
            recommend_storage_on_cpu,
            b
        )
        return b
    end
end

#TODO: Support taking a raw pointer for the buffer data

@inline function set_up_buffer( byte_size::I, can_change_data::Bool,
                                initial_byte_data::Optional{Ref},
                                recommend_storage_on_cpu::Bool,
                                output::Buffer
                              ) where {I<:Integer}
    @bp_check(exists(get_context()), "No Bplus Context to create this buffer in")
    handle::Ptr_Buffer = Ptr_Buffer(get_from_ogl(gl_type(Ptr_Buffer), glCreateBuffers, 1))

    flags::GLbitfield = 0
    if recommend_storage_on_cpu
        flags |= GL_CLIENT_STORAGE_BIT
    end
    if can_change_data
        flags |= GL_DYNAMIC_STORAGE_BIT
    end

    setfield!(output, :handle, handle)
    setfield!(output, :byte_size, UInt64(byte_size))
    setfield!(output, :is_mutable, can_change_data)

    glNamedBufferStorage(handle, byte_size,
                         exists(initial_byte_data) ?
                             initial_byte_data :
                             C_NULL,
                         flags)
end

Base.show(io::IO, b::Buffer) = print(io,
    "Buffer<",
    Base.format_bytes(b.byte_size),
    (b.is_mutable ? " Mutable" : ""),
    " ", b.handle,
    ">"
)

function Base.close(b::Buffer)
    h = b.handle
    glDeleteBuffers(1, Ref{GLuint}(b.handle))
    setfield!(b, :handle, Ptr_Buffer())
end

export Buffer


################################
#       Buffer Operations      #
################################

"
Uploads the given data into the buffer.
Note that counts are per-element, not per-byte.
"
function set_buffer_data( b::Buffer,
                          new_elements::Vector{T}
                          ;
                          # Which part of the input array to read from
                          src_element_range::IntervalU = IntervalU((min=1, size=length(new_elements))),
                          # Shifts the first element of the buffer's array to write to
                          dest_element_offset::UInt = zero(UInt),
                          # A byte offset, to be combined wth 'dest_element_start'
                          dest_byte_offset::UInt = zero(UInt)
                        ) where {T}
    @bp_check(b.is_mutable, "Buffer is immutable")
    @bp_check(max_inclusive(src_element_range) <= length(new_elements),
              "Trying to upload a range of data beyond the input buffer")

    first_byte::UInt = dest_byte_offset + ((dest_element_offset) * sizeof(T))
    byte_size::UInt = sizeof(T) * size(src_element_range)
    last_byte::UInt = first_byte + byte_size - 1
    @bp_check(last_byte <= b.byte_size,
              "Trying to write past the end of the buffer: ",
                 "bytes ", first_byte, " => ", last_byte,
                 ", when there's only ", b.byte_size, " bytes")

    if byte_size >= 1
        ptr = Ref(new_elements, Int(min_inclusive(src_element_range)))
        glNamedBufferSubData(b.handle, first_byte, byte_size, ptr)
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
                          output::Union{Vector{T}, Type{T}} = UInt8
                          ;
                          # Shifts the first element to write to in the output array
                          dest_offset::Integer = zero(UInt),
                          # The start of the buffer's array data
                          src_byte_offset::Integer = zero(UInt),
                          # The elements to read from the buffer (defaults to as much as possible)
                          src_elements::IntervalU = IntervalU((
                              min=1,
                              size=min((b.byte_size - src_byte_offset) ÷ sizeof(T),
                                       (output isa Vector{T}) ?
                                           (length(output) - dest_offset) :
                                           typemax(UInt))
                          ))
                        )::Optional{Vector{T}} where {T}
    src_first_byte::UInt = convert(UInt, src_byte_offset + ((min_inclusive(src_elements) - 1) * sizeof(T)))
    n_bytes::UInt = convert(UInt, size(src_elements) * sizeof(T))

    if output isa Vector{T}
        @bp_check(dest_offset + size(src_elements) <= length(output),
                  "Trying to read Buffer into an array, but the array isn't big enough.",
                    " Trying to write to elements ", (dest_offset + 1),
                    " - ", (dest_offset + size(src_elements)), ", but there are only ",
                    length(output))
    else
        @bp_check(dest_offset == 0x0,
                  "In 'get_buffer_data()', you provided 'dest_offset' of ", dest_offset,
                     " but no output array")
    end
    output_array::Vector{T} = (output isa Vector{T}) ?
                                  output :
                                  Vector{T}(undef, size(src_elements))

    output_ptr = Ref(output_array, Int(dest_offset + 1))

    glGetNamedBufferSubData(b.handle, src_first_byte, n_bytes, output_ptr)
    
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
                      src_byte_offset::Integer = 0x0,
                      dest_byte_offset::Integer = 0x0,
                      byte_size::Integer = min(src.byte_size - src.byte_offset,
                                               dest.byte_size - dest.byte_offset)
                    )
    @bp_check(src_byte_offset + byte_size <= src.byte_size,
              "Going outside the bounds of the 'src' buffer in a copy:",
                " from ", src_byte_offset, " to ", src_byte_offset + byte_size)
    @bp_check(dest_byte_offset + byte_size <= dest.byte_size,
              "Going outside the bounds of the 'dest' buffer in a copy:",
                " from ", dest_byte_offset, " to ", dest_byte_offset + byte_size)
    @bp_check(dest.is_mutable, "Destination buffer is immutable")

    glCopyNamedBufferSubData(src.handle, dest.handle,
                             src_byte_offset,
                             dest_byte_offset,
                             byte_size)
end

export set_buffer_data, get_buffer_data, copy_buffer


#TODO: Rest of the operations (mapping)