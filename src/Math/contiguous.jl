# This definition belongs in Utilities,
#    but it requires the existence of Vec.

export Contiguous, ContiguousSimple, ContiguousRaw,
       contiguous_length, contiguous_ref, contiguous_ptr


###############
##  Aliases  ##
###############

"
Collections which can be contiguous in memory, even when nested arbitrarily-many times.
Includes the type itself, i.e. `Int <: ContiguousRaw{Int}`.
"
const ContiguousRaw{T} = Union{
    T,
    SArray{S, T} where {S},
    Vec{N, T} where {N},
    ConstVector{T}
}

"
A flat, contiguous collection of some type `T`.
Includes the type itself, i.e. `Int <: ContiguousSimple{Int}`.
"
const ContiguousSimple{T} = Union{
    ContiguousRaw{T},
    MArray{S, T} where {S},
    StridedArray{T},
}

"
Any kind of contiguous data, suitable for passing into a C function as a raw pointer.
Includes everything from a single bare element, to something like
    `Vector{NTuple{4, v3f}}`.

It's not actually possible to explicitly list the infinite variety of possible nestings,
so this union currently supports up to 3 levels, which should be enough.
"
const Contiguous{T} = Union{
    ContiguousSimple{T},
    ContiguousSimple{<:ContiguousRaw{T}},
    ContiguousSimple{<:ContiguousRaw{<:ContiguousRaw{T}}},
    ContiguousSimple{<:ContiguousRaw{<:ContiguousRaw{<:ContiguousRaw{T}}}}
}


#################
##  Functions  ##
#################

"
Gets the total length of a contiguous array of `T` data, regardless of how deeply it's nested.

Note that there's an implementation limit on how deeply-nested the array can really be;
    see `Contiguous{T}` if you get MethodErrors.
"
contiguous_length(x::T, ::Type{T}) where {T} = 1
contiguous_length(x::ContiguousSimple{T}, ::Type{T}) where {T} = length(x)
contiguous_length(x::Contiguous{T}, ::Type{T}) where {T} =
    if isempty(x)
        0
    else
        length(x) * contiguous_length(x[1], T)
    end
#

"
Gets a `Ptr{T}` to an element of a contiguous array of `T` data,
    regardless of how deeply it's nested.
Takes an index into the element (defaulting to 1), but be aware
    that this is on top of the Ref itself!
If the Ref wasn't generated with `contiguous_ref()`,
    then it may already be offsetting from the first element.

Note that there's an implementation limit on how deeply-nested the array can really be;
    see `Contiguous{T}` if you get MethodErrors.
"
function contiguous_ptr(r::Ref{T2}, ::Type{T}, i::Int = 1) where {T, T2}
    # Make sure the reference is to a contiguous chunk of T data.
    # NOTE: I originally had this in the method signature, `r::Ref{<:Contiguous{T}}`,
    #    but then Julia fails to call this overload for some reason.
    @bp_check(T2 <: Contiguous{T}, "Reference to ", T2, " isn't a Contiguous{", T, "}")

    ptr = Base.unsafe_convert(Ptr{Nothing}, r)
    ptr += (i - 1) * sizeof(T)
    return Base.unsafe_convert(Ptr{T}, ptr)
end


#################
##  Reference  ##
#################

"
Ref to a contiguous set of data, possibly nested (e.x. `Float32`s in a `Vector{NTuple{4, v3f}}`).
Note that the index is in terms of the individual contiguous elements.
"
mutable struct ContiguousRef{TEl, TCollection<:Contiguous{TEl}} <: Ref{TEl}
    data::TCollection
    element_idx::UInt
end
@inline contiguous_ref(collection, ::Type{T}, idx::Integer = 1) where{T} = ContiguousRef{T, typeof(collection)}(collection, UInt(idx))

function Base.unsafe_convert( P::Union{Type{Ptr{TEl}}, Type{Ptr{Cvoid}}},
                              r::ContiguousRef{TEl, TC}
                            )::P where {TEl, TC}
    @bp_math_assert(isbitstype(TEl), "Can't have a contiguous reference to non-bits types")

    n_elements::Int = contiguous_length(r.data, TEl)
    @boundscheck(@bp_check(r.element_idx in 1:n_elements))

    # Keep in mind that in Julia, pointer arithmetic is always in bytes,
    #    regardless of the pointer's type.
    byte_offset::UInt = convert(UInt, sizeof(TEl)) * (r.element_idx - 1)

    if isbitstype(TC)
        data_field_idx = Base.fieldindex(typeof(r), :data)
        return pointer_from_objref(r) +
               fieldoffset(typeof(r), data_field_idx) +
               byte_offset
    else
        return Ptr{TEl}(pointer(r.data)) +
               byte_offset
    end
end