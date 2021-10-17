using TupleTools, Setfield

"""
A vector math struct.
You can get its data from the field "data",
  or access individual components "x|r", "y|g", "z|b", "w|a",
  or swizzle it, e.x. "v.xz".
Swizzles can also use '0' for a constant 0, '1' for a constant 1,
  '⋀' (\\bigwedge) for the max finite value, and '⋁' (\\bigvee) for the min finite value.
For example, swizzling a Vec{4, UInt8} with "v.rrr⋀" results in
  a vector with the RGB components set to the original red component,
  and the alpha set to 255.
You can also treat it like an AbstractVector, with indices, `map()`, etc.
To change how many digits are printed in the REPL (i.e. Base.show()),
  set VEC_N_DIGITS or call `use_vec_digits()`.
NOTE: Broadcasting is not specialized; the output is an array instead of another Vec,
  because I don't know how to overload broadcasting correctly.
NOTE: Comparing two vectors with == or != returns a boolean as expected,
  but other comparisons (<, <=, >, <=) return a component-wise result.
"""
struct Vec{N, T} <: AbstractVector{T}
    data::NTuple{N, T}

    # Construct with individual components.
    Vec(data::NTuple{N, T}) where {N, T} = new{N, T}(data)
    Vec(data::T...) where {T} = Vec(data)
    Vec(data::Any...) = Vec(promote(data...))

    # Construct with a type parameter, and set of values.
    function Vec{T}(data::NTuple{N, T2}) where {N, T, T2}
        @bp_check(!(T isa Int), "Constructors of the form 'VecN(x, y, ...)' aren't allowed. Try 'Vec(x, y, ...)', or 'VecN{T}(x, y, ...)'")
        return new{N, T}(convert(NTuple{N, T}, data))
    end
    function Vec{T}(data::T2...) where {T, T2}
        @bp_check(!(T isa Int), "Constructors of the form 'VecN(x, y, ...)' aren't allowed. Try 'Vec(x, y, ...)', or 'VecN{T}(x, y, ...)'")
        return Vec{T}(data)
    end
    Vec{N, T}(data::NTuple{N, T2}) where {N, T, T2} = Vec{T}(convert(NTuple{N, T}, data))
    Vec{N, T}(data::T2...) where {N, T, T2} = Vec{N, T}(data)

    # "Empty" constructor makes a value with all 0's.
    Vec{N, T}() where {N, T} = new{N, T}(ntuple(i->zero(T), N))

    # Construct by appending smaller vectors/components together.
    Vec(first::Vec{N, T}, rest::T2...) where {N, T, T2} = Vec(promote(first..., rest...))
    Vec(first::Vec{N, T}, rest::Vec{N2, T2}) where {N, N2, T, T2} = Vec(promote(first..., rest...))
end
export Vec

###############
#   Aliases   #
###############

# Note that in gamedev, 32-bit floats are far more common than 64-bit, for performance reasons.
# Especially when passing data to the GPU --
#    commercial gaming GPU's can have pretty terrible float64 performance.

const Vec2{T} = Vec{2, T}
const Vec3{T} = Vec{3, T}
const Vec4{T} = Vec{4, T}

const vRGB{T} = Vec{3, T}
const vRGBu8 = vRGB{UInt8}
const vRGBi8 = vRGB{Int8}
const vRGBf = vRGB{Float32}

const vRGBA{T} = Vec{4, T}
const vRGBAu8 = vRGBA{UInt8}
const vRGBAi8 = vRGBA{Int8}
const vRGBAf = vRGBA{Float32}

const VecF{N} = Vec{N, Float32}
const v2f = VecF{2}
const v3f = VecF{3}
const v4f = VecF{4}

const VecI{N} = Vec{N, Int32}
const v2i = VecI{2}
const v3i = VecI{3}
const v4i = VecI{4}

const VecU{N} = Vec{N, UInt32}
const v2u = VecU{2}
const v3u = VecU{3}
const v4u = VecU{4}

const VecB{N} = Vec{N, Bool}
const v2b = VecB{2}
const v3b = VecB{3}
const v4b = VecB{4}

const VecF64{N} = Vec{N, Float64}
const VecI64{N} = Vec{N, Int64}
const VecU64{N} = Vec{N, UInt64}

export Vec2, Vec3, Vec4,
       vRGB, vRGBA,
       vRGBu8, vRGBi8, vRGBf,
       vRGBAu8, vRGBAi8, vRGBAf,
       VecF, VecI, VecU, VecB,
       VecF64, VecI64, VecU64,
       v2f, v3f, v4f,
       v2i, v3i, v4i,
       v2u, v3u, v4u,
       v2b, v3b, v4b
#

################
#   Printing   #
################

# Base.show() prints the vector with a certain number of digits.

VEC_N_DIGITS = 2

printable_component(f::AbstractFloat, n_digits::Int) = round(f; digits=n_digits)
printable_component(f::Integer, n_digits::Int) = f

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
function use_vec_digits(to_do::Function, n::Int)
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

#################################
#   Base overloads/interfaces   #
#################################

Base.zero(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}()
Base.one(::Type{Vec{N, T}}) where {N, T} = Vec{N, T}(ntuple(i->one(T), N))

Base.convert(::Type{Vec{N, T2}}, v::Vec{N, T}) where {N, T, T2} = map(T2, v)

Base.getindex(v::Vec, i::Int) = v.data[i]
Base.eltype(::Vec{N, T}) where {N, T} = T
Base.length(::Vec{N, T}) where {N, T} = N
Base.size(::Vec{N, T}) where {N, T} = (N, )
Base.IndexStyle(::Vec{N, T}) where {N, T} = IndexLinear()
Base.iterate(v::Vec, state...) = iterate(v.data, state...)
Base.map(f, v::Vec) = Vec(map(f, v.data))

@inline function Base.foldl(func::F, v::Vec{N, T}) where {F, N, T}
    f::T = v[1]
    for i in 2:N
        f = func(f, v[i])
    end
    return f
end
@inline function Base.foldl(func::F, v::Vec{N, T}, init::T2)::T2 where {F, N, T, T2}
    output::T2 = init
    for f::T in v
        output = func(output, f)
    end
    return output
end
@inline function Base.foldr(func::F, v::Vec{N, T}) where {F, N, T}
    f::T = v[end]
    for i in (N-1):-1:1
        f = func(f, v[i])
    end
    return f
end
@inline function Base.foldr(func::F, v::Vec{N, T}, init::T2)::T2 where {F, N, T, T2}
    f::T2 = init
    for i in N:-1:1
        f = func(f, v[i])
    end
    return f
end

# I can't figure out how to make the general-case form of `isapprox()` work
#   without heap allocations (I think it's related to the keyword arguments).
# I tried all sorts of stuff, including @generate.
# So here are two simple, common cases.
# Note that I've done the same thing for Quaternions.
Base.isapprox(a::Vec{N, T1}, b::Vec{N, T2}) where {N, T1, T2} =
    all(t -> isapprox(t[1], t[2]), zip(a, b))
Base.isapprox(a::Vec{N, T1}, b::Vec{N, T2}, atol) where {N, T1, T2} =
    all(t -> isapprox(t[1], t[2]; atol=atol), zip(a, b))


#TODO: Implement broadcasting. I tried already, but turns out it's really complicated...

@inline function Setfield.ConstructionBase.setproperties(v::Vec{N, T}, patch::NamedTuple)::Vec{N, T} where {N, T}
    for name::Symbol in propertynames(patch)
        v = v_setproperty(v, name, getfield(patch, name))
    end
    return v
end

@inline function v_setproperty( v::Vec{N, T},
                                n::Symbol,
                                val::Union{T2, NTuple{N, T2}}
                              )::Vec{N, T} where {N, T, T2}
    if (n == :x) | (n == :r)
        return Vec(@set getfield(v, :data)[1] = convert(T, val::T2))
    elseif (n == :y) | (n == :g)
        return Vec(@set getfield(v, :data)[2] = convert(T, val::T2))
    elseif (n == :z) | (n == :b)
        return Vec(@set getfield(v, :data)[3] = convert(T, val::T2))
    elseif (n == :w) | (n == :a)
        return Vec(@set getfield(v, :data)[4] = convert(T, val::T2))
    elseif n == :data
        return Vec{N, T}(val::NTuple{N, T2})
    else
        error("No property on Vec: '", n, "'")
    end
end

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
@inline Base.getproperty(v::Vec, ::Val{T}) where {T} = swizzle(v, T)

Base.propertynames(::Vec, _) = (:x, :y, :z, :w, :data)
swizzle(v::Vec{N, T}, n::Symbol) where {N, T} = swizzle(v, Val(n))
#TODO: I think we can make all swizzles fast by using @generated with the argument type Val{S}.

Base.:(+)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i+j for (i,j) in zip(a, b))...)
Base.:(-)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i-j for (i,j) in zip(a, b))...)
Base.:(*)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i*j for (i,j) in zip(a, b))...)
Base.:(/)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i/j for (i,j) in zip(a, b))...)

Base.:(+)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f+b), a)
Base.:(-)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f-b), a)
Base.:(*)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f*b), a)
Base.:(/)(a::Vec{N, T}, b::T2) where {N, T, T2<:Number} = map(f->(f/b), a)

Base.:(+)(a::Number, b::Vec) = b+a
Base.:(-)(a::Number, b::Vec) = (-b)+a
Base.:(*)(a::Number, b::Vec) = b*a
Base.:(/)(a::Number, b::Vec) = map(f->(a/f), b)

Base.:(-)(a::Vec)::Vec = map(-, a)

Base.:(<)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i<j for (i,j) in zip(a, b))...)
Base.:(<)(a::Vec{N, T}, b::T2) where {N, T, T2} = map(x -> x<b, a)
Base.:(<)(a::T2, b::Vec{N, T}) where {N, T, T2} = map(x -> a<x, b)
Base.:(<=)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i<=j for (i,j) in zip(a, b))...)
Base.:(<=)(a::Vec{N, T}, b::T2) where {N, T, T2} = map(x -> x<=b, a)
Base.:(<=)(a::T2, b::Vec{N, T}) where {N, T, T2} = map(x -> a<=x, b)

@inline Base.:(&)(a::VecB{N}, b::VecB{N}) where {N} = Vec((i&j for (i,j) in zip(a, b))...)
Base.:(&)(a::VecB{N}, b::Bool) where {N} = map(x -> x&b, a)
@inline Base.:(&)(a::Bool, b::VecB) = b & a

@inline Base.:(|)(a::VecB{N}, b::VecB{N}) where {N} = Vec((i|j for (i,j) in zip(a, b))...)
Base.:(|)(a::VecB{N}, b::Bool) where {N} = map(x -> x|b, a)
@inline Base.:(|)(a::Bool, b::VecB) = b | a


#######################
#   Mutable Vectors   #
#######################

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
Base.convert(::Type{Vec{N, T}}, v::MVec{N, T2}) where {N, T, T2} = Vec(map(T, v.data))
Base.convert(::Type{Vec{N, T}}, v::MVec{N, T}) where {N, T} = Vec(v.data)
Base.convert(::Type{MVec{N, T}}, v::Vec{N, T}) where {N, T} = MVec(v.data)
Base.convert(::Type{MVec{N, T}}, v::Vec{N, T2}) where {N, T, T2} = MVec(map(T, v.data))
# AbstractArray iterface for MVec:
Base.getindex(v::MVec, i::Int) = v.data[i]
function Base.setindex!(v::MVec{N, T}, val::T, i::Int) where {N, T}
    d::NTuple{N, T} = v.data
    @set! d[i] = val
    v.data = d
end
@inline function Base.setproperty!( v::MVec{N, T},
                                    name::Symbol,
                                    val::Union{T, NTuple{N, T}}
                                  ) where {N, T}
    if (name == :x) | (name == :r)
        setfield!(v, :data, @set(getfield(v, :data)[1] = val::T))
    elseif (name == :y) | (name == :g)
        setfield!(v, :data, @set(getfield(v, :data)[2] = val::T))
    elseif (name == :z) | (name == :b)
        setfield!(v, :data, @set(getfield(v, :data)[3] = val::T))
    elseif (name == :w) | (name == :a)
        setfield!(v, :data, @set(getfield(v, :data)[4] = val::T))
    elseif (name == :data)
        setfield!(v, :data, val::NTuple{N, T})
    else
        error("Invalid field of MVec: '", name, "'")
    end
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
Base.copy!(dest::MVec{N, T}, src::Vec{N, T2}) where {N, T, T2} = (dest.data = map(T, src.data))

Base.similar(v::Vec) = MVec(v.data)
Base.similar(v::Vec{N, T}, ::Type{T2}) where {N, T, T2} = MVec(map(T2, v.data))
function Base.similar(v::Vec{N, T}, ::Type{T2}, dims::Tuple{Int}) where {N, T, T2}
    N2 = dims[1]
    v2::Vec{N2, T2} = Vec(ntuple(i->zero(T2), N2))

    #TODO: Use broadcasting instead
    for i in 1:N
        @set! v2[i] = convert(T2, v[i])
    end

    return v2
end

#######################
#   Other functions   #
#######################

# The general use-case of swizzle, which is forced to stringify the symbol
#    to iterate its characters.
# See below for specific optimized overloads.
swizzle(v::Vec, ::Val{T}) where {T} = swizzle(v, string(T))
@inline function swizzle(v::Vec{N, T}, c_str::S) where {N, T} where {S<:AbstractString}
    # Unfortunately, no way to do this with symbols.
    # Symbols don't provide a way to access their characters.
    #TODO: Check whether this function is truly inlined, and if the stringification can get compiled out.
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
            @set! v2[i] = one(T)
        elseif (component == '0')
            @set! v2[i] = zero(T)
        elseif (component == '⋀')
            @set! v2[i] = typemax_finite(T)
        elseif (component == '⋁')
            @set! v2[i] = typemin_finite(T)
        else
            error("Invalid swizzle char: '", component, "'")
        end
    end

    return v2
end

"Computes the dot product of two vectors"
vdot(v1::Vec, v2::Vec) = sum(Iterators.map(t->t[1]*t[2], zip(v1, v2)))
export vdot

"Computes the square distance between two vectors"
@inline function vdist_sqr(v1::Vec{N, T1}, v2::Vec{N, T2})::promote_type(T1, T2) where {N, T1, T2}
    delta::Vec{N, promote_type(T1, T2)} = v1 - v2
    delta = map(f -> f*f, delta)
    return sum(delta)
end
"Computes the distance between two vectors"
vdist(v1::Vec, v2::Vec) = sqrt(vdist_sqr(v1, v2))
export vdist_sqr, vdist

"Computes the square length of a vector"
@inline function vlength_sqr(v::Vec{N, T})::T where {N, T}
    return vdot(v, v)
end
"Computes the length of a vector"
@inline vlength(v::Vec, out_type=Float64)::out_type = convert(out_type, sqrt(vlength_sqr(v)))
export vlength_sqr, vlength

"Normalizes a vector"
vnorm(v::Vec) = v / vlength(v)
export vnorm

"Checks whether a vector is normalized, within the given epsilon"
@inline is_normalized(v::Vec{N, T}, atol::T2 = 0.0) where {N, T, T2} =
    isapprox(vlength_sqr(v), convert(T, 1.0); atol=atol*atol)
export is_normalized

"Computes the 3D cross product."
@inline function vcross( a::Vec3{T1},
                         b::Vec3{T2}
                       )::Vec3{promote_type(T1, T2)} where {T1, T2}
    return Vec(foldl(-, a.yz * b.zy),
               foldl(-, a.zx * b.xz),
               foldl(-, a.xy * b.yx))
end
export vcross


##########################
#   Symbolic operators   #
##########################

# Define some mathematical aliases for some vector ops, in case anybody likes mathy syntax.

"The \\cdot character represents the dot product."
const ⋅ = vdot
"The \\circ character also represents the dot product, as it's easier to read than the dot."
const ∘ = vdot
"The \\times character represents the cross product."
const × = vcross
export ⋅, ∘, ×


###########################
#   Vertical Coordinates  #
###########################

# Different 3D environments use different up-axes, usually Y or Z.
# Some helpers are provided to separate the horizontal and vertical components of a vector,
#    and they can be configured dynamically to recompile for a different up axis
#    for your particular project.

"""
Gets the vertical axis -- 1=X, 2=Y, 3=Z.
Redefine this to set the vertical axis for your project.
Julia's JIT should ensure it's a compile-time constant, even after redefinition.
By default, uses Z as the up-axis.
"""
get_up_axis()::Int = 3

"""
Gets the sign of the "Upward" direction of the vertical axis.
For example, if `get_up_axis()==3 && get_up_sign()==-1`,
  then the "Up" direction is pointing in the -Z direction.
Like `get_up_axis()`, you can redefine this to set the value for your project.
By default, points in the positive direction.
"""
get_up_sign()::Int = 1


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
    return TupleTools.sort(idcs)
end

"Extracts the horizontal components from a 3D vector."
@inline function get_horz(v::Vec3)
    (a::Int, b::Int) = get_horz_axes()
    Vec(v[a], v[b])
end

"Extracts the vertical component from a 3D vector."
get_vert(v::Vec3) = v[get_up_axis()]

"Inserts a vertical component into the given horizontal vector."
@inline function to_3d(v_2d::Vec2{T}, vertical::T2 = zero(T))::Vec3{T} where {T, T2}
    v::Vec3{T} = zero(Vec3{T})
    
    @set! v[get_up_axis()] = convert(T, vertical)

    (a::Int, b::Int) = get_horz_axes()
    @set! v[a] = v_2d.x
    @set! v[b] = v_2d.y

    return v
end

export get_up_sign, get_up_axis, get_up_vector,
       get_horz_axes,
       get_horz, get_vert, to_3d
#

###########################
#   Optimized swizzling   #
###########################

"""
Defines a fast implementation of swizzling for a specific swizzle.
Valid swizzle chars are x, y, z, w, 0, 1, ⋀, ⋁.
RGBA swizzles are generated along with the XYZW swizzles.
Feel free to add this macro to your own code, for specific swizzles
    that you want to be high-performance.
"""
macro fast_swizzle(components_xyzw::Union{Symbol, Int}...)
    # If there is only one component, we don't actually need swizzling behavior for it.
    # Handling this edge-case makes it easier to generate many fast swizzles with other helper macros.
    if length(components_xyzw) == 1
        return :()
    end

    # Compute the RGBA version of the components.
    components_rgba = map(components_xyzw) do c
        if c isa Int
            return c
        elseif c == :x
            return :r
        elseif c == :y
            return :g
        elseif c == :z
            return :b
        elseif c == :w
            return :a
        elseif c in (:⋀, :⋁)
            return c
        else
            error("Swizzle char isn't valid (xyzw/rgba/01/!¡): '", c, "'")
        end
    end

    component_sets = (components_xyzw, components_rgba)

    # Generate the swizzle symbols (e.x. ":xyzw") for both XYZW and RGBA versions.
    swizzle_symbol_exprs = map(cs -> QuoteNode(Symbol(cs...)),
                               component_sets)
    # Don't use the RGBA version if it isn't actually different
    #   (e.x. the swizzle 'v.0001' -- very unlikely edge-case, but still).
    if eval(swizzle_symbol_exprs[1]) == eval(swizzle_symbol_exprs[2])
        component_sets = component_sets[1]
        swizzle_symbol_exprs = swizzle_symbol_exprs[1]
    end

    # Generate expressions to evaluate each swizzle component.
    # Note that this will always behave the same for the XYZW and RGBA versions,
    #    so we only need one copy.
    components_expr = map(component_sets[1]) do c
        if c == 0
            return :( zero(T) )
        elseif c == 1
            return :( one(T) )
        elseif c == :⋀
            return :( typemax_finite(T) )
        elseif c == :⋁
            return :( typemin_finite(T) )
        elseif (c == :x) || (c == :r)
            return :( data[1] )
        elseif (c == :y) || (c == :g)
            return :( data[2] )
        elseif (c == :z) || (c == :b)
            return :( data[3] )
        elseif (c == :w) || (c == :a)
            return :( data[4] )
        else
            error("Unknown component in macro: '", c, "'")
        end
    end

    # Generate the XYZW and RGBA swizzle overloads.
    output = Expr(:block)
    for swizzle_symbol_expr in swizzle_symbol_exprs
        push!(output.args, :(
            @inline function Base.getproperty( v::Vec{N, T},
                                               ::Val{$swizzle_symbol_expr}
                                             ) where {N, T}
                data::NTuple{N, T} = getfield(v, :data)
                return Vec($(components_expr...))
            end
        ))
    end
    return esc(output)
end

# Define fast cases for all one-component swizzles like "v.xx", "v.yyyy", "v.bb", etc.
macro fast_swizzle_single(component::Symbol)
    @bp_check(length(string(component)) == 1)
    return esc(quote
        Math.@fast_swizzle $component $component
        Math.@fast_swizzle $component $component $component
        Math.@fast_swizzle $component $component $component $component
    end)
end
@fast_swizzle_single x
@fast_swizzle_single y
@fast_swizzle_single z
@fast_swizzle_single w
# Define fast cases for all two-component swizzles of XYZ, and of XW (for red-alpha textures).
@fast_swizzle x y
@fast_swizzle y x
@fast_swizzle x z
@fast_swizzle z x
@fast_swizzle y z
@fast_swizzle z y
@fast_swizzle x w
@fast_swizzle w x
@fast_swizzle x 0
@fast_swizzle x 1
@fast_swizzle x ⋀
@fast_swizzle x ⋁
# Define fast cases for simple three-component swizzles, optionally with constant alpha.
@fast_swizzle x y z
@fast_swizzle x y z 0
@fast_swizzle x y z 1
@fast_swizzle x y z ⋀
@fast_swizzle x y z ⋁
@fast_swizzle z y x
@fast_swizzle z y x w
@fast_swizzle z y x 0
@fast_swizzle z y x 1
@fast_swizzle z y x ⋀
@fast_swizzle z y x ⋁
# Define fast cases that turn Red-Alpha color data into RGBA data.
@fast_swizzle x x x w
# Define fast cases that turn greyscale data into RGBA data.
@fast_swizzle x x x 0
@fast_swizzle x x x 1
@fast_swizzle x x x ⋀
@fast_swizzle x x x ⋁