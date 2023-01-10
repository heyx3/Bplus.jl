"
An axis-aligned bounding box of some number of dimensions.
"
struct Box{T<:Union{Vec, Number}}
    min::T
    size::T

    # Replace the default constructor with one that can promote each field's type.
    Box(min::T1, size::T2) where {T1, T2} = Box{promote_type(T1, T2)}(min, size)
    Box{T}(min::T, size::T) where {T} = new{T}(min, size)
    Box{T}(min::T1, size::T2) where {T, T1, T2} = new{T}(
        convert(T, min),
        convert(T, size)
    )

    # Allow the user to provide a number for one field, and a vector for the other.
    function Box{T}(min::T1, size::Vec{N, T2}) where {N, T<:Vec, T1<:Number, T2<:Number}
        min_f = convert(T.parameters[2], min)
        return new{T}(T(i -> min_f), convert(T, size))
    end
    function Box{T}(min::Vec{N, T1}, size::T2) where {N, T<:Vec, T1<:Number, T2<:Number}
        size_f = convert(T.parameters[2], size)
        return new{T}(convert(T, min), T(i -> size_f))
    end
    Box(min::T, size::Vec{N, T2}) where {N, T<:Number, T2<:Number} = Box{Vec{N, promote_type(T, T2)}}(min, size)
    Box(min::Vec{N, T}, size::T2) where {N, T<:Number, T2<:Number} = Box{Vec{N, promote_type(T, T2)}}(min, size)

    # If 1-D vectors are passed, and the box's type isn't fixed, replace them with scalars.
    Box(min::Vec{1, T}, size::Vec{1, T2}) where {T, T2} = Box(min.x, size.x)
    Box(min::T, size::Vec{1, T2}) where {T<:Number, T2} = Box(min, size.x)
    Box(min::Vec{1, T}, size::T2) where {T, T2<:Number} = Box(min.x, size)
    # If scalars are passed, and the box's type is Vec1, wrap the scalars.
    Box{Vec{1, T}}(min::T2, size::T3) where {T, T2<:Number, T3<:Number} = Box{Vec{1, T}}(Vec(convert(T, min)), Vec(convert(T, size)))
    Box{Vec{1, T}}(min::Vec{1, T2}, size::T3) where {T, T2<:Number, T3<:Number} = Box{Vec{1, T}}(Vec(convert(T, min.x)), Vec(convert(T, size)))
    Box{Vec{1, T}}(min::T2, size::Vec{1, T3}) where {T, T2<:Number, T3<:Number} = Box{Vec{1, T}}(Vec(convert(T, min)), Vec(convert(T, size.x)))
end
StructTypes.StructType(::Type{<:Box}) = StructTypes.OrderedStruct()

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
Box(T) = Box(zero(T), zero(T))

"Constructs a Box covering the given range, *ignoring* the step size."
Box(range::VecRange) = Box_minmax(range.a, range.b)
Box(range::UnitRange) = Box_minmax(first(range), last(range))

"Constructs a box from a min and inclusive max"
Box_minmax(min, max_inclusive) = Box(min, box_typenext(max_inclusive) - min)

"Constructs a box from a min and size"
Box_minsize(min, size) = Box(min, size)

"Constructs a box from an inclusive max and size"
Box_maxsize(max_inclusive, size) = Box(box_typenext(max_inclusive) - size, size)

"Constructs a box bounded by a set of points/boxes"
Box_bounding(point::Union{Number, Vec}) = Box_minsize(point, box_typenext(zero(typeof(point))))
Box_bounding(b::Box) = b
@inline function Box_bounding(a::P1, b::P2, rest...) where {P1<:Union{Number, Vec}, P2<:Union{Number, Vec}}
    p_min = min(a, b)
    p_max = max(a, b)
    return Box_bounding(Box_minmax(p_min, p_max), rest...)
end
@inline function Box_bounding(a::B1, b::B2, rest...) where {B1<:Box, B2<:Box}
    p_min = min(a.min, b.min)
    p_max = max(max_inclusive(a), max_inclusive(b))
    return Box_bounding(Box_minmax(p_min, p_max), rest...)
end
@inline function Box_bounding(b::B, p::P, rest...) where {B<:Box, P<:Union{Number, Vec}}
    p_min = min(b.min, p)
    p_max = max(max_inclusive(b), p)
    return Box_bounding(Box_minmax(p_min, p_max), rest...)
end
@inline function Box_bounding(p::P, b::B, rest...) where {P<:Union{Number, Vec}, B<:Box}
    p_min = min(b.min, p)
    p_max = max(max_inclusive(b), p)
    return Box_bounding(Box_minmax(p_min, p_max), rest...)
end

export Box_minmax, Box_minsize, Box_maxsize, Box_bounding

# Convert between boxes of different type.
Base.convert(::Type{Box{T}}, b::Box{T2}) where {T, T2} = Box(convert(T, b.min), convert(T, b.size))
Base.convert(::Type{Box{T}}, b::Box{T2}) where {T<:Vec{1, <:Number}, T2<:Number} = Box{T}(T(b.min), T(b.size))

# Iterate through the coordinates of integer boxes.
@inline Base.iterate(b::Box{<:Union{Integer, VecT{<:Integer}}}       ) = iterate(b.min:max_inclusive(b)       )
@inline Base.iterate(b::Box{<:Union{Integer, VecT{<:Integer}}}, state) = iterate(b.min:max_inclusive(b), state)
#TODO: Unit tests for Box iteration


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
max_inclusive(b::Box) = box_typeprev(max_exclusive(b))
export max_exclusive, max_inclusive

"Gets the N-dimensional size of a Box"
volume(b::Box) = prod(b.size)
export volume

"Gets whether the point is somewhere in the box"
is_touching(box::Box{T}, point::T) where {T} = all(
    (point >= box.min) &
    (point < max_exclusive(box))
)
export is_touching

"
Gets whether the point is fully inside the box, not touching the edges.
This is primarily for integer boxes.
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
Note that, unlike the union of regular sets, a union of boxes
   will contain other elements that weren't in either set.
"
Base.union(b::Box) = b
Base.union(b1::Box{T}, b2::Box{T}) where {T} = Box_minmax(
    min(b1.min, b2.min),
    max(max_inclusive(b1), max_inclusive(b2))
)
Base.union(b1::Box{T}, b2::Box{T}...) where {T} = foldl(union, b2, init=b1)

"Gets the closest point on or in a box to a point 'p'."
closest_point(b::Box{T}, p::T) where {T} = clamp(p, b.min, max_inclusive(b))

"
Changes the dimensionality of the box.
By default, new dimensions are given the size 1 (both for integer and float boxes).
"
@inline function Base.reshape( b::Box{Vec{OldN, T}},
                               NewN::Int;
                               new_dims_size::T = one(T),
                               new_dims_value::T = zero(T)
                             ) where {OldN, T}
    if (NewN > OldN)
        return Box(vappend(b.min, Vec{NewN - OldN, T}(i -> new_dims_value)),
                   vappend(b.size, Vec{NewN - OldN, T}(i -> new_dims_size)))
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