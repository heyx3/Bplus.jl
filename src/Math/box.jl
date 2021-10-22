"
An axis-aligned bounding box of some number of dimensions.
"
struct Box{T<:Union{Vec, Number}}
    min::T
    size::T

    # Replace the default constructor with one that can promote each field's type.
    Box(min::T1, size::T2) where {T1, T2} = Box{promote_type(T1, T2)}(min, size)
    Box{T}(min::T1, size::T2) where {T, T1, T2} = new(
        convert(T, min),
        convert(T, size)
    )
end

Base.print(io::IO, b::Box) = print(io, box_typestr(typeof(b)), "(", b.min, ",", b.size, ")")
Base.show(io::IO, b::Box) = print(io,
    box_typestr(typeof(b)),
    "(min=", b.min,
    " size=", b.size,
    " inMax=", max_inclusive(b),
    ")"
)

Base.:(==)(b1::Box, b2::Box) = (b1.min == b2.min) & (b1.size == b2.size)

export Box


############################
#       Constructors       #
############################

"Constructs an empty box (size 0)"
Box{T}() where {T} = Box{T}(zero(T), zero(T))
"Constructs an empty box (size 0)"
Box(T) = Box(zero(T), zero(T))

"Constructs a box from a min and inclusive max"
Box_minmax(min, max_inclusive) = Box(min, box_typenext(max_inclusive) - min)

"Constructs a box from a min and size"
Box_minsize(min, size) = Box(min, size)

"Constructs a box from an inclusive max and size"
Box_maxsize(max_inclusive, size) = Box(box_typenext(max_inclusive) - size, size)

"Constructs a box that bounds the given points"
function Box_bounding(points_iterator::T) where T
    # Edge-case: no points, return an empty box.
    if isempty(points_iterator)
        return Box(eltype(points_iterator))
    end

    # Find the extrema.
    p_min = typemax(eltype(points_iterator))
    p_max = typemin(eltype(points_iterator))
    for p in points_iterator
        p_min = min(p_min, p)
        p_max = max(p_max, p)
    end

    return Box_minmax(p_min, p_max)
end
"Constructs a box that bounds the given points"
Box_bounding(points::T...) where {T<:Union{Vec, Number}} = Box_bounding(points)

export Box_minmax, Box_minsize, Box_maxsize, Box_bounding


###########################
#        Functions        #
###########################

"Gets whether the box has zero/negative volume"
is_empty(b::Box) = any(b.size <= 0)
export is_empty

"Gets the dimensionality of a box"
get_dimensions(::B) where {B<:Box} = get_dimensions(B)
get_dimensions(::Type{Box{T}}) where {T<:Number} = 1
get_dimensions(::Type{Box{Vec{N, T}}}) where {N, T} = N
export get_dimensions

"Gets the number type of a box (int, float, etc.)"
get_number_type(::B) where {B<:Box} = get_number_type(B)
get_number_type(::Type{Box{T}}) where {T<:Number} = T
get_number_type(::Type{Box{Vec{N, T}}}) where {N, T} = T
export get_number_type

"Gets the point just past the corner of the Box"
max_exclusive(b::Box) = b.min + b.size
"Gets the corner point of the Box"
max_inclusive(b::Box) = box_typeprev(get_max_exclusive(b))
export max_exclusive, max_inclusive

"Gets the N-dimensional size of a Box"
volume(b::Box) = reduce(*, b.size)
export volume

"Gets whether the point is somewhere in the box"
is_touching(box::Box{T}, point::T) where {T} = all(
    (point >= box.min) &
    (point <= box.max)
)
export is_touching

"
Gets whether the point is fully inside the box, not touching the edges.
This is more useful for integer boxes than floating-point ones.
"
is_inside(box::Box{T}, point::T) where {T} = all(
    (point > box.min) &
    (point < max_inclusive(box))
)
export is_inside

"
Calculate the intersection of two boxes.
If they don't intersect, it will have 0 or negative size (see `is_empty()`).
"
Base.intersect(b::Box) = b
function Base.intersect(b1::Box{T}, b2::Box{T}) where {T}
    b_min::T = max(b1.min, b2.min)
    b_max::T = min(max_inclusive(b1), max_inclusive(b2))
    # If using unsigned numbers, we can't have a negative size, so keep the size at least 0.
    if T <: Unsigned
        b_max = max(b_max, b_min)
    end
    return Box_minmax(b_min, b_max)
end
Base.intersect(b1::Box{T}, b2::Box{T}...) where {T} = foldl(intersect, b2, init=b1)

"
Calculate the union of boxes, a.k.a. a bigger box that contains all of them.
"
Base.union(b::Box) = b
Base.union(b1::Box{T}, b2::Box{T}) where {T} = Box_minmax(
    min(b1.min, b2.min),
    max(max_inclusive(b1), max_inclusive(b2))
)
Base.union(b1::Box{T}, b2::Box{T}...) where {T} = foldl(union, b2, init=b1)

"Gets the closest point in a box to the given point"
closest_point(b::Box{T}, p::T) where {T} = clamp(p, b.min, max_inclusive(b))

"
Changes the dimensionality of the box.
By default, new dimensions are given the smallest non-zero size.
"
@inline function Base.reshape( b::Box{Vec{OldN, T}},
                               NewN::Int,
                               new_dims_size::T = box_typenext(zero(T)),
                             ) where {OldN, T}
    if (NewN > OldN)
        return Box(Vec{NewN, T}(b.min, Vec{NewN - OldN, T}(i -> zero(T))),
                   Vec{NewN, T}(b.size, Vec{NewN - OldN, T}(i -> new_dims_size)))
    else
        return Box(b.min[1:NewN], b.size[1:NewN])
    end
end
@inline function Base.reshape(b::Box{T}, N::Int) where {T<:Number}
    if N != 1
        return reshape(Box(Vec(b.min), Vec(b.size)), N)
    elseif N == 1
        return b
    else
        error("Can't have a box of less than 1 dimensions: ", N)
    end
end


###########################
#         Aliases         #
###########################

const Interval{T<:Number} = Box{T}
const IntervalI = Interval{Int32}
const IntervalU = Interval{UInt32}
const IntervalF = Interval{Float32}
const IntervalD = Interval{Float64}

const Box2D{T} = Box{Vec{2, T}}
const Box2Di = Box2D{Int32}
const Box2Du = Box2D{UInt32}
const Box2Df = Box2D{Float32}
const Box2Dd = Box2D{Float64}

const Box3D{T} = Box{Vec{3, T}}
const Box3Di = Box3D{Int32}
const Box3Du = Box3D{UInt32}
const Box3Df = Box3D{Float32}
const Box3Dd = Box3D{Float64}

export Interval, Box2D, Box3D,
       IntervalI, IntervalU, IntervalF, IntervalD,
       Box2Di, Box2Du, Box2Df, Box2Dd,
       Box3Di, Box3Du, Box3Df, Box3Dd


##########################################
#         Implementation Details         #
##########################################

"A string representation of a box's type"
box_typestr(::Type{Box{Vec{N, T}}}) where {N, T} = string("Box", N, "D", type_str(T))
box_typestr(::Type{Box{T}}) where {T<:Number} = string("Interval", type_str(T))


# Helpers that generate the "epsilon" between the inclusive and exclusive max.

box_typeprev(n::I) where {I<:Integer} = n - one(I)
box_typeprev(v::Vec{N, I}) where {N, I<:Integer} = v - one(I)
box_typeprev(f::AbstractFloat) = prevfloat(f)
box_typeprev(v::Vec{N, F}) where {N, F<:AbstractFloat} = map(prevfloat, v)

box_typenext(n::I) where {I<:Integer} = n + one(I)
box_typenext(v::Vec{N, I}) where {N, I<:Integer} = v + one(I)
box_typenext(f::AbstractFloat) = nextfloat(f)
box_typenext(v::Vec{N, F}) where {N, F<:AbstractFloat} = map(nextfloat, v)