"An N-dimensional sphere"
struct Sphere{N, F<:AbstractFloat}
    center::Vec{N, F}
    radius::F
end
export Sphere


const Sphere1D{T} = Sphere{1, T}
const Sphere2D{T} = Sphere{2, T}
const Sphere3D{T} = Sphere{3, T}

const Sphere1 = Sphere1D{Float32}
const Sphere2 = Sphere2D{Float32}
const Sphere3 = Sphere3D{Float32}

export Sphere1D, Sphere2D, Sphere3D,
       Sphere1, Sphere2, Sphere3


"Constructs an empty sphere (size 0)"
Sphere{N, T}() where {N, T} = Sphere{N, T}(zero(Vec{N, T}), zero(T))
Sphere(N, T) = Sphere{N, T}()

#TODO: Sphere_bounding(). A bit more painful when calculating the union of two spheres.
# "Constructs a sphere bounding the given points/smaller spheres"
# Sphere_bounding(point::Vec{N, T}) where {N, T} = Sphere{N, T}(point, nextfloat(zero(T)))
# Sphere_bounding(s::Sphere) = s
# Sphere_bounding(a::Vec{N, T}, b::Vec{N, T2}) where {N, T, T2} = Sphere{N, promote_type(T, T2, Float32)}((a+b)/2, vdist(a, b)/2)
# ...
# export Sphere_bounding

# The below functions are also implemented by Box, and already exported.

"Gets the N-dimensional size of a Sphere"
function volume(s::Sphere{N, T}) where {N, T}
    # https://en.wikipedia.org/wiki/Volume_of_an_n-ball
    lower_volume::T = volume(Sphere(zero(Vec{N-2, T}),
                                    s.radius))
    convert(T, 2 * π) * s.radius * s.radius * lower_volume
end

volume(::Sphere{0, T}) where {T} = zero(T)
volume(s::Sphere1D) = s.radius * 2
volume(s::Sphere2D{T}) where {T} = convert(T, π) * s.radius * s.radius
volume(s::Sphere3D{T}) where {T} = convert(T, π * 4//3) * s.radius * s.radius * s.radius

"Gets whether the sphere has 0/negative volume"
is_empty(s::Sphere) = (s.radius <= 0)

"Gets the closest point on or in a sphere to a point 'p'."
@inline function closest_point(s::Sphere{N, T}, p::Vec{N, T}) where {N, T}
    center_to_point::Vec{N, T} = p - s.center

    # If it's past the surface of the sphere, constrain it.
    dist_sqr::T = vlength_sqr(center_to_point)
    if dist_sqr > (s.radius * s.radius)
        center_to_point = (center_to_point / sqrt(dist_sqr)) * s.radius
    end

    return s.center + center_to_point
end

#TODO: Approximate intersection/unions. https://mathworld.wolfram.com/Sphere-SphereIntersection.html