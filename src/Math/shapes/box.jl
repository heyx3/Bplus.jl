"
An axis-aligned bounding box of some number of dimensions.

Can be deserialized through StructTypes using almost any pair of properties --
   'min' + 'size'; 'min' + 'max'; 'max' + 'size'; 'center' + 'size', etc.
"
struct Box{N, F} <: AbstractShape{N, F}
    min::Vec{N, F}
    size::Vec{N, F}
end
export Box

println("#TODO: Test Box serialization flexibility, then port any fixes into Interval")
StructTypes.StructType(::Type{<:Box}) = StructTypes.CustomStruct()
StructTypes.lower(b::Box) = Dict("min"=>min_inclusive(b), "size"=>size(b))
StructTypes.lowertype(::Type{<:Box}) = Dict{AbstractString, Vec}
function StructTypes.construct(::Type{Box{N, F}}, d::Dict{AbstractString, Vec{N, Float64}}) where {N, F}
    pairs = collect(d)
    keys = map(kvp -> kvp[1], pairs)
    values = map(kvp -> kvp[2], pairs)
    return Box{N, F}(namedtuple(keys, values))
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

# Data interface:
@inline min_inclusive(b::Box) = b.min
@inline max_exclusive(b::Box) = b.min + b.size
@inline min_exclusive(b::Box) = box_typeprev(min_inclusive(b))
@inline max_inclusive(b::Box) = box_typeprev(max_exclusive(b))
Base.size(b::Box) = b.size
export min_inclusive, min_exclusive,
       max_inclusive, max_exclusive


###########################
#         Aliases         #
###########################

const BoxT{F, N} = Box{N, F}

const BoxI{N} = Box{N, Int32}
const BoxU{N} = Box{N, UInt32}
const BoxF{N} = Box{N, Float32}
const BoxD{N} = Box{N, Float64}

const Box2D{T} = Box{2, T}
const Box2Di = Box2D{Int32}
const Box2Du = Box2D{UInt32}
const Box2Df = Box2D{Float32}
const Box2Dd = Box2D{Float64}

const Box3D{T} = Box{3, T}
const Box3Di = Box3D{Int32}
const Box3Du = Box3D{UInt32}
const Box3Df = Box3D{Float32}
const Box3Dd = Box3D{Float64}

export BoxT, Box2D, Box3D,
       Box2Di, Box2Du, Box2Df, Box2Dd,
       Box3Di, Box3Du, Box3Df, Box3Dd,
       BoxI, BoxU, BoxF, BoxD


###########################################
#         AbstractShape interface         #
###########################################

volume(b::Box) = prod(b.size)
bounds(b::Box) = b
center(b::BoxT{F}) where {F} = b.min + (b.size / convert(F, 2))
is_touching(box::Box{N, F1}, point::Vec{N, F2}) where {N, F1, F2} = all(
    (point >= box.min) &
    (point < max_exclusive(box))
)
closest_point(b::Box{N, F1}, p::Vec{N, F2}) where {N, F1, F2} = convert(Vec{N, F1}, clamp(p, b.min, max_inclusive(b)))


############################
#       Constructors       #
############################

"Constructs an empty box (size 0)"
Box{N, F}() where {N, F} = Box{N, F}(zero(Vec{N, F}), zero(Vec{N, F}))

"Constructs a box, inferring types and automatically broadcasting scalars into vectors"
function Box(min::TMin, size::TSize) where {TMin, TSize}
    if !isa(min, Vec)
        n = isa(size, Vec) ? length(size) : 1
        return Box(Vec(i -> min, n), size)
    elseif !isa(size, Vec)
        return Box(min, Vec(i -> size, length(min)))
    elseif length(min) != length(size)
        error("Mismatch in dimensions: ", TMin, " vs ", TSize)
    else
        return Box{length(min), promote_type(eltype(min), eltype(size))}(min, size)
    end
end

"Creates a box given a min and size"
@inline Box(data::Union{NamedTuple{(:min, :size)}, NamedTuple{(:size, :min)}}) =
    Box(data.min, data.size)
"Creates a box given a min and *inclusive* max"
@inline Box(data::Union{NamedTuple{(:min, :max)}, NamedTuple{(:max, :min)}}) =
    Box(data.min, box_typenext(data.max) - data.min)
"Creates a box given a size and *inclusive* max"
@inline Box(data::Union{NamedTuple{(:max, :size)}, NamedTuple{(:size, :max)}}) =
    Box(box_typenext(data.max) - data.size, data.size)
"Creates a box given a center and size"
@inline function Box(data::Union{NamedTuple{(:center, :size)}, NamedTuple{(:size, :center)}})
    component_type = promote_type(
        (data.center isa Vec) ? eltype(data.center) : typeof(data.center),
        (data.size isa Vec) ? eltype(data.size) : typeof(data.size)
    )
    half_size = data.size / convert(component_type, 2)
    return Box(data.center - half_size, data.size)
end
@inline Box{N, F}(data::NamedTuple) where {N, F} = convert(Box{N, F}, Box(data))

"Constructs a Box covering the given range. Ignores the step size."
@inline Box(range::VecRange) = Box((min=range.a, max=range.b))
@inline Box(range::UnitRange) = Box((min=first(range), max=last(range)))

##  Boundary  ##

"Constructs a box bounded by a set of points/shapes"
boundary(point::Union{Number, Vec}) = Box((min=point, size=box_typenext(zero(typeof(point)))))
boundary(s::AbstractShape) = bounds(s)
boundary(b::Box) = b

@inline function boundary(a::P1, b::P2, rest...) where {P1<:Union{Number, Vec}, P2<:Union{Number, Vec}}
    return boundary(Box((min=min(a, b), max=max(a, b))), rest...)
end

@inline function boundary(a::B1, b::B2, rest...) where {B1<:Box, B2<:Box}
    return boundary(Box((min=min(min_inclusive(a), min_inclusive(b)),
                         max=max(max_inclusive(a), max_inclusive(b)))),
                    rest...)
end
@inline boundary(a::AbstractShape, b::AbstractShape, rest...) = boundary(bounds(a), bounds(b), rest...)

@inline function boundary(b::B, p::P, rest...) where {B<:Box, P<:Union{Number, Vec}}
    return boundary(Box((min=min(min_inclusive(b), p),
                         max=max(max_inclusive(b), p))),
                    rest...)
end
@inline boundary(a::AbstractShape, p::Union{Number, Vec}, rest...) = boundary(bounds(a), p, rest...)

@inline function boundary(p::P, b::B, rest...) where {P<:Union{Number, Vec}, B<:Box}
    return boundary(Box((min=min(min_inclusive(b), p),
                         max=max(max_inclusive(b), p))),
                    rest...)
end
@inline boundary(p::Union{Number, Vec}, s::AbstractShape, rest...) = boundary(p, bounds(s), p, rest...)

@inline function boundary(points::AbstractVector)
    @bp_check(!isempty(points), "Bounding points list is empty; return type can't be inferred!")
    p_min::eltype(points) = points[1]
    p_max::eltype(points) = points[1]
    for point in Iterators.drop(points, 1)
        p_min = min(p_min, point)
        p_max = max(p_max, point)
    end
    return Box((min=p_min, max=p_max))
end
export boundary

# Convert between boxes of different type.
Base.convert(::Type{Box{N, F1}}, b::Box{N, F2}) where {N, F1, F2} =
    Box{N, F1}(convert(Vec{N, F1}, b.min),
               convert(Vec{N, F1}, b.size))

# Iterate through the coordinates of integer boxes.
@inline Base.iterate(b::BoxT{<:Integer}       ) = iterate(min_inclusive(b) : max_inclusive(b)       )
@inline Base.iterate(b::BoxT{<:Integer}, state)= iterate(min_inclusive(b) : max_inclusive(b), state)
#TODO: Unit tests for Box iteration


###########################
#        Functions        #
###########################

"Gets whether the box has zero/negative volume"
@inline is_empty(b::Box) = any(b.size <= 0)
export is_empty

"
Gets whether the point is fully inside the box, not touching the edges.
This is primarily for integer boxes.
"
is_inside(box::Box{N, F}, point::Vec{N, F}) where {N, F} = all(
    (point > min_inclusive(box)) &
    (point < max_inclusive(box))
)
export is_inside

"
Calculate the intersection of two or more boxes.
If they don't intersect, it will have 0 or negative size (see `is_empty()`).
"
Base.intersect(b::Box) = b
function Base.intersect(b1::Box{N, F1}, b2::Box{N, F2}) where {N, F1, F2}
    F3 = promote_type(F1, F2)
    b_min::Vec{N, F3} = max(min_inclusive(b1), min_inclusive(b2))
    b_max::Vec{N, F3} = min(max_inclusive(b1), max_inclusive(b2))

    # If using unsigned numbers, we can't have a negative size, so keep the size at least 0.
    if F3 <: Unsigned
        b_max = max(b_max, b_min)
    end

    return Box((min=b_min, max=b_max))
end
Base.intersect(b1::Box, b2::Box...) = foldl(intersect, b2, init=b1)

"
Calculate the union of boxes, a.k.a. a bigger box that contains all of them.
Note that, unlike the union of regular sets, a union of boxes
   will contain other elements that weren't in either set.
"
Base.union(b::Box) = b
Base.union(b1::Box{N, F1}, b2::Box{N, F2}) where {N, F1, F2} = Box((
    min=min(min_inclusive(b1), min_inclusive(b2)),
    max=max(max_inclusive(b1), max_inclusive(b2))
))
Base.union(b1::Box, b2::Box...) where {T} = foldl(union, b2, init=b1)

"
Changes the dimensionality of the box.
By default, new dimensions are given the size 1 (both for integer and float boxes).
"
@inline function Base.reshape( b::Box{OldN, T},
                               NewN::Integer;
                               new_dims_size = one(T),
                               new_dims_min = zero(T)
                             ) where {OldN, T}
    if (NewN > OldN)
        return Box((min=vappend(min_inclusive(b), Vec{Int(NewN - OldN), T}(i -> convert(T, new_dims_min))),
                    size=vappend(size(b), Vec{Int(NewN - OldN), T}(i -> convert(T, new_dims_size)))))
    else
        return Box((min=min_inclusive(b)[1:NewN], size=size(b)[1:NewN]))
    end
end

###########################
#        Intervals        #
###########################

"Like a 1D box, but using scalars instead of 1D vectors"
struct Interval{F}
    box::Box{1, F}
end

StructTypes.StructType(::Type{<:Interval}) = StructTypes.CustomStruct()
StructTypes.lower(i::Interval) = Dict("min"=>min_inclusive(i.box).x, "size"=>size(i.box).x)
StructTypes.lowertype(::Type{<:Interval}) = Dict{AbstractString, Float64}
function StructTypes.construct(::Type{Interval{F}}, d::Dict{AbstractString, Float64}) where {F}
    pairs = collect(d)
    keys = map(kvp -> kvp[1], pairs)
    values = map(kvp -> Vec(kvp[2]), pairs)
    return Interval(Box{1, F}(namedtuple(keys, values)))
end

const IntervalI = Interval{Int32}
const IntervalU = Interval{UInt32}
const IntervalF = Interval{Float32}
const IntervalD = Interval{Float64}

export Interval, IntervalI, IntervalU, IntervalF, IntervalD

# Re-implement box stuff for interval.
# Where a Box function would return a Vec1, return a scalar instead.
Interval(inputs::NamedTuple) = Interval(Box(inputs))
Interval{F}(inputs::NamedTuple) where {F} = Interval{F}(Box{1, F}(inputs))
Interval(r::VecRange) = Interval(Box(r))
Interval(r::UnitRange) = Interval(Box(r))
Base.convert(::Type{Box{1, F1}}, i::Interval{F2}) where {F1, F2} = Box((min=Vec{1, F1}(min_inclusive(i)),
                                                                        max=Vec{1, F1}(max_inclusive(i))))
Base.convert(::Type{Interval{F1}}, b::Box{1, F2}) where {F1, F2} = Interval{F1}(convert(Box{1, F1}, b))
# Functions that should pass through the return value unchanged:
for name in [ :volume, :bounds, :boundary, :is_empty, :is_inside ]
    @eval @inline $name(i::Interval, rest...) = $name(i.box, rest...)
end
# Functions that should turn their Vec1 return value into a scalar:
for name in [ :min_inclusive, :max_inclusive, :min_exclusive, :max_exclusive,
              :(Base.size), :center
            ]
    @eval @inline $name(i::Interval, rest...) = $name(i.box, rest...).x
end
@inline closest_point(i::Interval, p) = closest_point(i.box, Vec(p)).x
@inline is_touching(i::Interval, f) = is_touching(i.box, Vec(f))
@inline function Base.iterate(i::Interval, rest...)
    result = iterate(i.box, rest...)
    if exists(result)
        @set! result[1] = result[1].x
    end
    return result
end
@inline Base.intersect(i::Interval, rest::Interval...) = Interval(intersect(i.box, (i2.box for i2 in rest)...))
@inline Base.union(i::Interval, rest::Interval...) = Interval(union(i.box, (i2.box for i2 in rest)...))


##########################################
#         Implementation Details         #
##########################################

"A string representation of a box's type"
box_typestr(::Type{Box{N, F}}) where {N, F} = string("Box", N, "D", type_str(F))
box_typestr(::Type{Interval{F}}) where {F} = string("Interval", type_str(F))


# Helpers that generate the "epsilon" between the inclusive and exclusive max.

box_typeprev(n::I) where {I<:Integer} = n - one(I)
box_typeprev(v::Vec{N, I}) where {N, I<:Integer} = v - one(I)
box_typeprev(f::AbstractFloat) = prevfloat(f)
box_typeprev(v::Vec{N, F}) where {N, F<:AbstractFloat} = map(prevfloat, v)

box_typenext(n::I) where {I<:Integer} = n + one(I)
box_typenext(v::Vec{N, I}) where {N, I<:Integer} = v + one(I)
box_typenext(f::AbstractFloat) = nextfloat(f)
box_typenext(v::Vec{N, F}) where {N, F<:AbstractFloat} = map(nextfloat, v)