"""
A contiguous block of memory on the GPU,
   for storing any kind of data.
Most commonly used to store mesh vertices/indices, or other arrays of things.
Instances can be "mapped" to the CPU, allowing you to write/read them directly
   as if they were a plain C array.
This is often more efficient than setting the buffer data the usual way,
   e.x. you could read the mesh data from disk directly into this mapped memory.
"""
mutable struct Buffer <: AbstractResource
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
                     contiguous_element_range::Interval{<:Integer} = Interval(
                        min=1,
                        size=contiguous_length(initial_elements, T)
                     )
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
                          src_elements::IntervalU = IntervalU(
                              min=1,
                              size=min((b.byte_size - src_byte_offset) ÷ sizeof(T),
                                       (output isa Vector{T}) ?
                                           (length(output) - dest_offset) :
                                           typemax(UInt))
                          )
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


###############################
#       Automatic Layout      #
###############################

#TODO: My whole approach to @std140 is broken because Julia pads bits data itself. I need to switch to a raw ntuple of bytes that's entirely accessed via properties.
#=
abstract type AbstractOglBlock end

padding_size(b::AbstractOglBlock) = padding_size(typeof(b))
padding_size(T::Type{<:AbstractOglBlock}) = error("Not implemented: ", T)

base_alignment(b::AbstractOglBlock) = base_alignment(typeof(b))
base_alignment(T::Type{<:AbstractOglBlock}) = error("Not implemented: ", T)

@inline Base.propertynames(a::AbstractOglBlock) = propertynames(typeof(a))
@generated function Base.:(==)(a::T, b::T)::Bool where {T<:AbstractOglBlock}
    output = :( true )
    for p_name in propertynames(T)
        qp = QuoteNode(p_name)
        output = :( $output && (getfield(a, $qp) == getfield(b, $qp)) )
    end
    return output
end
@generated function Base.hash(a::T, h::UInt)::UInt where {T<:AbstractOglBlock}
    output = quote end
    for p_name in propertynames(T)
        qp = QuoteNode(p_name)
        output = quote
            $output
            h = hash(getfield(a, $qp), h)
        end
    end
    return quote
        $output
        return h
    end
end

function Base.show(io::IO, a::AbstractOglBlock)
    print(io, typeof(a), '(')
    for (i, prop) in enumerate(propertynames(a))
        if i > 1
            print(io, ", ")
        end
        show(io, getproperty(a, prop))
    end
    print(io, ')')
end


abstract type OglBlock_std140 <: AbstractOglBlock end
abstract type OglBlock_std430 <: AbstractOglBlock end


#TODO: Support some kind of annotation for row-major matrices.
"""
Generates an immutable struct using OpenGL-friendly types,
    whose byte layout exactly follows the std140 standard in shader blocks.

Sample usage:
````
@std140 struct MyInnerUniformBlock
    f::Float32
    bools::vb4
    position_array::NTuple{12, v3f}
end

@std140 struct MyOuterUniformBlock
    i::Int32 # Be careful; 'Int' in Julia means Int64
    items::NTuple{5, MyInnerUniformBlock}
    b::Bool
end

const MY_UBO_DATA = MyOuterUniformBlock(
    3,
    ntuple(i -> zero(MyInnerUniformBlock), 5),
    true
)
println("i is: ", MY_UBO_DATA.i)
````
"""
macro std140(struct_expr)
    return block_struct_impl(struct_expr)
end
function block_struct_impl(struct_expr)
    if !Base.is_expr(struct_expr, :struct)
        error("Expected struct block, got: ", struct_expr)
    end

    # Parse the header.
    (is_mutable::Bool, struct_name, body::Expr) = struct_expr.args
    if !isa(struct_name, Symbol)
        error("std140 struct has invalid name: '", struct_name, "'")
    elseif is_mutable
        error("std140 struct '", struct_name, "' must not be mutable")
    end

    lines = [line for line in body.args if !isa(line, LineNumberNode)]

    # Generate per-field code.

    property_names = Symbol[ ]
    field_definitions = Expr[ ]
    constructor_params = [ ]
    constructor_values = [ ]
    getproperty_clauses = Expr[ ]
    max_field_base_alignment::Int = zero(Int)
    n_padding_fields::Int = 0
    n_padding_bytes::Int = 0
    n_total_size::Int = 0

    SCALAR_TYPES = Union{Scalar32, Scalar64, Bool}
    VECTOR_TYPES = Union{(
        Vec{n, t}
          for (n, t) in Iterators.product(1:4, union_types(SCALAR_TYPES))
    )...}
    MATRIX_TYPES = Union{(
        @Mat(c, r, f)
          for (c, r, f) in Iterators.product(1:4, 1:4, (Float32, Float64))
    )...}
    NON_ARRAY_TYPES = Union{SCALAR_TYPES, VECTOR_TYPES, MATRIX_TYPES, OglBlock_std140}

    function add_padding_field(n_bytes::Integer, core_name,
                               pad_value::UInt8 = UInt8(n_padding_fields + 1))
        n_padding_fields += 1
        n_total_size += n_bytes
        n_padding_bytes += n_bytes
        push!(field_definitions, :(
            $(Symbol(core_name, ": ", n_padding_fields))::NTuple{$n_bytes, UInt8}
        ))
        push!(constructor_values, :(
            ntuple(i -> $pad_value, Val($n_bytes))
        ))
    end
    function align_for_next_field(alignment_bytes::Int)
        max_field_base_alignment = max(max_field_base_alignment, alignment_bytes)

        missing_bytes = (alignment_bytes - (n_total_size % alignment_bytes))
        if missing_bytes < alignment_bytes # Only if not already aligned
            add_padding_field(missing_bytes, :PAD)
        end
    end
    function add_field(field_def, constructor_value, getproperty_clause, byte_size)
        push!(field_definitions, field_def)
        push!(constructor_values, constructor_value)
        push!(getproperty_clauses, getproperty_clause)
        n_total_size += byte_size
    end
    function check_field_errors(field_name, T, is_within_array::Bool = false)
        if T <: Vec
            if !(T <: VECTOR_TYPES)
                error("Invalid vector count or type in ", field_name, ": ", T)
            end
        elseif T <: Mat
            if !(T <: MATRIX_TYPES)
                error("Invalid matrix size or component type in ", field_name, ": ", T)
            end
        elseif isstructtype(T)
            if !(T <: OglBlock_std140)
                error("Non-std140 struct referenced by ", field_name, ": ", T)
            end
        elseif T <: NTuple
            if is_within_array
                error("No nested arrays allowed, for simplicity. Flatten field ", field_name)
            end
            check_field_errors(field_name, eltype(T), true)
        elseif T <: SCALAR_TYPES
            # Nothing to check
        else
            error("Unexpected type in field ", field_name, ": ", T)
        end
    end

    # Note that 'esc' is automatically inserted at the end; it isn't needed in this loop.
    for line in lines
        if isa(line, LineNumberNode)
            continue
        end
        if !Base.is_expr(line, :(::))
            error("Expected only field declarations ('a::B'). Got: ", line)
        end

        (field_name, field_type) = line.args
        if !isa(field_name, Symbol)
            error("Name of the field should be a simple token. Got: '", field_name, "'")
        end
        field_type = eval(field_type)
        if !isa(field_type, Type)
            error("Expected a concrete type for the field's value. Got: ", field_type)
        end

        check_field_errors(field_name, field_type)

        push!(property_names, field_name)
        push!(constructor_params, :( $field_name ))

        # Generate code for the field.
        quoted_name = QuoteNode(field_name)
        if field_type == Bool
            align_for_next_field(4)
            add_field(
                :( $field_name::UInt32 ),
                :( convert(Bool, $field_name) ),
                :( (name == $quoted_name) &&
                     return convert(Bool, getfield(s, $quoted_name)) ),
                4
            )
        elseif field_type <: VecB
            N = length(field_type)
            if !in(N, 1:4)
                error("Vector type must be 1D - 4D. Got ", N, "D in field ",
                        struct_name, ".", field_type)
            end

            align_for_next_field(4 * (1, 2, 4, 4)[N])
            add_field(
                :( $field_name::Vec{$N, UInt32} ),
                :( convert(Vec{$N, UInt32}($field_name)) ),
                :( (name == $quoted_name) &&
                     return Vec{$N, Bool}(getfield(s, $quoted_name)) ),
                N * 4
            )
        elseif field_type <: SCALAR_TYPES
            align_for_next_field(sizeof(field_type))
            add_field(
                :( $field_name::$field_type ),
                :( convert($field_type, $field_name) ),
                :( (name == $quoted_name) && return getfield(s, $quoted_name) ),
                sizeof(field_type)
            )
        elseif field_type <: VECTOR_TYPES
            (N, T) = field_type.parameters
            align_for_next_field(4 * (1, 2, 4, 4)[N])
            add_field(
                :( $field_name::$field_type ),
                :( convert($field_type, $field_name) ),
                :( (name == $quoted_name) &&
                     return getfield(s, $quoted_name) ),
                N * sizeof(T)
            )
        elseif field_type <: MATRIX_TYPES
            (C, R, F) = mat_params(field_type)
            ColumnVec = Vec{R, F}
            vec_alignment = max(sizeof(F) * (1, 2, 4, 4)[R],
                                sizeof(v4f))

            column_names = Symbol[ ]
            align_for_next_field(vec_alignment)
            for v_idx in 1:C
                column_name = Symbol(field_name, ": column: ", v_idx)
                push!(column_names, column_name)

                push!(field_definitions, :( $column_name::Vec{$R, $F} ))
                push!(constructor_values, :(
                    Vec{$R, $F}($field_name[:, $v_idx]...)
                ))
                n_total_size += sizeof(ColumnVec)

                align_for_next_field(vec_alignment)
            end

            push!(getproperty_clauses, :(
                $field_type($((
                    :( getfield(s, $(QuoteNode(c_name))) )
                      for c_name in column_names
                )...))
            ))
        elseif field_type <: OglBlock_std140
            align_for_next_field(base_alignment(field_type))
            add_field(
                :( $field_name::$field_type ),
                :( convert($field_type, $field_name) ),
                :( (name == $quoted_name) &&
                     return getfield(s, $quoted_name) ),
                sizeof(field_type)
            )
        elseif field_type <: ConstVector{<:NON_ARRAY_TYPES}
            element_count = tuple_length(field_type)
            T = field_type.parameters[1]

            if T == Bool
                element_alignment = sizeof(v4f)
                padded_type = NTuple{element_count, v4u}
                align_for_next_field(element_alignment)
                add_field(
                    :( $field_name::$padded_type ),
                    :( v4u(UInt32.($field_name), 0, 0, 0) ),
                    :( (name == $quoted_name) &&
                         return map(v -> Bool(v.x), getfield(s, $quoted_name)) ),
                    sizeof(padded_type)
                )
            elseif T <: VecB
                element_alignment = sizeof(v4f)
                component_count = length(T)
                extra_components = 4 - component_count
                padded_type = NTuple{element_count, v4u}
                align_for_next_field(element_alignment)
                add_field(
                    :( $field_name::$padded_type ),
                    :( ntuple(i -> vappend(convert(Vec{$component_count, UInt32},
                                                   $field_name[i]),
                                           zero(Vec{$extra_components, UInt32})),
                              Val($element_count)) ),
                    :( (name == $quoted_name) &&
                         return map(v -> map(Bool, v[1:$component_count]),
                                    getfield(s, $quoted_name)) ),
                    sizeof(padded_type)
                )
            elseif T <: SCALAR_TYPES
                element_alignment = max(sizeof(T), sizeof(v4f))
                extra_bytes = max(0, Int(sizeof(T)) - element_alignment)
                padded_element_type = Tuple{field_type, NTuple{extra_bytes, UInt8}}
                padded_type = NTuple{element_count, padded_element_type}
                align_for_next_field(element_alignment)
                add_field(
                    :( $field_name::$padded_type ),
                    :( ntuple(i -> ($field_name[i], ntuple(i->0x0, Val($extra_bytes))),
                              Val($element_count)) ),
                    :( (name == $quoted_name) &&
                         return map(v -> (v, ntuple(i->0x0, Val($extra_bytes))),
                                    getfield(s, $quoted_name)) ),
                    sizeof(padded_type)
                )
            else
                error("Unexpected type in std140 array '", field_name, "': ", T)
            end
        else
            error("Unexpected type in std140 struct: ", field_type)
        end
    end

    # Struct alignment is the largest field alignment, rounded up to a vec4 alignment.
    struct_alignment = begin
        (quotient, remainder) = divrem(max_field_base_alignment, sizeof(v4f))
        iszero(remainder) ?
            max_field_base_alignment :
            max_field_base_alignment + (sizeof(v4f) - remainder)
    end

    # Add padding to the struct to match its alignment.
    # Note that this incorrectly modifies 'max_field_base_alignment',
    #    but that variable isn't used past this point.
    align_for_next_field(struct_alignment)

    # Output the final code.
    struct_name = esc(struct_name)
    ret = quote
        struct $struct_name <: OglBlock_std140
            $(esc.(field_definitions)...)
            $struct_name($(esc.(constructor_params)...)) = new($(esc.(constructor_values)...))
        end

        # Properties:
        @inline Base.propertynames(::Type{$struct_name}) = tuple($(QuoteNode.(property_names)...))
        @inline Base.getproperty($(esc(:s))::$struct_name, $(esc(:name))::Symbol) = begin
            $(esc.(getproperty_clauses)...)
            ($(esc(:name)) in fieldnames($struct_name)) && error("Do not depend on internal fields of the std140 struct!")
            error("Unknown field name '", $(esc(:name)), "'")
        end

        # AbstractOglBlock interface:
        base_alignment(::Type{$struct_name}) = $struct_alignment
        padding_size(::Type{$struct_name}) = $n_padding_bytes
    end
    # println(ret, "\n\n\n")
    return ret
end

#TODO: @std430. Unfortunately it requires a distinction between top-level and nested data.

# std140 rules:
#=
    (1) If the member is a scalar consuming <N> basic machine units, the
        base alignment is <N>.

    (2) If the member is a two- or four-component vector with components
        consuming <N> basic machine units, the base alignment is 2<N> or
        4<N>, respectively.

    (3) If the member is a three-component vector with components consuming
        <N> basic machine units, the base alignment is 4<N>.

    (4) If the member is an array of scalars or vectors, the base alignment
        and array stride are set to match the base alignment of a single
        array element, according to rules (1), (2), and (3), and rounded up
        to the base alignment of a vec4. The array may have padding at the
        end; the base offset of the member following the array is rounded up
        to the next multiple of the base alignment.

    (5) If the member is a column-major matrix with <C> columns and <R>
        rows, the matrix is stored identically to an array of <C> column
        vectors with <R> components each, according to rule (4).

    (6) If the member is an array of <S> column-major matrices with <C>
        columns and <R> rows, the matrix is stored identically to a row of
        <S>*<C> column vectors with <R> components each, according to rule
        (4).

    (7) If the member is a row-major matrix with <C> columns and <R> rows,
        the matrix is stored identically to an array of <R> row vectors
        with <C> components each, according to rule (4).

    (8) If the member is an array of <S> row-major matrices with <C> columns
        and <R> rows, the matrix is stored identically to a row of <S>*<R>
        row vectors with <C> components each, according to rule (4).

    (9) If the member is a structure, the base alignment of the structure is
        <N>, where <N> is the largest base alignment value of any of its
        members, and rounded up to the base alignment of a vec4. The
        individual members of this sub-structure are then assigned offsets 
        by applying this set of rules recursively, where the base offset of
        the first member of the sub-structure is equal to the aligned offset
        of the structure. The structure may have padding at the end; the 
        base offset of the member following the sub-structure is rounded up
        to the next multiple of the base alignment of the structure.

    (10) If the member is an array of <S> structures, the <S> elements of
        the array are laid out in order, according to rule (9).
=#
export padding_size, base_alignment, @std140, @std430

=#