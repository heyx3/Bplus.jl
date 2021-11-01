"An N-dimensional ray"
struct Ray{N, F<:AbstractFloat}
    start::Vec{N, F}
    dir::Vec{N, F}
end
export Ray

Base.show(io::IO, r::Ray) = print(io,
    "Ray<", r.start, " towards ", r.dir, ">"
)

const Ray2D{F} = Ray{2, F}
const Ray3D{F} = Ray{3, F}
const Ray4D{F} = Ray{4, F}

const Ray2 = Ray2D{Float32}
const Ray3 = Ray3D{Float32}
const Ray4 = Ray4D{Float32}

export Ray2D, Ray3D, Ray4D,
       Ray2, Ray3, Ray4


@inline ray_at(ray::Ray, t::AbstractFloat) = ray.start + (ray.dir * t)
export ray_at


"Given two ray hit points, filters and returns them in ascending order"
@inline function sanitize_hits( h1::F, h2::F,
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


"
Calculates the intersections of a ray with the sphere.
Returns zero, one, or two hits, as interpolants along the ray
  (which you can convert into positions with `ray_at()`).
The hits will be returned in ascending order.
"
@inline function Base.intersect( s::Sphere{N, F},
                                 r::Ray{N, F}
                                 ;
                                 min_t::F = zero(F),
                                 max_t::F = typemax_finite(F),
                                 atol::F = zero(F)  # Epsilon value, used to check if
                                                    #    two intersections are approximately duplicates
                               )::UpTo{2, F} where {N, F}
    # Source: https://fiftylinesofcode.com/ray-sphere-intersection/

    @bp_math_assert(is_normalized(r.dir), "Ray must be normalized to do ray-sphere intersection: ", r)

    sphere_to_ray::Vec{N, F} = r.start - s.center

    p::F = vdot(r.dir, sphere_to_ray)
    q::F = vlength_sqr(sphere_to_ray) - (s.radius * s.radius)

    discriminant::F = (p * p) - q
    if discriminant < 0
        return ()
    end

    offset::F = sqrt(discriminant)
    ts::NTuple{2, F} = (-(p + offset), -(p - offset))

    return sanitize_hits(ts..., min_t, max_t, atol)
end
Base.intersect(r::Ray, s::Sphere) = intersect(s, r)

"
Calculates the intersections of a ray with the box.
Returns zero, one, or two hits, as interpolants along the ray
  (which you can convert into positions with `ray_at()`).
The hits will be returned in ascending order.
"
@inline function Base.intersect( b::Box{Vec{N, F}},
                                 r::Ray{N, F}
                                 ;
                                 min_t::F = zero(F),
                                 max_t::F = typemax_finite(F),
                                 atol::F = zero(F)  # Epsilon value, used to check if
                                                    #    two intersections are approximately duplicates
                               )::UpTo{2, F} where {N, F}
    # Source: https://github.com/heyx3/heyx3RT/blob/master/RT/RT/Impl/BoundingBox.cpp

    # Get the points of intersection along each of the 6 cube faces.
    inv_dir::Vec{N, F} = one(F) / r.dir
    min_face_ts::Vec{N, F} = (b.min - r.start) * inv_dir
    max_face_ts::Vec{N, F} = (max_exclusive(b) - r.start) * inv_dir

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
Base.intersect(r::Ray, b::Box) = intersect(s, b)


"Returns whether the ray hits the given shape."
intersects(r::Ray, shape; kw...) = !isempty(intersect(r, shape; kw...))
intersects(shape, r::Ray; kw...) = intersects(r, shape; kw...)
#TODO: Optimized overloads of "intersects()" that do less math.