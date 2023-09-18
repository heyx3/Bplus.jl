"
Picks a random position within a unit radius sphere centered at the origin.

Requires 3 uniform-random values.
"
function rand_in_sphere(u::F, v::F, r::F)::Vec3{F} where {F<:AbstractFloat}
    # Source: https://karthikkaranth.me/blog/generating-random-points-in-a-sphere/

    theta::F = u * (convert(F, π) * 2)
    phi::F = acos((2 * v) - 1)
    rr::F = cbrt(r)

    st::F = sin(theta)
    ct::F = cos(theta)
    sp::F = sin(phi)
    cp::F = cos(phi)

    return Vec3{F}(rr * sp * ct,
                   rr * sp * st,
                   rr * cp)
end
@inline function rand_in_sphere(T::Type = Float32)::Vec3{T}
    return rand_in_sphere(Random.GLOBAL_RNG, T)
end
@inline function rand_in_sphere(rng::Random.AbstractRNG, ::Type{F} = Float32)::Vec3{F} where {F<:AbstractFloat}
    return rand_in_sphere(rand(rng, F), rand(rng, F), rand(rng, F))
end
function rand_in_sphere(rng::ConstPRNG, ::Type{F} = Float32)::Tuple{Vec3{F}, ConstPRNG} where {F<:AbstractFloat}
    (u::F, rng) = rand(rng, F)
    (v::F, rng) = rand(rng, F)
    (r::F, rng) = rand(rng, F)
    return (rand_in_sphere(u, v, r), rng)
end
export rand_in_sphere

#TODO: More random distributions