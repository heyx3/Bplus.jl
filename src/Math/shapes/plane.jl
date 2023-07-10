struct Plane{N, F} <: AbstractShape{N, F}
    origin::Vec{N, F}
    normal::Vec{N, F}
end

@inline Plane(origin::Vec{N, T1}, normal::Vec{N, T2}) where {N, T1, T2} =
    Plane{N, promote_type(T1, T2)}(origin, normal)

export Plane


#TODO: rest of AbstractShape interface

function intersections( p::Plane{N, F},
                        r::Ray{N, F},
                        ::Val{ShouldCalcNormal} = Val(false)
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)  # Epsilon value, used to check if
                                           #    two intersections are approximately duplicates
                      )::Union{UpTo{1, F}, Tuple{UpTo{1, F}, Vec3{F}}} where {N, F, ShouldCalcNormal}
    # Reference: https://en.wikipedia.org/wiki/Line-plane_intersection

    NULL_RESULT = ShouldCalcNormal ? (UpTo{1, F}(()), zero(Vec3{F})) : UpTo{1, F}(())

    # If the ray is parallel to the plane, there is no intersection.
    determinant::F = vdot(r.dir, p.normal)
    if abs(determinant) <= atol
        return NULL_RESULT
    end

    t::F = -vdot(p.normal, r.start - p.origin) / determinant
    if !is_touching(Interval{F}((min=min_t, max=max_t)), t)
        return NULL_RESULT
    end

    return ShouldCalcNormal ? (UpTo{1, F}((t, )), p.normal) : UpTo{1, F}((t, ))
end