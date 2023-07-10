"An N-dimensional ray"
struct Ray{N, F<:AbstractFloat}
    start::Vec{N, F}
    dir::Vec{N, F}

    Ray{N, F}(start::Vec{N, F}, dir::Vec{N, F}) where {N, F} = new{N, F}(start, dir)
    Ray{N, F}(start::Vec{N, F2}, dir::Vec{N, F3}) where {N, F, F2, F3} = new{N, F}(convert(Vec{N, F}, start),
                                                                                   convert(Vec{N, F}, dir))
    @inline function Ray(start::Vec{N, F1}, dir::Vec{N, F2}) where {N, F1, F2}
        F = promote_type(F1, F2)
        return new{N, F}(convert(Vec{N, F}, start),
                         convert(Vec{N, F}, dir))
    end
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


"Gets the point along the ray at distance `t`"
ray_at(ray::Ray, t) = ray.start + (ray.dir * t)

"Finds the closest point on a ray to another point"
function closest_point( ray::Ray{N, F}, pos::Vec{N, F}
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F)
                       )::F where {N, F}
    pa::Vec{N, F} = pos - ray.start
    ba::Vec{N, F} = ray.dir

    h::F = vdot(pa, ba) / vdot(ba, ba)
    h = clamp(h, min_t, max_t)

    return vlength(pa - (ba * h))
end

export ray_at, closest_point