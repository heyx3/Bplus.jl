# NOTE: Not a real module; nested within Math.

"
A geometric shape that implements the following functions:

* `volume(s)::F`
* `bounds(s)::Box{N, F}`
* `center(s)::Vec{N, F}`
* `is_touching(s, p::Vec{N, F})::Bool`
* `closest_point(s, p::Vec{N, F})::Vec{N, F}`
* `intersections(s, r::Ray{N, F}; ...)::UpTo{M, F}`
"
abstract type AbstractShape{N, F} end
export AbstractShape

"Calculates the N-dimensional volume of the shape."
volume(s::AbstractShape) = error("Unimplemented: ", typeof(s))

"Calculates a bounding Box for this shape"
bounds(s::AbstractShape) = error("Unimplemented: ", typeof(s))

"Calculates the center-of-mass for this shape"
center(s::AbstractShape) = error("Unimplemented: ", typeof(s))

"Gets the number of dimensions of a shape (the `N` type parameter)"
@inline shape_dimensions(s::AbstractShape) = shape_dimensions(typeof(s))
@inline shape_dimensions(::Type{<:AbstractShape{N}}) where {N} = N

"Gets the number type of a shape (the `F` type parameter)"
@inline shape_ntype(s::AbstractShape) = shape_ntype(typeof(s))
@inline shape_ntype(::Type{<:AbstractShape{N, F}}) where {N, F} = F

is_touching(s::AbstractShape, p)::Bool = error("Unimplemented: ", typeof(s))
@inline is_touching(s::AbstractShape, f::Real) = is_touching(s, Vec(f))

"Finds the closest point on/in a shape to the given point"
closest_point(s::AbstractShape{N, F}, p::Vec{N, F}) where {N, F} = error("Not implemented: ", typeof(s))
closest_point(s::AbstractShape{N, F}, p::Vec{N, F2}) where {N, F, F2} = closest_point(s, convert(Vec{N, F}, p))

"
Finds the intersection(s) of a ray passing through a shape.
Intersections are represented as distances along the ray.
They are returned in ascending order (first intersection is the closest one).

Optional parameters:

* `min_t`: the beginning of the ray. Defaults to 0.
* `max_t`: the end of the ray. Defaults to the largest finite value.
* `atol`: margin of error for various calculations.

For 3D shapes, you may pass an optional type param
    to add the first intersection's surface normal to the return value.
The normal is undefined if there are no intersections.
"
function intersections( s::AbstractShape{N, F}, r::Ray{N, F},
                        ::Val{ShouldCalcNormal} = Val(false)
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)
                      )::Union{UpTo{N, F}, Tuple{UpTo{N, F}, Vec3{F}}} where {N, F, ShouldCalcNormal}
    error("Raycasting not implemented for ", typeof(s))
end

"Checks whether two shapes collide"
collides(a::AbstractShape, b::AbstractShape)::Bool = error("Not implemented for ", typeof(a),
                                                             " and ", typeof(b))

export AbstractShape, shape_dimensions, shape_ntype,
       volume, bounds, center,
       is_touching, closest_point, intersections, collides
#


# Helper function for ray intersection tests
"Given two ray hit points, filters and returns them in ascending order"
function sanitize_hits( h1::F, h2::F,
                        min_t::F, max_t::F,
                        atol::F
                      )::UpTo{2, F} where {F}
    # If the intersections are the same, just return one of them.
    if isapprox(h1, h2; atol=atol)
        if is_touching(Interval{F}(min=min_t, max=max_t), h1)
            return (h1, )
        else
            return ()
        end
    end

    # Sort them.
    ts::Tuple{F, F} = (h1 < h2) ? (h1, h2) : (h2, h1)

    # Pick the correct set of values to return.
    # I had to write all the cases down to make sure I covered them :P
    if (ts[1] > max_t) | (ts[2] < min_t)  # Both are outside the range
        return ()
    elseif ts[2] > max_t  # The larger one is greater than the max
        return (ts[1] < min_t) ? () : (ts[1],)
    else # The second one is definitely in-range.
        return (ts[1] < min_t) ? (ts[2], ) : ts
    end
end


"Returns whether the ray hits the given shape."
intersects(r::Ray, shape; kw...) = !isempty(intersections(r, shape; kw...))
intersects(shape, r::Ray; kw...) = intersects(r, shape; kw...)
#TODO: Optimized overloads of "intersects()" that do less math.

include("box.jl")
include("sphere.jl")
include("capsule.jl")
include("plane.jl")
include("triangle.jl")

include("collision.jl")