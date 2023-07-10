struct Triangle{N, F} <: AbstractShape{N, F}
    a::Vec{N, F}
    b::Vec{N, F}
    c::Vec{N, F}
    # property 'points::NTuple{3, Vec{N, F}}'
end
@inline Triangle(a::Vec{N, T1}, b::Vec{N, T2}, c::Vec{N, T3}) where {N, T1, T2, T3} =
    Triangle{N, promote_type(T1, promote_type(T2, T3))}(a, b, c)

export Triangle

@inline Base.getproperty(t::Triangle, n::Symbol) = getproperty(t, Val(n))
@inline Base.getproperty(t::Triangle, ::Val{F}) where {F} = getfield(t, F)
@inline Base.getproperty(t::Triangle, ::Val{:points}) = (t.a, t.b, t.c)
#TODO: Make this getproperty design pattern into a macro


function intersections( t::Triangle{N, F},
                        r::Ray{N, F},
                        ::Val{ShouldCalcNormal} = Val(false)
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)  # Epsilon value, used to check if
                                           #    two intersections are approximately duplicates
                      )::Union{UpTo{1, F}, Tuple{UpTo{1, F}, Vec3{F}}} where {N, F, ShouldCalcNormal}
    out_intersection::F = zero(F)
    out_normal::Vec3{F} = zero(Vec3{F})
    null_intersection = ShouldCalcNormal ?
                          (UpTo{1, F}(()), zero(Vec3{F})) :
                          UpTo{1, F}(())
    if N == 3
        # Reference: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm

        edge1::Vec{N, F} = t.b - t.a
        edge2::Vec{N, F} = t.c - t.a
        h::Vec{N, F} = r.dir × edge2

        a::F = vdot(edge1, h)
        if abs(a) <= atol
            return null_intersection
        end

        f::F = one(F) / a
        s::Vec{N, F} = r.start - t.a
        u::F = f * vdot(s, h)
        if !is_touching(Interval{F}((min=0, max=1)), u)
            return null_intersection
        end

        q::Vec{N, F} = s × edge1
        v::F = f * (r.dir ⋅ q)
        if (v < zero(F)) || ((u + v) > one(F))
            return null_intersection
        end

        out_intersection = f * vdot(edge2, q)
        if ShouldCalcNormal
            out_normal = vnorm(edge1 × edge2)
        end
    else
        @bp_check(!ShouldCalcNormal, "Can't calculate normal for a non-3D shape")

        #TODO: Triangle-ray intersection for non-3D triangles
        error("Triangle-ray intersection is only implemented in 3D")
    end

    # Check that the hit is within requested bounds.
    if !is_touching(Interval{F}((min=min_t, max=max_t)), out_intersection)
        return null_intersection
    end

    # Also return the normal if requested.
    hit_result = UpTo{1, F}(out_intersection)
    return ShouldCalcNormal ? (hit_result, out_normal) : hit_result
end

#TODO: rest of AbstractShape interface
