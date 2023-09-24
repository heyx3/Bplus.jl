using TupleTools, Setfield

"""
A vector math struct.
You can get its data from:
* The field "data", e.x. `v.data::NTuple`
* Individual XYZW or RGBA components, e.x. `v1.x == v1.r`
* Swizzling, e.x. `v.yyyz == Vec(v.y, v.y, v.y, v.z)`.

Swizzles can also use `0` for a constant 0, `1` for a constant 1,
  `∆` (\\increment) for the max finite value, and `∇` (\\nabla) for the min finite value.
For example, `Vec{UInt8}(128, 4, 200).rgb∆` results in `Vec{UInt8}(128, 4, 200, 255)`.

`Vec` has an efficient implementation of `AbstractVector`, including `map`, `foldl`, etc,
  except for broadcasting because I don't know how to do it yet.

You can use the colon operator to iterate between two values (e.x. `Vec(1, 1) : Vec(10, 10)`).

To change how many digits are printed in the REPL and `Base.show()`,
  set `VEC_N_DIGITS` or call `use_vec_digits() do ... end`.

NOTE: Comparing two vectors with `==` or `!=` returns a boolean as expected,
  but other comparisons (`<`, `<=`, `>`, `>=`) return a component-wise result.
"""
struct Vec{N, T} <: AbstractVector{T}
    data::NTuple{N, T}

    # Construct with components, and no type parameters.
    Vec(data::NTuple{N, T}) where {N, T} = new{N, T}(data)
    @inline Vec(data::Any...) = let promoted = promote(data...)
        new{length(data), eltype(promoted)}(promoted)
    end

    # Construct a "0-dimensional" vector.
    Vec{T}() where {T} = new{0, T}(tuple())

    # Construct a constant vector with all components set to one value.
    @inline Vec(n::Int, ::Val{X}) where {X} = new{n, typeof(X)}(tuple(Iterators.repeated(X, n)...))
    @inline Vec{N}(::Val{X}) where {N, X} = new{N, typeof(X)}(tuple(Iterators.repeated(X, N)...))
    @inline Vec{N, T}(::Val{X}) where {N, T, X} = new{N, T}(tuple(Iterators.repeated(X, N)...))

    # Construct with the type parameter (no length), and components.
    function Vec{T}(data::TTuple) where {T, TTuple<:Tuple}
        @bp_check(!(T isa Int), "Constructors of the form 'VecN(x, y, ...)' aren't allowed. Try 'Vec(x, y, ...)', or 'VecN{T}(x, y, ...)'")
        return new{length(data), T}(tuple((convert(T, d) for d in data)...))
    end
    @inline function Vec{T}(data::T2...) where {T, T2}
        @bp_check(!(T isa Int), "Constructors of the form 'VecN(x, y, ...)' aren't allowed. Try 'Vec(x, y, ...)', or 'VecN{T}(x, y, ...)'")
        return new{length(data), T}(tuple((convert(T, d) for d in data)...))
    end

    # Construct with the type and length parameters.
    Vec{N, T}(data::TTuple) where {N, T, TTuple<:Tuple} = begin
        @bp_check(length(data) == N,
                  "Expected ", N, " elements, got ", length(data))
        return new{N, T}(tuple((
            convert(T, d) for d in data
        )...))
    end
    @inline Vec{N, T}(data::Any...) where {N, T} = new{N, T}(tuple((
        convert(T, d) for d in data
    )...))
    @inline Vec{N, T}(data::T...) where {N, T} = new{N, T}(data)

    # "Empty" constructor makes a value with all 0's.
    Vec{N, T}() where {N, T} = new{N, T}(tuple(Iterators.repeated(zero(T), N)...))

    # Construct with a lambda, like ntuple().
    Vec(make_component::TCallable, n::Int) where {TCallable<:Base.Callable} = Vec(ntuple(make_component, n)...)
    Vec(make_component::TCallable, v::Val{N}) where {N, TCallable<:Base.Callable} = Vec(ntuple(make_component, v)...)
    Vec{N, T}(make_component::TCallable) where {N, T, TCallable<:Base.Callable} = new{N, T}(
        ntuple(i -> convert(T, make_component(i)), Val(N))
    )
end
export Vec

"Constructs a vector by appending smaller vectors and components together"
@inline vappend(x) = Vec{1, typeof(x)}(x)
@inline vappend(x::Vec) = x
@inline vappend(first, rest...) = Vec(vappend(first)..., vappend(rest...)...)
# NOTE: This was originally implemented as a Vec constructor,
#    but that caused havoc on type-inference.
# I don't recomment trying that again.
export vappend

StructTypes.construct(T::Type{<:Vec}, components::Vector) = T(components...)


###############
#   Aliases   #
###############

# Note that in gamedev, 32-bit data are far more common than 64-bit, for performance reasons.
# Especially when talking about float data on the GPU --
#    commercial gaming GPU's can have pretty terrible float64 performance.

const Vec2{T} = Vec{2, T}
const Vec3{T} = Vec{3, T}
const Vec4{T} = Vec{4, T}

const VecF{N} = Vec{N, Float32}
const v2f = VecF{2}
const v3f = VecF{3}
const v4f = VecF{4}
const VecD{N} = Vec{N, Float64}
const v2d = VecD{2}
const v3d = VecD{3}
const v4d = VecD{4}

const VecU{N} = Vec{N, UInt32}
const v2u = VecU{2}
const v3u = VecU{3}
const v4u = VecU{4}
const VecI{N} = Vec{N, Int32}
const v2i = VecI{2}
const v3i = VecI{3}
const v4i = VecI{4}

const VecB{N} = Vec{N, Bool}
const v2b = VecB{2}
const v3b = VecB{3}
const v4b = VecB{4}

"Allows you to specify types like `VecT{Int}`"
const VecT{T, N} = Vec{N, T}

const vRGB{T} = Vec{3, T}
const vRGBu8 = vRGB{UInt8}
const vRGBi8 = vRGB{Int8}
const vRGBu = vRGB{UInt32}
const vRGBi = vRGB{Int32}
const vRGBf = vRGB{Float32}

const vRGBA{T} = Vec{4, T}
const vRGBAu8 = vRGBA{UInt8}
const vRGBAi8 = vRGBA{Int8}
const vRGBAf = vRGBA{Float32}
const vRGBAu = vRGBA{UInt32}
const vRGBAi = vRGBA{Int32}


export Vec2, Vec3, Vec4,
       vRGB, vRGBA,
       vRGBu8, vRGBi8, vRGBf, vRGBu, vRGBi,
       vRGBAu8, vRGBAi8, vRGBAf, vRGBAu, vRGBAi,
       VecF, VecI, VecU, VecB, VecD,
       v2f, v3f, v4f,
       v2i, v3i, v4i,
       v2u, v3u, v4u,
       v2b, v3b, v4b,
       v2d, v3d, v4d,
       VecT
#

################
#   Printing   #
################

# Base.show() prints the vector with a certain number of digits.

VEC_N_DIGITS::Int = 2

printable_component(x, args...) = x
printable_component(f::AbstractFloat, n_digits::Int) = round(f; digits=n_digits)
printable_component(f::Integer, n_digits::Int) = f

Base.show(io::IO, v::Vec) = show(io, MIME"text/plain"(), v)
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
    n_digits::Int = VEC_N_DIGITS
    
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
function use_vec_digits(to_do::Base.Callable, n::Int)
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
export use_vec_digits

"Pretty-prints a vector using the given number of digits."
function show_vec(io::IO, v::Vec, n::Int)
    use_vec_digits(n) do
        show(io, v)
    end
end
export show_vec

###################################################
#   Arithmetic, Iteration, and other interfaces   #
###################################################

@inline Base.zero(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}()
@inline Base.zero(x::Vec) = zero(typeof(x))
@inline Base.one(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}(Val(one(T)))
@inline Base.one(x::Vec) = one(typeof(x))

Base.typemin(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}(Val(typemin(T)))
Base.typemax(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}(Val(typemax(T)))

@inline Base.min(v::Vec) = reduce(min, v)
Base.min(p1::Vec{N, T1}, p2::Vec{N, T2}) where {N, T1, T2} = Vec((min(p1[i], p2[i]) for i in 1:N)...)
Base.min(p1::Vec{N, T1}, p2::T2) where {N, T1, T2} = Vec((min(p1[i], p2) for i in 1:N)...)
@inline Base.min(p1::Number, p2::Vec) = min(p2, p1)

@inline Base.max(v::Vec) = reduce(max, v)
Base.max(p1::Vec{N, T1}, p2::Vec{N, T2}) where {N, T1, T2} = Vec((max(p1[i], p2[i]) for i in 1:N)...)
Base.max(p1::Vec{N, T1}, p2::T2) where {N, T1, T2} = Vec((max(p1[i], p2) for i in 1:N)...)
@inline Base.max(p1::Number, p2::Vec) = max(p2, p1)

@inline function Base.minmax(p1::Vec{N, T1}, p2::Vec{N, T2}) where {N, T1, T2}
    T3 = promote_type(T1, T2)
    minmax_tuple = map(minmax, p1, p2)
    return (
        Vec{N, T3}(i -> minmax_tuple[i][1]),
        Vec{N, T3}(i -> minmax_tuple[i][2])
    )
end
Base.minmax(p1::Vec{N, T1}, p2::T2) where {N, T1, T2} = minmax(p1, Vec{N, T2}(i->p2))
Base.minmax(p1::Number, p2::Vec) = minmax(p2, p1)

@inline function Base.divrem(x::Vec{N, T1}, y::Vec{N, T2}) where {N, T1, T2}
    T3 = promote_type(T1, T2)
    divrem_tuple = map(divrem, x, y)
    return (
        Vec{N, T3}(i->divrem_tuple[i][1]),
        Vec{N, T3}(i->divrem_tuple[i][2])
    )
end
Base.divrem(x::Vec{N, T1}, y::T2) where {N, T1, T2} = divrem(x, Vec{N, T2}(i->y))
Base.divrem(x::T1, y::Vec{N, T2}) where {N, T1, T2} = divrem(Vec{N, T1}(i->x), y)

@inline Base.abs(v::Vec) = Vec((abs(f) for f in v)...)

@inline Base.round(v::Vec, a...; kw...) = map(f -> round(f, a...; kw...), v)

"Finds the minimum component which passes a given predicate"
function Base.findmin(pred::F, v::Vec{N, T})::Optional{T} where {F, N, T}
    min_i::Int = 0
    for i::Int in 1:N
        if pred(v[i])
            min_i = min(min_i, i)
        end
    end
    return (min_i > 0) ? min_i : nothing
end

function Base.reinterpret(::Type{Vec{N, T2}}, v::Vec{N, T}) where {N, T, T2}
    @bp_check(sizeof(T) == sizeof(T2),
              "Can't reinterpret ", T, " (", sizeof(T), " bytes),",
                " as ", T2, " (", sizeof(T2), " bytes)")
    return map(x -> reinterpret(T2, x), v)
end

"Gets the size of an array as a `Vec`, rather than an `NTuple`."
vsize(arr::AbstractArray) = Vec(size(arr)...)
export vsize

Base.clamp(v::Vec{N, T}, a::T2, b::T3) where {N, T, T2, T3} = map(x -> convert(T, clamp(x, a, b)), v)
Base.clamp(v::Vec{N, T}, a::Vec{N, T2}, b::Vec{N, T3}) where {N, T, T2, T3} = Vec{N, T}((
    clamp(x, aa, bb) for (x, aa, bb) in zip(v, a, b)
)...)

Random.rand(rng::Random.AbstractRNG, ::Type{Vec{N, F}}) where {N, F} = Vec{N, F}(i -> Random.rand(rng, F))

Base.convert(::Type{Vec{N, T2}}, v::Vec{N, T}) where {N, T, T2} = map(x -> convert(T2, x), v)
Base.convert(::Type{Vec{N, T}}, v::Vec{N, T}) where {N, T} = v
Base.promote_rule(::Type{Vec{N, T1}}, ::Type{Vec{N, T2}}) where {N, T1, T2} = Vec{N, promote_type(T1, T2)}

Base.convert(::Type{Vec{N, T2}}, a::SVector{N, T}) where {N, T, T2} = Vec(T2.(a)...)

Base.reverse(v::Vec) = Vec(reverse(v.data))

Base.getindex(a::Array, i::VecT{<:Integer}) = a[i...]
Base.setindex!(a::Array, t, i::VecT{<:Integer}) = (a[i...] = t)

# StaticArrays tries to interpret Vec as an array of indices rather than a multidimensional index.
Base.getindex(a::StaticArray{N}, i::Vec{N, <:Integer}) where {N} = a[i...]
Base.setindex!(a::StaticArray{N}, t, i::Vec{N, <:Integer}) where {N} = (a[i...] = t)

@inline Base.getindex(v::Vec, i::Integer) = v.data[i]
@inline Base.getindex(v::Vec, r::UnitRange) = Vec(v.data[r])
Base.eltype(::Vec{N, T}) where {N, T} = T
Base.length(::Vec{N, T}) where {N, T} = N
Base.size(::Vec{N, T}) where {N, T} = (N, )
Base.IndexStyle(::Vec{N, T}) where {N, T} = IndexLinear()
@inline Base.iterate(v::Vec, state...) = iterate(v.data, state...)

Base.length(::Type{Vec{N, T}}) where {N, T} = N
Base.eltype(::Type{Vec{N, T}}) where {N, T} = T

# Allow for swizzling by passing multiple indices for getindex().
@inline Base.getindex(v::Vec, idcs::Integer...) = Vec((v[i] for i in idcs)...)

# The simpler syntax does not appear to work for type inference :(
@generated function Base.map(f, v::Vec{N}...)::Vec{N} where {N}
    components = [ ]
    exprs = quote end
    for i in 1:N
        push!(components, Symbol("a$i"))
        push!(exprs.args, :(
            $(components[i]) = f(getindex.(v, Ref($i))...)
        ))
    end
    push!(exprs.args, :( return Vec($(components...)) ))
    return exprs
end
# "
# Like map, but automatically converts all non-Vec inputs into Vec's.
# Ref values will be left alone, similar to how broadcasting works.
# "
# @inline function vmap(f, values...)::Vec
#     param_sizes = tuple((
#         if v isa Ref
#             -1
#         elseif v isa Vec
#             length(v)
#         else
#             -1
#         end for v in values
#     )...)
#     N = max(param_sizes)
#     inputs = tuple((
#         if v isa Ref
#             v[]
#         elseif v isa Vec
#             v
#         else
#             Vec(i -> v, Val(length(v)))
#         end for v in values
#     )...)
#     return Vec(i -> f())
# end
#TODO: Revisit vmap() sometime


function Base.foldl(func::F, v::Vec{N, T}) where {F, N, T}
    f::T = v[1]
    for i in 2:N
        f = func(f, v[i])
    end
    return f
end
function Base.foldl(func::F, v::Vec{N, T}, init::T2)::T2 where {F, N, T, T2}
    output::T2 = init
    for f::T in v
        output = func(output, f)
    end
    return output
end
function Base.foldr(func::F, v::Vec{N, T}) where {F, N, T}
    f::T = v[end]
    for i in (N-1):-1:1
        f = func(f, v[i])
    end
    return f
end
function Base.foldr(func::F, v::Vec{N, T}, init::T2)::T2 where {F, N, T, T2}
    f::T2 = init
    for i in N:-1:1
        f = func(f, v[i])
    end
    return f
end

"
Like a binary version of lerp, or like `step()`.
Returns components of `a` when `t` is false, or `b` when `t` is true.
"
vselect(a::F, b::F, t::Bool) where {F} = (t ? b : a)
vselect(a::Vec{N, T}, b::Vec{N, T}, t::Vec{N}) where {N, T} = Vec{N, T}((
    vselect(xA, xB, xT) for (xA, xB, xT) in zip(a, b, t)
)...)
export vselect

# I can't figure out how to make the general-case form of `isapprox()` work
#   without heap allocations (I think it's related to the keyword arguments).
# I tried all sorts of stuff, including @generate.
# So here are two simple, common cases.
# Note that I've done the same thing for Quaternions.
Base.isapprox(a::Vec{N, T1}, b::Vec{N, T2}) where {N, T1, T2} =
    all(t -> isapprox(t[1], t[2]), zip(a, b))
Base.isapprox(a::Vec{N, T1}, b::Vec{N, T2}, atol) where {N, T1, T2} =
    all(t -> isapprox(t[1], t[2]; atol=atol), zip(a, b))


@inline Base.getproperty(v::Vec, n::Symbol) = getproperty(v, Val(n))
# getproperty() for individual components:
@inline Base.getproperty(v::Vec, ::Val{:x}) = getfield(v, :data)[1]
@inline Base.getproperty(v::Vec, ::Val{:y}) = getfield(v, :data)[2]
@inline Base.getproperty(v::Vec, ::Val{:z}) = getfield(v, :data)[3]
@inline Base.getproperty(v::Vec, ::Val{:w}) = getfield(v, :data)[4]
@inline Base.getproperty(v::Vec, ::Val{:r}) = getfield(v, :data)[1]
@inline Base.getproperty(v::Vec, ::Val{:g}) = getfield(v, :data)[2]
@inline Base.getproperty(v::Vec, ::Val{:b}) = getfield(v, :data)[3]
@inline Base.getproperty(v::Vec, ::Val{:a}) = getfield(v, :data)[4]
# getproperty() for fields:
@inline Base.getproperty(v::Vec, ::Val{:data}) = getfield(v, :data)
# getproperty() for swizzling:
@inline Base.getproperty(v::Vec, ::Val{T}) where {T} = swizzle(v, Val(T))

@inline Base.propertynames(::Vec, _) = (:x, :y, :z, :w, :data)

Base.:(+)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i+j for (i,j) in zip(a, b))...)
Base.:(-)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i-j for (i,j) in zip(a, b))...)
Base.:(*)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i*j for (i,j) in zip(a, b))...)
Base.:(/)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i/j for (i,j) in zip(a, b))...)
Base.:(÷)(a::Vec{N, I1}, b::Vec{N, I2}) where {N, I1, I2} = Vec((i÷j for (i,j) in zip(a, b))...)
Base.:(^)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i^j for (i,j) in zip(a, b))...)
Base.:(%)(a::Vec{N, I1}, b::Vec{N, I2}) where {N, I1, I2} = Vec((i%j for (i,j) in zip(a, b))...)
Base.mod(a::Vec{N, I1}, b::Vec{N, I2}) where {N, I1, I2} = Vec((mod(i, j) for (i,j) in zip(a, b))...)

Base.:(+)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f+b), a)
Base.:(-)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f-b), a)
Base.:(*)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f*b), a)
Base.:(/)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f/b), a)
Base.:(÷)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f÷b), a)
Base.:(^)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f^b), a)
Base.:(%)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f%b), a)
Base.mod(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f -> mod(f, b), a)

@inline Base.:(+)(a::Number, b::Vec) = b+a
@inline Base.:(-)(a::Number, b::Vec) = (-b)+a
@inline Base.:(*)(a::Number, b::Vec) = b*a
@inline Base.:(/)(a::Number, b::Vec) = map(f->(a/f), b)
@inline Base.:(÷)(a::Number, b::Vec) = map(f->(a÷f), b)
@inline Base.:(%)(a::Number, b::Vec) = map(f->(a%f), b)

@inline Base.:(-)(a::Vec)::Vec = map(-, a)

@inline Base.:(==)(a::Number, b::Vec) = b==a
@inline Base.:(==)(a::Vec, b::Number) = all(x->x==b, a)

Base.:(<)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i<j for (i,j) in zip(a, b))...)
Base.:(<)(a::Vec{N, T}, b::T2) where {N, T, T2} = map(x -> x<b, a)
Base.:(<)(a::T2, b::Vec{N, T}) where {N, T, T2} = map(x -> a<x, b)
Base.:(<=)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i<=j for (i,j) in zip(a, b))...)
Base.:(<=)(a::Vec{N, T}, b::T2) where {N, T, T2} = map(x -> x<=b, a)
Base.:(<=)(a::T2, b::Vec{N, T}) where {N, T, T2} = map(x -> a<=x, b)

for bitwise_expr in [ :&, :|, :⊻, :⊼, :!, :<<, :>> ]
    @eval begin
        Base.$bitwise_expr(a::Vec{N}, b::Vec{N}) where {N} =
            Vec(($bitwise_expr(i, j) for (i, j) in zip(a, b))...)
        Base.$bitwise_expr(a::Vec, b) = map(x -> $bitwise_expr(x, b), a)
        Base.$bitwise_expr(a, b::Vec) = map(x -> $bitwise_expr(a, x), b)
    end
end

# Help convert a Ref(Vec{T}) to a Ptr{T}, for C calls.
Base.unsafe_convert(::Type{Ptr{T}}, r::Base.RefValue{<:VecT{T}}) where {T} =
    Base.unsafe_convert(Ptr{T}, Base.unsafe_convert(Ptr{Nothing}, r))
Base.unsafe_convert(::Type{Ptr{NTuple{N, T}}}, r::Base.RefValue{Vec{N, T}}) where {N, T} =
    Base.unsafe_convert(Ptr{NTuple{N, T}}, Base.unsafe_convert(Ptr{Nothing}, r))
#

#######################
#    Colon Operator   #
#######################

"Implements iteration over a range of coordinates (you can also use the `:` operator)"
struct VecRange{N, T} <: AbstractRange{Vec{N, T}}
    a::Vec{N, T}
    b::Vec{N, T}
    step::Vec{N, T}
end

@inline function Base.:(:)(a::Vec{N, T1}, b::Vec{N, T2}) where {N, T1, T2}
    T = promote_type(T1, T2)
    V = Vec{N, T}
    return VecRange(convert(V, a), convert(V, b), one(V))
end
@inline function Base.:(:)(a::Vec, step::Vec, b::Vec)
    @bp_check(none(iszero, step), "Iterating with a step size of 0: ", step)
    VecRange(a, b, step)
end

# Support mixing single values with vectors.
Base.:(:)(a::Vec, b::Number) = a : typeof(a)(i->b)
Base.:(:)(a::Number, b::Vec) = typeof(b)(i->a) : b
Base.:(:)(a::Vec, step::Number, b::Vec) = a : typeof(a)(i->step) : b
Base.:(:)(a::Number, step::Vec, b::Vec) = typeof(b)(i->a) : step : b
Base.:(:)(a::Number, step::Number, b::Vec) = typeof(b)(i->a) : typeof(b)(i->step) : b
Base.:(:)(a::Number, step::Vec, b::Number) = typeof(step)(i->a) : step : typeof(step)(i->b)
Base.:(:)(a::T, step::Vec, b::T) where {T<:Real} = typeof(step)(i->a) : step : typeof(step)(i->b)

@inline Base.in(v::Vec{N, T2}, r::VecRange{N, T}) where {N, T, T2} = all(tuple(
    (v[i] in r.a[i]:r.step[i]:r.b[i]) for i in 1:N
)...)
@inline Base.in(v::Vec{N, T}, range::AbstractRange) where {N, T} = all(tuple(
    (v_ in range) for v_ in v
)...)
Base.eltype(::VecRange{N, T}) where {N, T} = T

function Base.getindex(v::VecRange{N, T}, i::Integer) where {N, T}
    # Copied and modified from the implementation for `AbstractRange`.
    @inline
    i isa Bool && throw(ArgumentError("invalid index: $i of type Bool"))
    # Here's the modified part:
    axis_count = (last(v) - first(v) + 1) ÷ step(v)
    i_per_axis = Vec(Base._ind2sub(axis_count.data, i)...)
    @boundscheck (all(i_per_axis > 0) & (i_per_axis <= axis_count)) || Base.throw_boundserror(v, i)
    return first(v) + ((i_per_axis - 1) * step(v))
end

function Random.rand(rng::Random.AbstractRNG, range::VecRange{N, T}) where {N, T}
    rand_along_axis(i) = rand(rng, range.a[i]:range.step[i]:range.b[i])
    Vec{N, T}(rand_along_axis)
end

Base.first(r::TVecRange) where {TVecRange<:VecRange} = r.a
Base.last(r::TVecRange) where {TVecRange<:VecRange} = r.b
Base.step(r::TVecRange) where {TVecRange<:VecRange} = r.step

Base.isempty(r::TVecRange) where {TVecRange<:VecRange} = any(
    (b < a) for (a,b) in zip(r.a, r.b)
)

Base.size(r::TVecRange) where {TVecRange<:VecRange} = tuple(
    (length(a:step:b) for (a, b, step) in zip(r.a, r.b, r.step))...
)
Base.length(r::VecRange) = prod(size(r))

# The iteration algorithm is recursive, with a type parameter for the axis being incremented.
@inline function Base.iterate(r::VecRange)
    if any(r.a > r.b)
        return nothing
    else
        return (r.a, r.a)
    end
end
@inline function Base.iterate( r::VecRange{N, T},
                               last_pos::Vec{N, T}
                             ) where {N, T}
    p::Vec{N, T} = vec_iterate(r, last_pos, Val(1))
    return (p[end] > r.b[end]) ? nothing : (p, p)
end
@inline function vec_iterate( r::VecRange{N, T},
                              last_pos::Vec{N, T},
                              ::Val{Axis}
                            )::Vec{N, T} where {N, T, Axis}
    # If we've passed the last dimension that could be incremented,
    #    then we've covered the whole space.
    if Axis > N
        return @set last_pos[end] = (r.b[end] + r.step[end])
    end

    # If there's more to iterate along this axis, do so.
    axis_step::T = r.step[Axis]
    next_axis_val::T = last_pos[Axis] + axis_step
    if ((axis_step > 0) & (next_axis_val <= r.b[Axis])) |
       ((axis_step < 0) & (next_axis_val >= r.b[Axis]))
    #begin
        return @set last_pos[Axis] = next_axis_val
    end

    # Otherwise, we passed the end of this axis, so reset it and move to the next axis.
    @set! last_pos[Axis] = r.a[Axis]
    return vec_iterate(r, last_pos, Val(Axis+1))
end


################
#   Setfield   #
################

Setfield.set(v::Vec,  ::Setfield.PropertyLens{field}, val) where {field} = Setfield.set(v, Val(field), val)
Setfield.set(v::Vec, i::Setfield.IndexLens, val) = typeof(v)(Setfield.set(v.data, i, val)...)
Setfield.set(v::Vec, f::Setfield.DynamicIndexLens, val) = typeof(v)(Setfield.set(v.data, f, val)...)
Setfield.set(v::Vec, f::Setfield.FunctionLens, val) = typeof(v)(Setfield.set(v.data, f, val)...)

Setfield.get(v::Vec, ::Setfield.PropertyLens{field}) where {field} = getproperty(v, Val(field))
Setfield.get(v::Vec, i::Setfield.IndexLens) = v[i.indices...]
Setfield.get(v::Vec, f::Setfield.DynamicIndexLens) = Setfield.get(v.data, f)
Setfield.get(v::Vec, f::Setfield.FunctionLens) = Setfield.get(v.data, f)

Setfield.set(v::Vec, ::Union{Val{:x}, Val{:r}}, val) = let d = v.data
    Vec(@set d[1] = val)
end
Setfield.set(v::Vec, ::Union{Val{:y}, Val{:g}}, val) = let d = v.data
    Vec(@set d[2] = val)
end
Setfield.set(v::Vec, ::Union{Val{:z}, Val{:b}}, val) = let d = v.data
    Vec(@set d[3] = val)
end
Setfield.set(v::Vec, ::Union{Val{:w}, Val{:a}}, val) = let d = v.data
    Vec(@set d[4] = val)
end

Setfield.set(v::Vec, ::Val{:data}, val) = return typeof(v)(val...)


#######################
#   Other functions   #
#######################

@inline swizzle(v::Vec, components::Symbol) = swizzle(v, Val(components))
@inline @generated swizzle(v::Vec{N, T}, components::Val{S}) where {N, T, S} = swizzle_ast(N, T, S, :v)

function swizzle_ast(N::Int, T::Type, S::Symbol, var_name::Symbol)
    chars = string(S)

    outputs = [ ]
    for char::Char in chars
        push!(outputs,
            if char in ('x', 'r')
                :( $var_name.x )
            elseif char in ('y', 'g')
                :( $var_name.y )
            elseif char in ('z', 'b')
                :( $var_name.z )
            elseif char in ('w', 'a')
                :( $var_name.w )
            elseif char == '1'
                one(T)
            elseif char == '0'
                zero(T)
            elseif char == 'Δ'
                typemax_finite(T)
            elseif char =='∇'
                typemin_finite(T)
            else
                error("Invalid swizzle char: '", char, "'")
            end
        )
    end

    N2 = length(outputs)
    return :( Vec{$N2, T}($(outputs...)) )
end

"Computes the dot product of two vectors"
@inline vdot(v1::Vec, v2::Vec) = +(((t[1] * t[2]) for t in zip(v1, v2))...)
export vdot

"Computes the square distance between two vectors"
function vdist_sqr(v1::Vec{N, T1}, v2::Vec{N, T2})::promote_type(T1, T2) where {N, T1, T2}
    delta::Vec{N, promote_type(T1, T2)} = v1 - v2
    return sum(delta * delta)
end
"Computes the distance between two vectors"
@inline vdist(v1::Vec, v2::Vec) = sqrt(vdist_sqr(v1, v2))
export vdist_sqr, vdist

"Computes the square length of a vector"
function vlength_sqr(v::Vec{N, T})::T where {N, T}
    return vdot(v, v)
end
"Computes the length of a vector"
@inline vlength(v::Vec) = sqrt(vlength_sqr(v))
export vlength_sqr, vlength

"Normalizes a vector"
@inline vnorm(v::Vec) = v / vlength(v)
export vnorm

"Reflects a vector along a 'normal' vector"
vreflect(v::Vec{N}, normal::Vec{N}) where {N} = let F = promote_type(eltype(v), eltype(normal))
    return v - (convert(F, 2) * vdot(v, normal) * normal)
end
export vreflect

function vrefract( v::Vec{N, F1},
                   normal::Vec{N, F2},
                   refractive_index_src_over_dest::F3
                 )::Vec{N} where {N, F1, F2, F3}
    # Promote all number types involved to a common type.
    F = promote_type(F1, promote_type(F2, F3))
    v_ = convert(Vec{N, F}, v)
    normal_ = convert(Vec{N, F}, normal)
    ior_ = convert(F, refractive_index_src_over_dest)

    V = Vec{N, F}
    ONE::F = one(F)

    cos_theta::F = min(-v_ ⋅ normal_, ONE)
    perpendicular_part::V = ior_ * (v_ + (cos_theta * normal_))
    parallel_part::V = -sqrt(abs(ONE - vlength_sqr(perpendicular_part))) * normal_

    return perpendicular_part * parallel_part
end
export vrefract

"Three vectors defining an ortho-normal basis: `forward`, `right`, and `up`"
struct VBasis{F}
    forward::Vec3{F}
    up::Vec3{F}
    right::Vec3{F}
end
Base.convert(::Type{VBasis{F2}}, b::VBasis{F1}) where {F1, F2} = VBasis(
    convert(Vec3{F2}, b.forward),
    convert(Vec3{F2}, b.up),
    convert(Vec3{F2}, b.right)
)
export VBasis

"
Constructs an orthogonal 3D vector basis as similar as possible to the input vectors.
If the vectors are equal, then the up axis will usually be calculated with the global up axis.
Returns the new forward and up vectors.
"
function vbasis( forward::Vec3{F1},
                 up::Vec3{F2} = get_up_vector(F1),
                 atol::Real = F1(0.001)
               )::VBasis where {F1, F2}
    F3 = promote_type(F1, F2)
    @inline are_parallel(a::Vec3, b::Vec3) = (1 - abs(vdot(a, b))) <= atol

    # Find the right vector.
    local right::Vec3{F3}
    if !are_parallel(forward, up)
        right = vnorm(v_rightward(forward, up))
        up = vnorm(v_rightward(right, forward))
        return VBasis(forward, up, right)
    else
        right = Vec3{F3}(1, 0, 0)
        if are_parallel(right, forward) || are_parallel(right, up)
            right = Vec3{F3}(0, 1, 0)
            if are_parallel(right, forward) || are_parallel(right, up)
                right = Vec3{F3}(0, 0, 1)
                @bp_math_assert(!are_parallel(right, forward) && !are_parallel(right, up))
            end
        end

        up = vnorm(v_rightward(right, forward))
        right = v_rightward(forward, up) # Normalization not needed; inputs are normalized
                                         #    and perpendicular.
        return VBasis(forward, up, right)
    end
end
export vbasis

"Checks whether a vector is normalized, within the given epsilon"
v_is_normalized(v::Vec{N, T}, atol::T2 = 0.0) where {N, T, T2} =
    isapprox(vlength_sqr(v), one(T); atol=atol*atol)
export v_is_normalized

"Computes the 3D cross product."
function vcross( a::Vec3{T1},
                 b::Vec3{T2}
               )::Vec3{promote_type(T1, T2)} where {T1, T2}
    return Vec3{promote_type(T1, T2)}(
        foldl(-, a.yz * b.zy),
        foldl(-, a.zx * b.xz),
        foldl(-, a.xy * b.yx)
    )
end
export vcross


##########################
#   Symbolic operators   #
##########################

# Define some mathematical aliases for some vector ops, in case anybody likes mathy syntax.

"The \\cdot character represents the dot product."
const ⋅ = vdot
"The \\times character represents the cross product."
const × = vcross
export ⋅, ×


###########################
#   Vertical Coordinates  #
###########################

# Different 3D environments use different up-axes, usually Y or Z.
# Some helpers are provided to separate the horizontal and vertical components of a vector,
#    and they can be configured dynamically to recompile for a different up axis
#    for your particular project.

# A similar method is applied to handed-ness.


##  Core functions  ##

"""
Gets the vertical axis -- 1=X, 2=Y, 3=Z.
By default, uses Z as the up-axis.

Redefine this (e.x. `get_up_axis() = 2`) to change the vertical axis for your project.
Julia's JIT will allow it to act as a compile-time constant
    after the initial overhead of recompilation.
"""
get_up_axis()::Int = 3

"""
Gets the sign of the "Upward" direction of the vertical axis.
For example, if `get_up_axis()==3 && get_up_sign()==-1`,
  then the "Up" direction is pointing in the -Z direction.

By default, points in the positive direction (i.e. returns -1).

Like `get_up_axis()`, you can redefine this to configure coordinate math for your project.
"""
get_up_sign()::Int = 1

"""
Gets whether coordinate math is right-handed.
By default, is true.

Redefine this (`get_right_handed() = false`) to make your project left-handed.
Julia's JIT will allow it to act as a compile-time constant
    after the initial overhead of recompilation.

For example, you might want to do this if your game is primarily rendered through Dear ImGUI.
"""
get_right_handed()::Bool = true


##  Helpers  ##

"Does a cross product in the correct way to get a rightward vector from forward and upward ones"
v_rightward(forward::Vec3, up::Vec3) = get_right_handed() ? (forward × up) : (up × forward)

"Gets a normalized vector pointing in the positive direction along the Up axis"
@inline get_up_vector(F = Float32) = Vec3{F}(
    @set (0, 0, 0)[get_up_axis()] = sign(get_up_sign())
)

"Gets the horizontal axes -- 1=X, 2=Y, 3=Z."
@inline function get_horz_axes()::Tuple{Int, Int}
    up_i::Int = get_up_axis()
    idcs = (
        mod1(up_i + 1, 3),
        mod1(up_i + 2, 3)
    )
    if (idcs[1] > idcs[2])
        idcs = (idcs[2], idcs[1])
    end
    return idcs
end
"Gets one of the two horizontal vectors."
@inline function get_horz_vector(i::Int, F = Float32)::Vec3{F}
    v = zero(Vec3{F})
    @set! v[get_horz_axes()[i]] = one(F)
    return v
end

"Extracts the horizontal components from a 3D vector."
@inline function get_horz(v::Vec3)
    (a::Int, b::Int) = get_horz_axes()
    Vec(v[a], v[b])
end

"Extracts the vertical component from a 3D vector."
@inline get_vert(v::Vec3) = v[get_up_axis()]

"Inserts a vertical component into the given horizontal vector."
@inline function to_3d(v_2d::Vec2{T}, vertical::T2 = zero(T))::Vec3{T} where {T, T2}
    v::Vec3{T} = zero(Vec3{T})

    @set! v[get_up_axis()] = convert(T, vertical)

    (a::Int, b::Int) = get_horz_axes()
    @set! v[a] = v_2d.x
    @set! v[b] = v_2d.y

    return v
end

export get_right_handed, get_up_sign, get_up_axis, get_up_vector,
       get_horz_axes, get_horz_vector,
       get_horz, get_vert, to_3d, v_rightward
#