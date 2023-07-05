"An N-dimensional sphere"
struct Sphere{N, F<:AbstractFloat} <: AbstractShape{N, F}
    center::Vec{N, F}
    radius::F
end
"Constructs an empty sphere (size 0)"
Sphere{N, T}() where {N, T} = Sphere{N, T}(zero(Vec{N, T}), zero(T))
Sphere(N, T) = Sphere{N, T}()

const Sphere1D{T} = Sphere{1, T}
const Sphere2D{T} = Sphere{2, T}
const Sphere3D{T} = Sphere{3, T}

export Sphere, Sphere1D, Sphere2D, Sphere3D


"Gets whether a sphere has 0 (or negative) volume"
is_empty(s::Sphere) = (s.radius <= 0)

export is_empty

#TODO: Sphere_bounding(). A bit more painful when calculating the union of two spheres.
# "Constructs a sphere bounding the given points/smaller spheres"
# Sphere_bounding(point::Vec{N, T}) where {N, T} = Sphere{N, T}(point, nextfloat(zero(T)))
# Sphere_bounding(s::Sphere) = s
# Sphere_bounding(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Sphere{N, promote_type(T, T2, Float32)}((a+b)/2, vdist(a, b)/2)
# ...
# export Sphere_bounding

# Sphere volume has a recursive formula based on number of dimensions.
#    https://en.wikipedia.org/wiki/Volume_of_an_n-ball
volume(::Sphere{0, T}) where {T} = zero(T)
volume(s::Sphere1D) = s.radius * 2
volume(s::Sphere2D{T}) where {T} = convert(T, π) * s.radius * s.radius
volume(s::Sphere3D{T}) where {T} = convert(T, π * 4 / 3) * s.radius * s.radius * s.radius
volume(s::Sphere{N, F}) where {N, F} = let lower_origin = zero(Vec{N-2, F}),
                                           lower_volume = volume(Sphere(lower_origin, s.radius))
    return convert(F, π * 2) * lower_volume * s.radius * s.radius
end

@inline function closest_point(s::Sphere{N, T}, p::Vec{N, T})::Vec{N, T} where {N, T}
    center_to_point::Vec{N, T} = p - s.center

    # If it's past the surface of the sphere, constrain it.
    dist_sqr::T = vlength_sqr(center_to_point)
    if dist_sqr > (s.radius * s.radius)
        center_to_point = (center_to_point / sqrt(dist_sqr)) * s.radius
    end

    return s.center + center_to_point
end

function intersections( s::Sphere{N, F},
                        r::Ray{N, F}
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)
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

#TODO: Approximate intersection/unions. https://mathworld.wolfram.com/Sphere-SphereIntersection.html