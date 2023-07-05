println("#TODO: Math/shapes/collision.jl")
#TODO: use this reference for the more arcane collisions: https://wickedengine.net/2020/04/26/capsule-collision-detection/

function collides(a::Box{N, F}, b::Box{N, F})::Bool where {N, F}
    a_min = a.min
    a_max = max_inclusive(a)

    b_min = b.min
    b_max = max_inclusive(b)

    return all(a_min < b_max) && all(b_min < a_max)
end

function collides(b::Box{N, F}, s::Sphere{N, F})::Bool where {N, F}
    # Skip lots of math by checking the sphere's bounding box.
    if !collides(b, Box((center=s.center, size=Vec(i -> s.radius, Val(N)))))
        return false
    end

    return vdist_sqr(s.center, closest_point(b, s.center)) <= (s.radius * s.radius)
end
collides(s::Sphere, b::Box) = collides(b, s)