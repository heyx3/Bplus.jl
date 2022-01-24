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
Gets a `Ref` to some contiguous data.
For example, `contiguous_ref(tuple(v2f(1, 2), v2f(3, 4)), Float32)`
    gets a reference to the value `1`.

Note that the ref's type is not necessarily the type of the element `T`;
    use the function `contiguous_ptr()` to get the correctly-typed pointer to a contiguous Ref.
"
contiguous_ref(x::Contiguous{T}, ::Type{T}) where {T} = Ref(x, 1)
contiguous_ref(x::Union{T, VecT{<:ContiguousRaw{T}}, ConstVector{<:ContiguousRaw{T}}}, ::Type{T}) where {T} = Ref(x)

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
function contiguous_ptr(r::Ref, ::Type{T}, i::Int = 1) where {T}
    ptr = Base.unsafe_convert(Ptr{Nothing}, r)
    return Base.unsafe_convert(Ptr{T}, ptr) +
           ((i - 1) * sizeof(T))
end