"A static, type-stable container for 0 - N elements, to store return values"
struct UpTo{N, T} <: AbstractVector{T}
    buffer::NTuple{N, T}
    count::Int
end
export UpTo

"Construct an UpTo with no elements"
function UpTo{N, T}(undef_init::T = zero(T)) where {N, T}
    return UpTo{N, T}(
        ntuple(i -> undef_init, N),
        0
    )
end
"Construct an UpTo with the given set of elements"
function UpTo{N, T}( elements::NTuple{N2, T};
                     undef_init::T = zero(T)
                   )::UpTo{N, T} where {N, T, N2}
    @bp_check(N2 <= N, "Giving too many elements to create an UpTo{", N, ", ", T, "}: ", N2)
    return UpTo{N, T}(
        ntuple(i-> (i <= N2) ? elements[i] : undef_init,
               N),
        N2
    )
end

# Convert zero or more elements to an UpTo automatically.
function Base.convert( ::Type{UpTo{N, T}},
                       t::Tuple{},
                       undef_init::T = zero(T)
                     ) where {N, T}
    return UpTo{N, T}(ntuple(i -> undef_init, N), 0)
end
function Base.convert( ::Type{UpTo{N, T}},
                       t::T,
                       undef_init::T = zero(T)
                     ) where {N, T}
    return UpTo{N, T}((t, ); undef_init=undef_init)
end
function Base.convert( ::Type{UpTo{N, T}},
                       t::NTuple{N2, T},
                       undef_init::T = zero(T)
                     ) where {N, T, N2}
    return UpTo{N, T}(t; undef_init=undef_init)
end

# By defalt, show() will make it look like an array, which confused the hell out of me
Base.show(io::IO, u::UpTo) = print(io,
    typeof(u), "(tuple(",
    iter_join(Iterators.take(u.buffer, u.count), ", ")...,
    "))"
)

# Equality with raw tuples and single elements.
@inline Base.:(==)(a::UpTo, t::Tuple) = (length(t) == length(a)) && (all(i -> a[i]==t[i], 1:length(a)))
@inline Base.:(==)(a::UpTo{N, T}, t::T) where {N, T} = (length(a) == 1) && (a[1] == t)

# The ability to compare with tuples *and* elements means that hashing won't work,
#    because a 1-tuple has a different hash than its element.
Base.hash(::UpTo, args...) = error("Can't hash type 'UpTo', due to how it defines equality")

Base.size(a::UpTo) = (a.count, )
Base.getindex(a::UpTo, i::Int) = a.buffer[i]
Base.IndexStyle(::Type{<:UpTo}) = IndexLinear()

# Append two instances together:
function append( a::UpTo{N1, T1},
                 b::UpTo{N2, T2},
                 undef_init = zero(promote_type(T1, T2))
               )::UpTo{N1 + N2, promote_type(T1, T2)} where {N1, T1,  N2, T2}
    T3 = promote_type(T1, T2)
    new_elements = ntuple(Val(N1 + N2)) do i::Int
        convert(T3,
            if i <= a.count
                a[i]
            elseif i - a.count <= b.count
                b[i - a.count]
            else
                undef_init
            end
        )
    end
    return UpTo{N1 + N2, T3}(new_elements, a.count + b.count)
end
export append

# UpTo is meant for reading small return data,
#    so any attempt to convert it to heap-allocated data throws an error,
#    to prevent accidental garbage.
Base.setindex!(::UpTo, _, ::Int) = error("Cannot modify an UpTo instance, it's read-only")
Base.similar(::UpTo, args...) = error("You shouldn't be creating a mutable copy of UpTo")
# collect() is allowed, as it's explicit.
Base.collect(a::UpTo) = collect(a.buffer[1:a.count])