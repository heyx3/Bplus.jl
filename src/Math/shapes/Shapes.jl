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
"
function intersections( s::AbstractShape{N, F}, r::Ray{N, F}
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)
                      )::UpTo where {N, F}
    error("Raycasting not implemented for ", typeof(s))
end

"Checks whether two shapes collide"
collides(a::AbstractShape, b::AbstractShape)::Bool = error("Not implemented for ", typeof(a),
                                                             " and ", typeof(b))

export AbstractShape, shape_dimensions, shape_ntype,
       volume, bounds, center,
       is_touching, closest_point, intersections, collides



# Helper function for ray intersection tests
"Given two ray hit points, filters and returns them in ascending order"
function sanitize_hits( h1::F, h2::F,
                        min_t::F, max_t::F,
                        atol::F
                      )::UpTo{2, F} where {F}
    # If the intersections are the same, just return one of them.
    if isapprox(h1, h2; atol=atol)
        return (h1 < min_t) ? () : (h1, )
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


function intersections( b::Box{N, F},
                        r::Ray{N, F}
                        ;
                        inv_ray_dir::Vec{N, F} = one(F) / r.dir, # Pass pre-calculated value if you have it
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)  # Epsilon value, used to check if
                                           #    two intersections are approximately duplicates
                      )::UpTo{2, F} where {N, F}
    # Source: https://github.com/heyx3/heyx3RT/blob/master/RT/RT/Impl/BoundingBox.cpp

    # Get the points of intersection along each of the 6 cube faces.
    min_face_ts::Vec{N, F} = (min_inclusive(b) - r.start) * inv_ray_dir
    max_face_ts::Vec{N, F} = (max_exclusive(b) - r.start) * inv_ray_dir

    # The ray initially extends from 'min_t' to 'max_t'.
    # Trim the ends of this ray to fit within the bounds of each pair of min/max cube faces.
    min_face_ts, max_face_ts = minmax(min_face_ts, max_face_ts)
    ts::NTuple{2, F} = (max(min_face_ts), min(max_face_ts))

    if ts[1] > ts[2]
        return ()
    else
        return sanitize_hits(ts..., min_t, max_t, atol)
    end
end
function intersections( p::Plane{N, F},
                        r::Ray{N, F}
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)  # Epsilon value, used to check if
                                           #    two intersections are approximately duplicates
                      )::Optional{F} where {N, F}
    # Reference: https://en.wikipedia.org/wiki/Line-plane_intersection

    determinant::F = vdot(r.dir, p.normal)
    if abs(determinant) <= atol
        return nothing
    end

    t::F = vdot(p.normal, p.origin - r.start) / determinant
    return is_touching(Box((min=min_t, max=max_t)), t) ? t : nothing
end
function intersections( t::Triangle{N, F},
                        r::Ray{N, F}
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)  # Epsilon value, used to check if
                                           #    two intersections are approximately duplicates
                      )::Optional{F} where {N, F}
    out_intersection::F = zero(F)
    if N == 3
        # Reference: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

        edge1::Vec{N, F} = t.b - t.a
        edge2::Vec{N, F} = t.c - t.a
        h::Vec{N, F} = vcross(r.dir, edge2)

        a::F = vdot(edge1, h)
        if abs(a) <= atol
            return nothing
        end

        f::F = one(F) / a
        s::Vec{N, F} = r.start - t.a
        u::F = f * vdot(s, h)
        if !is_touching(Box((min=zero(F), max=one(F))), u)
            return nothing
        end

        q::Vec{N, F} = vcross(s, edge1)
        v::F = f * vdot(r.dir, q)
        if (v < zero(F)) || ((u + v) > one(F))
            return nothing
        end

        out_intersection = f * vdot(edge2, q)
    else
        #TODO: Triangle-ray intersection for non-3D triangles
        error("Triangle-ray intersection is only implemented in 3D")
    end

    return is_touching(Box((min=min_t, max=max_t)), out_intersection) ?
               out_intersection :
               nothing
end
function intersections( c::Capsule{N, F},
                        r::Ray{N, F}
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)  # Epsilon value, used to check if
                                           #    two intersections are approximately duplicates
                      )::UpTo{2, F} where {N, F}
    # Check intersections with the sphere caps, and the inner cylinder.
    # Reference: https://iquilezles.org/articles/intersectors/

    ts_sphere1 = intersections(Sphere{N, F}(c.a, c.radius), r;
                               min_t=min_t, max_t=max_t, atol=atol)
    ts_sphere2 = intersections(Sphere{N, F}(c.b, c.radius), r;
                               min_t=min_t, max_t=max_t, atol=atol)
    ts_body = intersections(Cylinder{N, F}(c.a, c.b, c.radius), r;
                            min_t=min_t, max_t=max_t, atol=atol)

    # Find the first and last intersection.
    ts_full::UpTo{6, F} = append(ts_sphere1, append(ts_sphere2, ts_body))
    if isempty(ts_full)
        return ()
    end
    t_min::F = ts_full[1]
    t_max::F = ts_full[1]
    for i::Int in 2:length(ts_full)
        t_min = min(t_min, ts_full[i])
        t_max = max(t_max, ts_full[i])
    end
    ts::NTuple{2, F} = (t_min, t_max)

    return sanitize_hits(ts..., min_t, max_t, atol)
end


"A special shape just for ray intersection tests"
struct Cylinder{N, F}
    a::Vec{N, F}
    b::Vec{N, F}
    radius::Vec{N, F}
end
# Not exported because it can't do other shape stuff (collisions)
# Intersection function is inlined because it shares a lot of math with capsule, who invokes it
@inline function intersections( c::Cylinder{N, F},
                                r::Ray{N, F}
                                ;
                                min_t::F = zero(F),
                                max_t::F = typemax_finite(F),
                                atol::F = zero(F)  # Epsilon value, used to check if
                                                   #    two intersections are approximately duplicates
                            )::UpTo{2, F} where {N, F}
    ba::Vec{N, F} = c.b - c.a
    oc::Vec{N, F} = r.start - c.a

    baba::F = vdot(ba, ba)
    bard::F = vdot(ba, r.dir)
    baoc::F = vdot(ba, oc)

    k2::F = baba - (bard * bard)
    k1::F = (baba * vdot(oc, r.dir)) - (baoc * bard)
    k0::F = (baba * vdot(oc, oc)) - (baoc * baoc) - (c.radius * c.radius * baba)

    h::F = (k1 * k1) - (k2 * k0)
    if h < zero(F)
        return ()
    end
    h = sqrt(h)

    t::F = -(k1 + h) / k2

    # Body:
    y::F = baoc + (t * bard)
    if is_inside(Box((min=zero(F), max=baba)), y)
        #TODO: I think t2 = (h - k1) / k2
        return (t, )
    end

    # Caps:
    t = ( ((y < zero(F)) ? zero(F) : baba) - baoc) / bard
    if abs(k1 + (k2 * t)) < h
        #TODO: I think t2 = (... + baoc) / bard
        return (t, )
    end
end