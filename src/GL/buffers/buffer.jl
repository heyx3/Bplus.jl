"""
A contiguous block of memory on the GPU,
   for storing any kind of data.
Most commonly used to store mesh vertices/indices, or other arrays of things.

For help with uploading a whole data structure to a buffer, see `@std140` and `@std430`.

UNIMPLEMENTED: Instances can be "mapped" to the CPU, allowing you to write/read them directly
   as if they were a plain C array.
This is often more efficient than setting the buffer data the usual way,
   e.x. you could read the mesh data from disk directly into this mapped memory.
"""
mutable struct Buffer <: AbstractResource
    handle::Ptr_Buffer
    byte_size::UInt64
    is_mutable_from_cpu::Bool

    function Buffer( byte_size::Integer, can_change_data_from_cpu::Bool,
                     recommend_storage_on_cpu::Bool = false
                   )::Buffer
        b = new(Ptr_Buffer(), 0, false)
        set_up_buffer(
            byte_size, can_change_data_from_cpu,
            nothing,
            recommend_storage_on_cpu,
            b
        )
        return b
    end
    function Buffer( can_change_data_from_cpu::Bool,
                     initial_elements::Contiguous{T},
                     ::Type{T} = eltype(initial_elements)
                     ;
                     recommend_storage_on_cpu::Bool = false,
                     contiguous_element_range::Interval{<:Integer} = Interval(
                        min=1,
                        size=contiguous_length(initial_elements, T)
                     )
                   )::Buffer where {T}
        @bp_check(isbitstype(T), "Can't make a GPU buffer of ", T)
        b = new(Ptr_Buffer(), 0, false)
        set_up_buffer(
            size(contiguous_element_range) * sizeof(T),
            can_change_data_from_cpu,
            contiguous_ref(initial_elements, T,
                           min_inclusive(contiguous_element_range)),
            recommend_storage_on_cpu,
            b
        )
        return b
    end
end

@inline function set_up_buffer( byte_size::I, can_change_data_from_cpu::Bool,
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
    if can_change_data_from_cpu
        flags |= GL_DYNAMIC_STORAGE_BIT
    end

    setfield!(output, :handle, handle)
    setfield!(output, :byte_size, UInt64(byte_size))
    setfield!(output, :is_mutable_from_cpu, can_change_data_from_cpu)

    glNamedBufferStorage(handle, byte_size,
                         exists(initial_byte_data) ?
                             initial_byte_data :
                             C_NULL,
                         flags)
end

Base.show(io::IO, b::Buffer) = print(io,
    "Buffer<",
    Base.format_bytes(b.byte_size),
    (b.is_mutable_from_cpu ? " Mutable" : ""),
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

"Uploads the given data into the buffer"
function set_buffer_data( b::Buffer,
                          new_elements::Contiguous,
                          T = eltype(new_elements)
                          ;
                          # Which part of the input array to read from
                          src_element_range::IntervalU = IntervalU(min=1, size=contiguous_length(new_elements, T)),
                          # Shifts the first element of the buffer's array to write to
                          dest_element_offset::UInt = zero(UInt),
                          # A byte offset, to be combined wth 'dest_element_offset'
                          dest_byte_offset::UInt = zero(UInt)
                        )
    @bp_check(b.is_mutable_from_cpu, "Buffer is immutable")
    @bp_check(max_inclusive(src_element_range) <= contiguous_length(new_elements, T),
              "Trying to upload a range of data beyond the input buffer")

    first_byte::UInt = dest_byte_offset + ((dest_element_offset) * sizeof(T))
    byte_size::UInt = sizeof(T) * size(src_element_range)
    last_byte::UInt = first_byte + byte_size - 1
    @bp_check(last_byte <= b.byte_size,
              "Trying to write past the end of the buffer: ",
                 "bytes ", first_byte, " => ", last_byte,
                 ", when there's only ", b.byte_size, " bytes")

    if byte_size >= 1
        ptr = contiguous_ref(new_elements, T, Int(min_inclusive(src_element_range)))
        glNamedBufferSubData(b.handle, first_byte, byte_size, ptr)
    end
end

"
Loads the buffer's data into the given array.
If given a type instead of an array,
   then a new array of that type is allocated and returned.
Note that counts are per-element, not per-byte.
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
    @bp_check(dest.is_mutable_from_cpu, "Destination buffer is immutable")

    glCopyNamedBufferSubData(src.handle, dest.handle,
                             src_byte_offset,
                             dest_byte_offset,
                             byte_size)
end

export set_buffer_data, get_buffer_data, copy_buffer


###############################
#       Automatic Layout      #
###############################

"Some kind of bitstype data, laid out in a way that OpenGL/GLSL can understand"
abstract type AbstractOglBlock end
abstract type OglBlock_std140 <: AbstractOglBlock end
abstract type OglBlock_std430 <: AbstractOglBlock end


#  Interface:  #

"Gets the amount of padding in the given struct, in bytes"
padding_size(b::AbstractOglBlock) = padding_size(typeof(b))
padding_size(T::Type{<:AbstractOglBlock}) = error("Not implemented: ", T)

base_alignment(b::AbstractOglBlock) = base_alignment(typeof(b))
base_alignment(T::Type{<:AbstractOglBlock}) = error("Not implemented: ", T)

"Gets the bytes of a block, as a tuple of `UInt8`"
raw_bytes(b::AbstractOglBlock)::ConstVector{UInt8} = getfield(b, Symbol("raw bytes"))

"Returns the type of a property"
@inline property_type(b::AbstractOglBlock, name::Symbol) = property_type(typeof(b), Val(name))
@inline property_type(T::Type{<:AbstractOglBlock}, name::Symbol) = property_type(T, Val(name))
@inline property_type(T::Type{<:AbstractOglBlock}, Name::Val) = error(T, " has no property '", val_type(Name))

property_types(b::AbstractOglBlock) = property_types(typeof(b))
property_types(T::Type{<:AbstractOglBlock}) = error(T, " didn't implement ", property_types)

"Returns the byte offset to a property"
@inline property_offset(b::AbstractOglBlock, name::Symbol) = property_offset(typeof(b), name)
@inline property_offset(T::Type{<:AbstractOglBlock}, name::Symbol) = property_offset(T, Val(name))
@inline property_offset(T::Type{<:AbstractOglBlock}, Name::Val) = error(T, " has no property '", val_type(Name), "'")

"Parameters for a declaration of an OpenGL block"
@kwdef struct GLSLBlockDecl
    # Optional name that the block's fields will be nested within.
    glsl_name::Optional{AbstractString} = nothing
    # The official name of the block outside of pure GLSL code.
    open_gl_name::AbstractString

    # The type of block. Usually "uniform" or "buffer".
    type::AbstractString

    # Extra arguments to the "layout(...)" section.
    layout_qualifiers::AbstractString = ""
    # Optional final block element representing a dynamically-sized array
    final_array_field::Optional{AbstractString} = nothing

    # Maps nested structs to their corresponding name in GLSL.
    # If a type isn't in this lookup, its Julia name is carried into GLSL.
    type_names::Dict{Type, String} = Dict{Type, String}()
end
"Gets a GLSL string declaring the given block type"
glsl_decl(b::AbstractOglBlock, params::GLSLBlockDecl = GLSLBlockDecl()) = glsl_decl(typeof(b), params)
function glsl_decl(T::Type{<:AbstractOglBlock}, params::GLSLBlockDecl = GLSLBlockDecl())
    layout_std = if T <: OglBlock_std140
                     "std140"
                 elseif T <: OglBlock_std430
                     "std430"
                 else
                     error(T)
                 end
    return "layout($layout_std, $(params.layout_qualifiers)) $(params.type) $(params.open_gl_name)
    {
        $((
            "$(glsl_type_decl(property_type(T, f), string(f), params.type_names)) $f;"
                for f in propertynames(T)
        )...)
    } $(exists(params.glsl_name) ? params.glsl_name : "") ;"
end


#  Implementations:  #

@inline Base.propertynames(a::AbstractOglBlock) = propertynames(typeof(a))
@inline Base.getproperty(a::AbstractOglBlock, name::Symbol) = getproperty(a, Val(name))
@inline Base.setproperty!(r::Ref{<:AbstractOglBlock}, name::Symbol, value) = setproperty!(r, Val(name), value)

glsl_type_decl(::Type{Bool}, name::String, struct_lookup::Dict{Type, String}) = "bool $name;"
glsl_type_decl(::Type{Int32}, name::String, struct_lookup::Dict{Type, String}) = "int $name;"
glsl_type_decl(::Type{Int64}, name::String, struct_lookup::Dict{Type, String}) = "int64 $name;"
glsl_type_decl(::Type{UInt32}, name::String, struct_lookup::Dict{Type, String}) = "uint $name;"
glsl_type_decl(::Type{UInt64}, name::String, struct_lookup::Dict{Type, String}) = "uint64 $name;"
glsl_type_decl(::Type{Float32}, name::String, struct_lookup::Dict{Type, String}) = "float $name;"
glsl_type_decl(::Type{Float64}, name::String, struct_lookup::Dict{Type, String}) = "double $name;"
glsl_type_decl(::Type{Vec{N, Bool}}, name::String, struct_lookup::Dict{Type, String}) where {N} = "bvec$N $name;"
glsl_type_decl(::Type{Vec{N, Int32}}, name::String, struct_lookup::Dict{Type, String}) where {N} = "ivec$N $name;"
glsl_type_decl(::Type{Vec{N, UInt32}}, name::String, struct_lookup::Dict{Type, String}) where {N} = "uvec$N $name;"
glsl_type_decl(::Type{Vec{N, Float32}}, name::String, struct_lookup::Dict{Type, String}) where {N} = "vec$N $name;"
glsl_type_decl(::Type{Vec{N, Float64}}, name::String, struct_lookup::Dict{Type, String}) where {N} = "dvec$N $name;"
glsl_type_decl(::Type{<:Mat{C, R, Float32}}, name::String, struct_lookup::Dict{Type, String}) where {C, R} = "mat$Cx$R $name;"
glsl_type_decl(::Type{<:Mat{C, R, Float64}}, name::String, struct_lookup::Dict{Type, String}) where {C, R} = "dmat$Cx$R $name;"
glsl_type_decl(T::Type{<:AbstractOglBlock}, name::String, struct_lookup::Dict{Type, String}) = get(struct_lookup, T, string(T))
glsl_type_decl(::Type{NTuple{N, T}}, name::String, struct_lookup::Dict{Type, String}) where {N, T} = "$(glsl_type_decl(T, struct_lookup)) $name[$N]"


#NOTE: I'm pretty sure getproperty and setproperty could be implemented as normal, generic functions.
# However, once this code works, it shouldn't need any maintenance
#          as it's attached to a strict standard.

# Functions which generate code to memcpy the property data
#    into and out of the block struct's bytes.
function block_macro_get_property_body(T_expr, property_name::Symbol,
                                       property_byte_offset::Int,
                                       property_type::Type,
                                       mode::Symbol)::Expr
    mode_switch(std140, std430) = if mode == :std140
                                      std140()
                                  elseif mode == :std430
                                      std430()
                                  else
                                      error("Unexpected mode: ", mode)
                                  end
    base_type::Type{<:AbstractOglBlock} = mode_switch(() -> OglBlock_std140,
                                                      () -> OglBlock_std430)
    # Emits code that loads data from the block:
    @inline raw_value(; offset=property_byte_offset, t=property_type) =
        :( reinterpret_bytes(raw_bytes(a), $offset, $t) )

    # Booleans are 4 bytes on the GPU.
    if property_type == Bool
        return :(
            convert(Bool, $(raw_value(t=UInt32)))
        )
    elseif property_type <: VecT{Bool}
        N = length(property_type)
        return :(
            convert(Vec{$N, Bool}, $(raw_value(t=Vec{N, UInt32})))
        )
    # Non-bool scalars, vectors, and nested structs are trivially readable.
    elseif property_type <: Union{ScalarBits, Vec, base_type}
        return raw_value()
    # Matrices are stored like an array of vectors.
    elseif property_type <: Mat
        (C, R, F) = mat_params(property_type)

        column_stride_bytes = sizeof(F) * (1, 2, 4, 4)[R]
        column_stride_bytes = mode_switch(
            () -> round_up_to_multiple(column_stride_bytes, sizeof(v4f)),
            () -> column_stride_bytes
        )

        column_stride_elements = column_stride_bytes ÷ sizeof(F)
        component_count = column_stride_elements * C

        let_block = :( let floats = $(raw_value(t=NTuple{component_count, F}))
        end )
        columns = [ ]
        for col in 1:C
            start_idx = 1 + (column_stride_elements * (col - 1))
            end_idx = start_idx + R - 1
            push!(columns, :( floats[$start_idx:$end_idx]... ))
        end
        push!(let_block.args[2].args, :(
            return @Mat($C, $R, $F)($(columns...))
        ))

        return let_block
    # Array elements have a certain calculated stride (at least sizeof(v4f) in std140).
    elseif property_type <: NTuple
        array_N = tuple_length(property_type)
        array_T = eltype(property_type)

        # Emits code that loads an array of data from the block:
        function raw_values(; element_t = array_T,
                              element_stride_bytes = begin
                                  element_size_t = if element_t <: Vec3
                                                       sizeof(Vec{4, eltype(element_t)})
                                                   else
                                                       sizeof(element_t)
                                                   end
                                  mode_switch(
                                      () -> round_up_to_multiple(element_size_t, sizeof(v4f)),
                                      () -> element_size_t
                                  )
                              end,
                              byte_offset = 0,
                              element_expr_postprocess = identity)
            return :( let bytes = raw_bytes(a)
                tuple(
                    $((
                        element_expr_postprocess(:(
                            reinterpret_bytes(bytes,
                                              $(property_byte_offset + byte_offset +
                                                (element_stride_bytes * (array_idx-1))),
                                              $element_t)
                        )) for array_idx in 1:array_N
                    )...)
                )
            end )
        end

        # Unfortunately, the behavior is hard to generalize for all types of arrays.
        if array_T == Bool
            return :( Bool.($(raw_values(element_t=UInt32))) )
        elseif array_T <: VecT{Bool}
            vec_N = length(array_T)
            vec_us = raw_values(element_t=Vec{vec_N, UInt32})
            return :( convert.(Ref(Vec{$vec_N, Bool}), $vec_us) )
        elseif array_T <: Union{ScalarBits, Vec, base_type}
            # Note that nested structs are already padded in their size
            #    to take up a multiple of their alignment.
            # This means their stride is always equal to their size.
            return raw_values()
        elseif array_T <: Mat
            (C, R, F) = mat_params(array_T)

            column_stride_bytes = sizeof(F) * (1, 2, 4, 4)[R]
            column_stride_bytes = mode_switch(
                () -> round_up_to_multiple(column_stride_bytes, sizeof(v4f)),
                () -> column_stride_bytes
            )

            matrix_stride_bytes = column_stride_bytes * C

            # For each column of the matrix type,
            #    gather an array of that specific column.
            # Then, broadcast-construct all arrays at once.
            function matrix_columns_expr(col)
                raw_values(element_t=Vec{R, F},
                           element_stride_bytes=matrix_stride_bytes,
                           byte_offset=(col - 1) * column_stride_bytes)
            end
            column_exprs = (matrix_columns_expr(i) for i in 1:C)
            return quote
                @inline constructor($((Symbol(:column, i) for i in 1:C)...)) =
                    @Mat($C, $R, $F)($((:( $(Symbol(:column, i))... ) for i in 1:C)...))
                constructor.($(column_exprs...))
            end
        else
            return :( error("Unexpected array element type: ", $T_expr, "::", $array_T) )
        end
    else
        return :( error("Unexpected type: ", $T_expr, "::", $property_type) )
    end
end
# The setproperty version assumes the instance being set is hidden inside a Ref[].
function block_macro_set_property_body(T_expr, property_name::Symbol,
                                       property_byte_offset::Int,
                                       property_type::Type,
                                       mode::Symbol)::Expr
    name_str = string(property_name)
    mode_switch(std140, std430) = if mode == :std140
                                      std140()
                                  elseif mode == :std430
                                      std430()
                                  else
                                      error("Unexpected mode: ", mode)
                                  end
    base_type::Type{<:AbstractOglBlock} = mode_switch(() -> OglBlock_std140,
                                                      () -> OglBlock_std430)

    # Emits code that stores data into the block:
    function store_value_expr(;
                              value_expr=:( convert($property_type, value) ),
                              byte_offset=property_byte_offset)
        return :( let value = $value_expr
            ptr_base = Base.unsafe_convert(Ptr{$T_expr}, r)
            ptr_data = Ptr{typeof(value)}(ptr_base) + $byte_offset
            GC.@preserve r unsafe_store!(ptr_data, value)
        end )
    end

    # Booleans are 4 bytes on the GPU.
    if property_type == Bool
        return store_value_expr(value_expr=:( UInt32(value) ))
    elseif property_type <: VecT{Bool}
        return store_value_expr(value_expr=:( map(UInt32, value) ))
    # Non-bool scalars, vectors, and nested structs are trivially writable.
    elseif property_type <: Union{ScalarBits, Vec, base_type}
        return store_value_expr()
    # Matrices are stored like an array of vectors.
    elseif property_type <: Mat
        (C, R, F) = mat_params(property_type)
        column_stride_bytes = sizeof(F) * (1, 2, 4, 4)[R]
        column_stride_bytes = mode_switch(
            () -> round_up_to_multiple(column_stride_bytes, sizeof(v4f)),
            () -> column_stride_bytes
        )

        column_exprs = map(1:C) do col::Int
            column_byte_offset = column_stride_bytes * (col - 1)
            return store_value_expr(value_expr=:( convert.(Ref($F), value[:, $col]) ),
                                    byte_offset=property_byte_offset + column_byte_offset)
        end
        return quote
            $(column_exprs...)
        end
    # Array elements have a calculated stride (at least sizeof(v4f) in std140).
    elseif property_type <: NTuple
        array_N = tuple_length(property_type)
        array_T = eltype(property_type)
        @bp_check(array_N > 0,
                  "This code assumes at least one element for array field '", property_name, "'")

        # Generates one expression per element index (represented as the emitted variable 'IDX'),
        #    and returns the expressions in a code block.
        function do_per_element_expr(element_IDX_expr)
            exprs = quote end
            append!(exprs.args, element_IDX_expr.(1:array_N))
            return exprs
        end
        # Generates a block of expressions that sets each element of the array,
        #    using the given code to generate values and byte offsets from a tuple of indices.
        function set_per_element_expr(; raw_element_t = array_T,
                                        expr_values_from_idcs = :( convert.(Ref($raw_element_t), value) ),
                                        expr_byte_offsets_from_idcs = begin
                                            element_size_t = if raw_element_t <: Vec3
                                                                 sizeof(Vec4{eltype(raw_element_t)})
                                                             else
                                                                 sizeof(raw_element_t)
                                                             end
                                            element_stride = mode_switch(
                                                () -> round_up_to_multiple(element_size_t, sizeof(v4f)),
                                                () -> element_size_t
                                            )
                                            :( (idcs .- 1) .* $element_stride )
                                        end
                                     )::Expr
            setup = quote
                ptr_base = Base.unsafe_convert(Ptr{$T_expr}, r)
                ptr_data = Ptr{$raw_element_t}(ptr_base) + $property_byte_offset
            end

            #NOTE: It's possible to optimize this in the case where
            #    the element stride matches the element size, by doing
            #    one big store rather than many small ones.
            #      However, I don't think the extra complexity is worth it.
            execution = :( let idcs = tuple((1:$array_N)...),
                               values = begin
                                   $expr_values_from_idcs
                               end,
                               byte_offsets = $expr_byte_offsets_from_idcs
                unsafe_store!.(ptr_data .+ byte_offsets, values)
                nothing
            end )

            return quote
                $setup
                $execution
            end
        end

        # Unfortunately, the behavior is hard to generalize for all types of arrays.
        # Bools are 4 bytes on the GPU, so we'll cast them to UInt32:
        if array_T == Bool
            return set_per_element_expr(;
                raw_element_t = UInt32,
                expr_values_from_idcs = :( UInt32.(value) )
            )
        elseif array_T <: VecT{Bool}
            return set_per_element_expr(;
                raw_element_t = Vec{length(array_T), UInt32},
                expr_values_from_idcs = quote
                    map.(Ref(UInt32), value)
                end
            )
        # Non-bool scalars and vectors, and nested structs, are trivial to set:
        elseif array_T <: Union{ScalarBits, Vec, base_type}
            # Note that nested structs are already padded in their size
            #    to take up a multiple of their alignment.
            # This means their stride is always equal to their size.
            return set_per_element_expr()
        # Matrices are treated like arrays of column vectors:
        elseif array_T <: Mat
            (C, R, F) = mat_params(array_T)

            column_stride_bytes = sizeof(F) * (1, 2, 4, 4)[R]
            column_stride_bytes = mode_switch(
                () -> round_up_to_multiple(column_stride_bytes, sizeof(v4f)),
                () -> column_stride_bytes
            )

            matrix_stride_bytes = column_stride_bytes * C

            # For each column of the matrix type,
            #    grab and store an array of that specific column.
            column_exprs = map(1:C) do col::Int
                column_byte_offset = (col - 1) * column_stride_bytes
                set_per_element_expr(;
                    raw_element_t = Vec{R, F},
                    expr_values_from_idcs = quote
                        matrices = getindex.(Ref(value), idcs)
                        columns = getindex.(matrices, Ref(:), $col)
                        convert.(Ref(Vec{$R, $F}), columns)
                    end,
                    expr_byte_offsets_from_idcs = :( begin
                        ((idcs .- 1) .* $matrix_stride_bytes) .+ $column_byte_offset
                    end )
                )
            end
            return quote
                $(column_exprs...)
            end
        else
            error("Unexpected array element type in field '", name_str, ": ", T_expr)
        end
    else
        error("Unexpected type of field '", name_str, "': ", T_expr)
    end
end

"Internal constructor for a buffer block (i.e. std140, std430)"
function construct_block(T::Type{<:AbstractOglBlock}, properties...)::T
    # Store an instance in a Ref, mutate its fields, then return it.
    let ref = Ref(T(ntuple(i -> zero(UInt8), Val(sizeof(T)))))
        setproperty!.(Ref(ref), propertynames(T), properties)
        return ref[]
    end
end


function Base.:(==)(a::T, b::T)::Bool where {T<:AbstractOglBlock}
    return all(Base.:(==).(
        getproperty.(Ref(a), propertynames(T)),
        getproperty.(Ref(b), propertynames(T))
    ))
end

function Base.hash(a::T, h::UInt)::UInt where {T<:AbstractOglBlock}
    return hash(
        getproperty.(Ref(a), propertynames(T)),
        h
    )
end

function Base.show(io::IO, a::AbstractOglBlock)
    print(io, Base.typename(typeof(a)).name, '(')
    for (i, prop) in enumerate(propertynames(a))
        if i > 1
            print(io, ", ")
        end
        show(io, getproperty(a, prop))
    end
    print(io, ')')
end


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

const MUTABLE_UBO = Ref(MyInnerUniformBlock(3.5,
                                            vb4(false, false, true, true),
                                            ntuple(i -> zero(v3f))))
MUTABLE_UBO.f = 3.4f0
println("f is: ", MUTABLE_UBO[].f)
````
"""
macro std140(struct_expr)
    return block_struct_impl(struct_expr, :std140, __module__)
end
"""
Generates an immutable struct using OpenGL-friendly types,
    whose byte layout exactly follows the std430 standard in shader blocks.

Sample usage:
````
@std430 struct MyInnerShaderStorageBlock
    f::Float32
    bools::vb4
    position_array::NTuple{12, v3f}
end

@std430 struct MyOuterShaderStorageBlock
    i::Int32 # Be careful; 'Int' in Julia means Int64
    items::NTuple{5, MyInnerShaderStorageBlock}
    b::Bool
end

const MY_SSBO_DATA = MyOuterShaderStorageBlock(
    3,
    ntuple(i -> zero(MyInnerShaderStorageBlock), 5),
    true
)
println("i is: ", MY_SSBO_DATA.i)

const MUTABLE_SSBO = Ref(MyInnerShaderStorageBlock(3.5,
                                                   vb4(false, false, true, true),
                                                   ntuple(i -> zero(v3f))))
MUTABLE_SSBO.f = 3.4f0
println("f is: ", MUTABLE_SSBO[].f)
````
"""
macro std430(struct_expr)
    return block_struct_impl(struct_expr, :std430, __module__)
end
function block_struct_impl(struct_expr, mode::Symbol, invoking_module::Module)
    if !Base.is_expr(struct_expr, :struct)
        error("Expected struct block, got: ", struct_expr)
    end

    mode_switch(std140, std430) = if mode == :std140
                                      std140()
                                  elseif mode == :std430
                                      std430()
                                  else
                                      error("Unexpected mode: ", mode)
                                  end
    base_type::Type{<:AbstractOglBlock} = mode_switch(() -> OglBlock_std140,
                                                      () -> OglBlock_std430)

    SCALAR_TYPES = Union{Scalar32, Scalar64, Bool}
    VECTOR_TYPES = Union{(
        Vec{n, t}
          for (n, t) in Iterators.product(1:4, union_types(SCALAR_TYPES))
    )...}
    MATRIX_TYPES = Union{(
        @Mat(c, r, f)
          for (c, r, f) in Iterators.product(1:4, 1:4, (Float32, Float64))
    )...}
    NON_ARRAY_TYPES = Union{SCALAR_TYPES, VECTOR_TYPES, MATRIX_TYPES, base_type}
    ARRAY_TYPES = ConstVector{<:NON_ARRAY_TYPES}

    # Parse the header.
    (is_mutable_from_cpu::Bool, struct_name, body::Expr) = struct_expr.args
    if !isa(struct_name, Symbol)
        error(mode, " struct has invalid name: '", struct_name, "'")
    elseif is_mutable_from_cpu
        error(mode, " struct '", struct_name, "' must not be mutable")
    end

    # Parse the body.
    lines = [line for line in body.args if !isa(line, LineNumberNode)]
    function check_field_errors(field_name, T, is_within_array::Bool = false)
        if T <: Vec
            if !(T <: VECTOR_TYPES)
                error("Invalid vector count or type in ", field_name, ": ", T)
            elseif T <: Vec3
                error("Problem with field ", field_name,
                        ": 3D vectors are not padded correctly by every graphics driver, ",
                        "and their size is almost always padded out to 4D anyway, so just use ",
                        "a 4D vector")
            end
        elseif T <: Mat
            if !(T <: MATRIX_TYPES)
                error("Invalid matrix size or component type in ", field_name, ": ", T)
            elseif (T <: Mat{C, 3} where {C})
                error("Problem with field ", field_name,
                        ": 3D vectors (and by extension, 3-row matrices) are not padded correctly ",
                        "by every graphics driver. Their size gets padded out to 4D anyway, ",
                        "so just use a 4-row matrix.")
            end
        elseif T <: NTuple
            if is_within_array
                error("No nested arrays allowed, for simplicity. Flatten field ", field_name)
            end
            check_field_errors(field_name, eltype(T), true)
        elseif isstructtype(T) # Note that NTuple is a 'struct type', so
                               #    we have to handle the NTuple case first
            if !(T <: base_type)
                error("Non-", mode, " struct referenced by ", field_name, ": ", T)
            end
        elseif T <: SCALAR_TYPES
            # Nothing to check
        else
            error("Unexpected type in field ", field_name, ": ", T)
        end
    end
    field_definitions = map(lines) do line
        if !Base.is_expr(line, :(::))
            error("Expected only field declarations ('a::B'). Got: ", line)
        end

        (field_name, field_type) = line.args
        if !isa(field_name, Symbol)
            error("Name of the field should be a simple token. Got: '", field_name, "'")
        end
        field_type = invoking_module.eval(field_type)
        if !isa(field_type, Type)
            error("Expected a concrete type for the field's value. Got: ", field_type)
        end

        check_field_errors(field_name, field_type)

        return (field_name, field_type)
    end

    # Figure out padding and field offsets.
    total_byte_size::Int = 0
    total_padding_bytes::Int = 0
    max_field_alignment::Int = 0
    property_offsets = Vector{Int}()
    function align_next_field(alignment, record_offset = true)
        max_field_alignment = max(max_field_alignment, alignment)

        missing_bytes = (alignment - (total_byte_size % alignment))
        if missing_bytes < alignment # Only if not already aligned
            total_byte_size += missing_bytes
            total_padding_bytes += missing_bytes
        end

        if record_offset
            push!(property_offsets, total_byte_size)
        end
    end
    for (field_name, field_type) in field_definitions
        if field_type == Bool
            align_next_field(4)
            total_byte_size += 4
        elseif field_type <: VecT{Bool}
            n_components = length(field_type)
            byte_size = 4 * n_components
            alignment = 4 * (1, 2, 4, 4)[n_components]

            align_next_field(alignment)
            total_byte_size += byte_size
        elseif field_type <: ScalarBits
            byte_size = sizeof(field_type)
            alignment = byte_size

            align_next_field(alignment)
            total_byte_size += byte_size
        elseif field_type <: Vec
            n_components = length(field_type)
            component_type = eltype(field_type)
            byte_size = sizeof(component_type) * n_components
            alignment = sizeof(component_type) * (1, 2, 4, 4)[n_components]

            align_next_field(alignment)
            total_byte_size += byte_size
        elseif field_type <: Mat
            (C, R, F) = mat_params(field_type)

            column_alignment = sizeof(F) * (1, 2, 4, 4)[R]
            column_alignment = mode_switch(
                () -> round_up_to_multiple(column_alignment, sizeof(v4f)),
                () -> column_alignment
            )

            byte_size = C * column_alignment
            alignment = column_alignment

            align_next_field(alignment)
            total_byte_size += byte_size
        elseif field_type <: base_type
            byte_size = sizeof(field_type)
            alignment = base_alignment(field_type)

            align_next_field(alignment)
            total_byte_size += byte_size
        elseif field_type <: NTuple
            array_length = tuple_length(field_type)
            element_type = eltype(field_type)

            (element_alignment, element_stride) =
                if element_type <: SCALAR_TYPES
                    size = (element_type == Bool) ? 4 : sizeof(element_type)
                    mode_switch(
                        () -> (size = round_up_to_multiple(size, sizeof(v4f))),
                        () -> nothing
                    )
                    (size, size)
                elseif element_type <: Vec
                    component_size = (eltype(element_type) == Bool) ?
                                            4 :
                                            sizeof(eltype(element_type))
                    vec_size = (1, 2, 4, 4)[length(element_type)] * component_size
                    mode_switch(
                        () -> (vec_size = round_up_to_multiple(vec_size, sizeof(v4f))),
                        () -> nothing
                    )
                    (vec_size, vec_size)
                elseif element_type <: Mat
                    (C, R, F) = mat_params(element_type)
                    column_size = sizeof(F) * (1, 2, 4, 4)[R]
                    mode_switch(
                        () -> (column_size = round_up_to_multiple(column_size, sizeof(v4f))),
                        () -> nothing
                    )
                    (column_size, column_size * C)
                elseif element_type <: base_type
                    # Any @std140/@std430 struct will already be padded to the right size,
                    #    but the padding logic is still here for completeness.
                    (a, s) = (base_alignment(element_type), sizeof(element_type))
                    mode_switch(
                        () -> (round_up_to_multiple(a, sizeof(v4f)),
                               round_up_to_multiple(s, sizeof(v4f))),
                        () -> (a, s)
                    )
                else
                    error("Unhandled case: ", element_type)
                end

            align_next_field(element_alignment)
            total_byte_size += element_stride * array_length
        else
            error("Unhandled: ", field_type)
        end
    end

    # Struct alignment is the largest field alignment, then in std140 rounded up to a vec4 alignment.
    struct_alignment = mode_switch(
        () -> round_up_to_multiple(max_field_alignment, sizeof(v4f)),
        () -> max_field_alignment
    )

    # Add padding to the struct to match its alignment.
    # Note that this inadvertently modifies 'max_field_alignment',
    #    but that variable isn't used past this point.
    align_next_field(struct_alignment, false)

    # Generate the final code.
    struct_name_str = string(struct_name)
    struct_name = esc(struct_name)
    (property_names, property_types) = unzip(field_definitions, 2)
    property_functions = map(zip(field_definitions, property_offsets)) do ((name, type), offset)
        compile_time_name = :( Val{$(QuoteNode(name))} )
        return quote
            $(@__MODULE__).property_offset(::Type{$struct_name}, ::$compile_time_name) = $offset
            $(@__MODULE__).property_type(::Type{$struct_name}, ::$compile_time_name) = $type

            $(esc(:Base)).getproperty(a::$struct_name, ::$compile_time_name) =
                $(block_macro_get_property_body(struct_name, name, offset, type, mode))
            $(esc(:Base)).setproperty!(r::Ref{$struct_name}, ::$compile_time_name, value) =
                $(block_macro_set_property_body(struct_name, name, offset, type, mode))
        end
    end
    return quote
        Core.@__doc__ struct $struct_name <: $base_type
            var"raw bytes"::NTuple{$total_byte_size, UInt8}
        end
        @bp_check(sizeof($struct_name) == $total_byte_size,
                  $struct_name_str, " should be ", $total_byte_size,
                    " bytes but it was changed by Julia to ", sizeof($struct_name))

        Base.propertynames(::Type{$struct_name}) = tuple($(QuoteNode.(property_names)...))
        $(@__MODULE__).property_types(::Type{$struct_name}) = tuple($(property_types...))
        $(property_functions...)

        $(@__MODULE__).padding_size(::Type{$struct_name}) = $total_padding_bytes
        $(@__MODULE__).base_alignment(::Type{$struct_name}) = $struct_alignment

        $struct_name($(esc.(property_names)...)) = $(@__MODULE__).construct_block($struct_name, $(esc.(property_names)...))
    end
end

export @std140, @std430
       padding_size, raw_bytes,
       glsl_decl, GLSLBlockDecl