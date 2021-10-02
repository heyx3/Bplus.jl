using TupleTools, Setfield

"""
A vector math struct.
You can get its data from the field "data",
  or access individual components "x|r", "y|g", "z|b", "w|a",
  or even swizzle it, with 0 and 1 constants (e.x. "v.bgr1").
You can also access its components like an AbstractVector.
To change how many digits are printed in the REPL (i.e. Base.show()),
  set VEC_N_DIGITS or call `print_vec_digits()`.
NOTE: Broadcasting is not specialized; the output is an array instead of another Vector,
  because I don't know how to overload broadcasting correctly.
"""
struct Vec{N, T} <: AbstractVector{T}
    data::NTuple{N, T}

    # Construct with individual components.
    Vec(data::NTuple{N, T}) where {N, T} = new{N, T}(data)
    Vec(data::T...) where {T} = new{length(data), T}(data)
    Vec(data::Any...) = Vec(promote(data...))

    # Construct with a type parameter, and set of values.
    Vec{T}(data::NTuple{N, T2}) where {N, T, T2} = new{N, T}(map(T, data))
    Vec{T}(data::T2...) where {T, T2} = Vec(map(T, data))

    # "Empty" constructor makes a value with all 0's.
    Vec{N, T}() where {N, T} = new{N, T}(ntuple(i->zero(T), N))

    # Construct by appending smaller vectors/components together.
    Vec(first::Vec{N, T}, rest::T2...) where {N, T, T2} = Vec(promote(first..., rest...))
    Vec(first::Vec{N, T}, rest::Vec{N2, T2}) where {N, N2, T, T2} = Vec(promote(first..., rest...))
end
export Vec


# Base.show() prints the vector with a certain number of digits.
# Base.print() prints with a few extra digits.
VEC_N_DIGITS = 3
printable_component(f::T, n_digits::Int) where {T} = round(f; digits=n_digits)
printable_component(f::I, n_digits::Int) where {I<:Integer} = f
function Base.show(io::IO, ::MIME"text/plain", v::Vec{N, T}) where {N, T}
    global VEC_N_DIGITS
    n_digits::Int = VEC_N_DIGITS

    print(io, "{ ")
    if N > 0
        print(io, printable_component(v.x, n_digits))
    end
    for i in 2:N
        print(io, ", ", printable_component(v[i], n_digits))
    end
    print(io, " }")
end
function Base.print(io::IO, v::Vec{N, T}) where {N, T}
    global VEC_N_DIGITS
    n_digits::Int = VEC_N_DIGITS + 2
    
    print(io, "Vec", N, "(")
    if N > 0
        print(io, printable_component(v.x, n_digits))
    end
    for i in 2:N
        print(io, ", ", printable_component(v[i], n_digits))
    end
    print(io, ")")
end

"Runs the given code with VEC_N_DIGITS temporarily changed to the given value."
function print_vec_digits(to_do::Function, n::Int)
    global VEC_N_DIGITS
    old::Int = VEC_N_DIGITS
    VEC_N_DIGITS = n
    try
        to_do()
    catch e
        rethrow(e)
    finally
        VEC_N_DIGITS = old
    end
end
"Pretty-prints a vector using the given number of digits."
function show_vec(io::IO, v::Vec, n::Int)
    print_vec_digits(n) do
        show(io, v)
    end
end


Base.zero(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}()
Base.one(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}(ntuple(i->one(T), N))

Base.getindex(v::Vec, i::Int) = v.data[i]
Base.eltype(::Vec{N, T}) where {N, T} = T
Base.length(::Vec{N, T}) where {N, T} = N
Base.size(::Vec{N, T}) where {N, T} = (N, )
Base.IndexStyle(::Vec{N, T}) where {N, T} = IndexLinear()
Base.iterate(v::Vec, state...) = iterate(v.data, state...)
Base.map(f::Function, v::Vec) = Vec(map(f, v.data))

#TODO: Implement broadcasting. I tried already, but turns out it's really complicated...

@inline Base.getproperty(v::Vec, n::Symbol) =
    if (n == :x) | (n == :r)
        getfield(v, :data)[1]
    elseif (n == :y) | (n == :g)
        getfield(v, :data)[2]
    elseif (n == :z) | (n == :b)
        getfield(v, :data)[3]
    elseif (n == :w) | (n == :a)
        getfield(v, :data)[4]
    elseif n == :data
        return getfield(v, :data)
    else # Assume it's a swizzle.
        return swizzle(v, n)
    end
    
swizzle(v::Vec, n::Symbol) = swizzle(v, Val(n))


# In order to use things like Setfield, we need to overload Base.similar().
# The default behavior puts the components into an Array, instead of another Vec,
#    which creates heap allocations.
# So here we define a mutable version of Vec for this purpose.
mutable struct MVec{N, T} <: AbstractVector{T}
    data::NTuple{N, T}
end
# Conversions/constructors between Vec and MVec:
MVec(v::Vec{N, T}) where {N, T} = MVec(v.data)
Vec(v::MVec{N, T}) where {N, T} = Vec(v.data)
Base.convert(::Type{Vec{N, T}}, v::MVec{N, T}) where {N, T} = Vec(v.data)
Base.convert(::Type{MVec{N, T}}, v::Vec{N, T}) where {N, T} = MVec(v.data)
# AbstractArray iterface for MVec:
Base.getindex(v::MVec, i::Int) = v.data[i]
function Base.setindex!(v::MVec{N, T}, val::T, i::Int) where {N, T}
    d::NTuple{N, T} = v.data
    @set! d[i] = val
    v.data = d
end
Base.similar(v::MVec) = MVec(v.data)
Base.similar(v::MVec{N, T}, ::Type{T2}) where {N, T, T2} = MVec(map(T2, v.data))
function Base.similar(v::MVec{N, T}, ::Type{T2}, dims::Tuple{I}) where {N, T, T2, I<:Integer}
    N2 = dims[1]
    v2::MVec{N2, T2} = MVec(ntuple(i->zero(T2), N2))

    #TODO: Use broadcasting instead
    for i in 1:N
        v2[i] = convert(T2, v[i])
    end

    return v2
end
Base.eltype(::MVec{N, T}) where {N, T} = T
Base.length(::MVec{N, T}) where {N, T} = N
Base.size(::MVec{N, T}) where {N, T} = (N, )
Base.IndexStyle(::MVec{N, T}) where {N, T} = IndexLinear()
Base.iterate(v::MVec, state...) = iterate(v.data, state...)

Base.similar(v::Vec) = MVec(v.data)
Base.similar(v::Vec{N, T}, ::Type{T2}) where {N, T, T2} = MVec(map(T2, v.data))
function Base.similar(v::Vec{N, T}, ::Type{T2}, dims::Tuple{I}) where {N, T, T2, I<:Integer}
    N2 = dims[1]
    v2::Vec{N2, T2} = Vec(ntuple(i->zero(T2), N2))

    #TODO: Use broadcasting instead
    for i in 1:N
        @set! v2[i] = convert(T2, v[i])
    end

    return v2
end

# Common cases of swizzle(), with much better performance than the general case.
swizzle(v::Vec, ::Val{:xx}) = Vec(v.x, v.x)
swizzle(v::Vec, ::Val{:xxx}) = Vec(v.x, v.x, v.x)
swizzle(v::Vec, ::Val{:xxxx}) = Vec(v.x, v.x, v.x, v.x)
swizzle(v::Vec, ::Val{:yy}) = Vec(v.y, v.y)
swizzle(v::Vec, ::Val{:yyy}) = Vec(v.y, v.y, v.y)
swizzle(v::Vec, ::Val{:yyyy}) = Vec(v.y, v.y, v.y, v.y)
swizzle(v::Vec, ::Val{:zz}) = Vec(v.z, v.z)
swizzle(v::Vec, ::Val{:zzz}) = Vec(v.z, v.z, v.z)
swizzle(v::Vec, ::Val{:zzzz}) = Vec(v.z, v.z, v.z, v.z)
swibble(v::Vec, ::Val{:rr}) = Vec(v.r, v.r)
swibble(v::Vec, ::Val{:rrr}) = Vec(v.r, v.r, v.r)
swibble(v::Vec, ::Val{:rrrr}) = Vec(v.r, v.r, v.r, v.r)
swibble(v::Vec, ::Val{:gg}) = Vec(v.g, v.g)
swibble(v::Vec, ::Val{:ggg}) = Vec(v.g, v.g, v.g)
swibble(v::Vec, ::Val{:gggg}) = Vec(v.g, v.g, v.g, v.g)
swibble(v::Vec, ::Val{:bb}) = Vec(v.b, v.b)
swibble(v::Vec, ::Val{:bbb}) = Vec(v.b, v.b, v.b)
swibble(v::Vec, ::Val{:bbbb}) = Vec(v.b, v.b, v.b, v.b)
swizzle(v::Vec, ::Val{:xy}) = Vec(v.x, v.y)
swizzle(v::Vec, ::Val{:xyz}) = Vec(v.x, v.y, v.z)
swizzle(v::Vec, ::Val{:bgr}) = Vec(v.b, v.g, v.r)
swizzle(v::Vec, ::Val{:rgb}) = Vec(v.r, v.g, v.b)
swizzle(v::Vec, ::Val{:rg}) = Vec(v.r, v.g)
swizzle(v::Vec{N, T}, ::Val{:xyz0}) where {N, T} = Vec(v.x, v.y, v.z, zero(T))
swizzle(v::Vec{N, T}, ::Val{:rgb0}) where {N, T} = Vec(v.r, v.g, v.b, zero(T))
swizzle(v::Vec{N, T}, ::Val{:xyz1}) where {N, T} = Vec(v.x, v.y, v.z, one(T))
swizzle(v::Vec{N, T}, ::Val{:rgb1}) where {N, T} = Vec(v.r, v.g, v.b, one(T))
swizzle(v::Vec{N, T}, ::Val{Symbol("000w")}) where {N, T} = Vec(Vec(zero(T)).xxx, v.w)
swizzle(v::Vec{N, T}, ::Val{Symbol("000a")}) where {N, T} = Vec(Vec(zero(T)).xxx, v.a)
swizzle(v::Vec{N, T}, ::Val{Symbol("111w")}) where {N, T} = Vec(Vec(one(T)).xxx, v.w)
swizzle(v::Vec{N, T}, ::Val{Symbol("111a")}) where {N, T} = Vec(Vec(one(T)).xxx, v.a)

# The general use-case, which is forced to stringify the symbol to iterate its characters:
swizzle(v::Vec, ::Val{T}) where {T} = swizzle(v, string(T))
function swizzle(v::Vec{N, T}, c_str::S) where {N, T} where {S<:AbstractString}
    # Unfortunately, no way to do this with symbols.
    # Symbols don't provide a way to access their characters.
    new_len::Int = length(c_str)
    v2::Vec{new_len, T} = Vec{new_len, T}()

    for (i::Int, component::Char) in enumerate(c_str)
        if (component == 'x') | (component == 'r')
            @set! v2[i] = v[1]
        elseif (component == 'y') | (component == 'g')
            @set! v2[i] = v[2]
        elseif (component == 'z') | (component == 'b')
            @set! v2[i] = v[3]
        elseif (component == 'w') | (component == 'a')
            @set! v2[i] = v[4]
        elseif (component == '1')
            @set! v2[i] = 1
        elseif (component == '0')
            @set! v2[i] = 0
        else
            error("Invalid swizzle char: '", component, "'")
        end
    end

    return v2
end

"Computes the dot product of two vectors"
vdot(v1::Vec, v2::Vec) = sum(Iterators.map(t->t[1]*t[2], zip(v1, v2)))
"The \\cdot character represents the dot product."
⋅ = vdot
export vdot, ⋅