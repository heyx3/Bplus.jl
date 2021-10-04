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


###############
#   Aliases   #
###############

# Note that in gamedev, 32-bit floats are far more common than 64-bit.

Vec2{T} = Vec{2, T}
Vec3{T} = Vec{3, T}
Vec4{T} = Vec{4, T}

vRGB{T} = Vec{3, T}
vRGBA{T} = Vec{4, T}

VecF{N} = Vec{N, Float32}
v2f = Vec{2, Float32}
v3f = Vec{3, Float32}
v4f = Vec{4, Float32}

VecI{N} = Vec{N, Int32}
v2i = Vec{2, Int32}
v3i = Vec{3, Int32}
v4i = Vec{4, Int32}

VecU{N} = Vec{N, UInt32}
v2u = Vec{2, UInt32}
v3u = Vec{3, UInt32}
v4u = Vec{4, UInt32}

VecF64{N} = Vec{N, Float64}
VecI64{N} = Vec{N, Int64}
VecU64{N} = Vec{N, UInt64}

export Vec2, Vec3, Vec4,
       vRGB, vRGBA,
       VecF, VecI, VecU,
       VecF64, VecI64, VecU64,
       v2f, v3f, v4f,
       v2i, v3i, v4i,
       v2u, v3u, v4u
#

################
#   Printing   #
################

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

"Checks whether two vectors are equal, within the given per-component error."
Base.isapprox(a::Vec{N, T}, b::Vec{N, T2}; kw...) where {N, T, T2} =
    all(isapprox(f1, f2; kw...) for (f1, f2) in zip(a, b))

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
Base.propertynames(v::Vec, _) = (:x, :y, :z, :w, :data)
    
swizzle(v::Vec, n::Symbol) = swizzle(v, Val(n))

Base.:(+)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i+j for (i,j) in zip(a, b))...)
Base.:(-)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i-j for (i,j) in zip(a, b))...)
Base.:(*)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i*j for (i,j) in zip(a, b))...)
Base.:(/)(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Vec((i/j for (i,j) in zip(a, b))...)

Base.:(+)(a::Vec{N, T}, b::T2) where {N, T, T2<:Real} = map(f->(f*b), a)
Base.:(-)(a::Vec{N, T}, b::T2) where {N, T, T2<:Real} = map(f->(f-b), a)
Base.:(*)(a::Vec{N, T}, b::T2) where {N, T, T2<:Real} = map(f->(f*b), a)
Base.:(/)(a::Vec{N, T}, b::T2) where {N, T, T2<:Real} = map(f->(f/b), a)

Base.:(+)(a, b::Vec) = b+a
Base.:(-)(a, b::Vec) = b-a
Base.:(*)(a, b::Vec) = b*a
Base.:(/)(a, b::Vec) = b/a

Base.:(-)(a::Vec) = map(-, a)



#TODO: Bool vectors


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
export vdot

"Computes the square distance between two vectors"
vdist_sqr(v1::Vec, v2::Vec) = sum(f*f for f in (a-b for (a,b) in zip(v1, v2)))
"Computes the distance between two vectors"
vdist(v1::Vec, v2::Vec) = sqrt(vdist_sqr(v1, v2))
export vdist_sqr, vdist

"Computes the square length of a vector"
vlength_sqr(v::Vec) = v⋅v
"Computes the length of a vector"
vlength(v::Vec) = sqrt(vlength_sqr(v))
export vlength_sqr, vlength

"Normalizes a vector"
vnorm(v::Vec) = v / vlength(v)
export vnorm

"Checks whether a vector is normalized, within the given epsilon"
@inline is_normalized(v::Vec{N, T}, eps::T) where {N, T} =
    isapprox(vlength_sqr(v), convert(T, 1.0); atol=eps*eps)
export is_normalized

"Computes the 3D cross product."
@inline vcross(a::Vec3, b::Vec3) = Vec(
    foldl(-, a.yz * b.zy),
    foldl(-, a.zx * b.xz),
    foldl(-, a.xy * b.yx)
)
export vcross


##########################
#   Symbolic operators   #
##########################

# Define some mathematical aliases for some vector ops, in case anybody likes mathy syntax.

"The \\cdot character represents the dot product."
⋅ = vdot
"The \\circ character also represents the dot product, as it's easier to read than the dot."
∘ = vdot
"The \\times character represents the cross product."
× = vcross
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
By default, defines Z as the up-axis.
"""
get_up_axis()::Int = 3

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

export get_up_axis, get_horz_axes,
       get_horz, get_vert
#

###########################
#   Optimized swizzling   #
###########################

"""
Defines a fast implementation of swizzling for a specific swizzle.
Valid swizzle chars are x, y, z, w, 0, 1.
RGBA swizzles are generated along with the XYZW swizzles.
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
        else
            error("Swizzle char isn't valid (xyzw/rgba/01): '", c, "'")
        end
    end

    component_sets = (components_xyzw, components_rgba)

    # Generate the swizzle symbols for both versions.
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
    components_expr = map(component_sets) do c
        if c == 0
            return :( zero(T) )
        elseif c == 1
            return :( one(T) )
        else
            return :( v.$c )
        end
    end

    # Generate the XYZW and RGBA swizzle overloads.
    output = Expr(:block)
    for swizzle_symbol_expr in swizzle_symbol_exprs
        push!(output.args, :(
            Math.swizzle( v::Vec{N, T},
                           ::Val{$swizzle_symbol_expr}
                        ) where {N, T} =
                Vec($(components_expr...))
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

@fast_swizzle x y
@fast_swizzle y x
@fast_swizzle x z
@fast_swizzle z x
@fast_swizzle y z
@fast_swizzle z y

@fast_swizzle x y z
@fast_swizzle x y z 0
@fast_swizzle x y z 1

@fast_swizzle z y x
@fast_swizzle z y x w
@fast_swizzle z y x 0
@fast_swizzle z y x 1

@fast_swizzle x x x w
@fast_swizzle x x x 0
@fast_swizzle x x x 1