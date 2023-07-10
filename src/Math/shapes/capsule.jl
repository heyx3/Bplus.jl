struct Capsule{N, F} <: AbstractShape{N, F}
    a::Vec{N, F}
    b::Vec{N, F}
    radius::F
end

@inline Capsule(a::Vec{N, T1}, b::Vec{N, T2}, radius::Vec{N, T3}) where {N, T1, T2, T3} =
    Capsule{N, promote_type(T1, promote_type(T2, T3))}(a, b, radius)

export Capsule

    
#TODO: rest of AbstractShape interface

# Internal constants for capsule-ray hit detection
@bp_enum CapsuleHitTypes null pA pB body

function intersections( c::Capsule{N, F},
                        r::Ray{N, F},
                        ::Val{ShouldCalcNormal} = Val(false)
                        ;
                        min_t::F = zero(F),
                        max_t::F = typemax_finite(F),
                        atol::F = zero(F)  # Epsilon value, used to check if
                                           #    two intersections are approximately duplicates
                      )::Union{UpTo{2, F}, Tuple{UpTo{2, F}, Vec3{F}}} where {N, F, ShouldCalcNormal}
    # Helpful aliases:
    ZERO = zero(F)
    ONE = one(F)
    V = Vec{N, F}
    NULL_RESULT = ShouldCalcNormal ?
                    (UpTo{2, F}(()), zero(Vec3{F})) :
                    UpTo{2, F}(())

    #TODO: Check against the capsule's bounding box first, as an optimization

    if N == 3
        #TODO: Rename variables
        
        # Source: https://github.com/NVIDIAGameWorks/PhysX/blob/4.1/physx/source/geomutils/src/intersection/GuIntersectionRayCapsule.cpp

        # Get capsule's delta from one end to the other.
        kW::V = c.b - c.a
        fW_length::F = vlength(kW)

        # If the capsule is effectively a sphere, do ray-sphere intersection.
        # This is not just an optimization; the rest of this function breaks in this case.
        if isapprox(fW_length, ZERO; atol=atol)
            return intersections(Sphere{N, F}(c.a, c.radius),
                                 r,
                                 Val(ShouldCalcNormal)
                                 ;
                                 min_t = min_t,
                                 max_t = max_t,
                                 atol = atol)
        end

        kW /= fW_length

        # Pick an orthonormal basis where 'forward' is the capsule's A=>B direction.
        local kU::V
        #TODO: Turn this technique into a Vec function: 'vbasis(forward::Vec3)'
        if fW_length > ZERO
            #TODO: Remove the branch by parameterizing the axes involved
            if abs(kW.x) >= abs(kW.y)
                # X or Z is the largest component; swap them.
                inv_length = ONE / vlength(kW.xz)
                kU = V(-kW.z * inv_length,
                       0,
                       kW.x * inv_length)
            else
                # Y or Z is the largest component; swap them.
                inv_length = ONE / vlength(kW.yz)
                kU = V(0,
                       kW.z * inv_length,
                       -kW.y * inv_length)
            end
        end
        kV::V = vnorm(v_rightward(kW, kU))


        # Project the ray direction into the capsule ortho-basis, normalized.
        kD::V = V(kU ⋅ r.dir,
                  kV ⋅ r.dir,
                  kW ⋅ r.dir)
        fD_length::F = vlength(kD)
        fD_inv_length::F = isapprox(fD_length, 0; atol=atol) ?
                               ZERO :
                               (ONE / fD_length)
        kD *= fD_inv_length

        # Get the direction from capsule origin to ray origin,
        #    in the capsule ortho-basis, normalized.
        kDiff::V = r.start - c.a
        kP::V = V(kU ⋅ kDiff,
                  kV ⋅ kDiff,
                  kW ⋅ kDiff)
        c_radius_sqr::F = c.radius * c.radius

        local hits::NTuple{2, F}

        # Is the ray parallel to the capsule (or zero)?
        # In that case it either passes through the caps, or doesn't hit at all.
        if (abs(kD.z) >= (ONE - atol)) || (fD_length < atol)
            axis_dir::F = r.dir ⋅ kW
            determinant::F = c_radius_sqr - (kP.x * kP.x) - (kP.y * kP.y)
            local hit_sides::NTuple{2, Int8} # Which cap is each hit associated with?
            if (axis_dir < ZERO) && (determinant >= ZERO)
                root::F = sqrt(determinant)
                hits = ((kP.z + root) * fD_inv_length,
                        -(fW_length - kP.z + root) * fD_inv_length)
                hit_sides = Int8.((2, 1))
            elseif (axis_dir > ZERO) && (determinant >= ZERO)
                root = sqrt(determinant)
                hits = ((-kP.z + root) * fD_inv_length,
                        (fW_length - kP.z + root) * fD_inv_length)
                hit_sides = Int8.((1, 2))
            else
                return NULL_RESULT
            end

            hits_output = UpTo{2, F}(hits)
            if !ShouldCalcNormal
                return hits_output
            end

            # The normals at the caps are just like the normals of a sphere.
            local closest_hit_normal::V
            closest_hit_pos::V = ray_at(r, hits_output[1])
            if hit_sides[1] == 1 # Capsule point A
                closest_hit_normal = vnorm(closest_hit_pos - c.a)
            else # Capsule point B
                @bp_math_assert(hit_sides[2] == 2)
                closest_hit_normal = vnorm(closest_hit_pos - c.b)
            end
            return (hits_output, closest_hit_normal)
        end

        # We'll need to check 6 possible intersections:
        #    2 for each cap, and 2 along the cylindrical body.
        # Only the earliest and latest hits need to be tracked.
        n_capsule_hits::Int8 = 0
        hits = (typemax(F), typemax(F))
        hit_types::NTuple{2, E_CapsuleHitTypes} = (CapsuleHitTypes.null,
                                                   CapsuleHitTypes.null)
        capsule_local_z_range = Interval{F}((min=0, max=fW_length))
        function calc_normal(t::F, type::E_CapsuleHitTypes)::Vec3{F}
            @bp_math_assert(type != CapsuleHitTypes.null,
                            "Asked to calculate the normal of a 'null' intersection")
            hit_pos::V = ray_at(r, t)
            if type == CapsuleHitTypes.pA
                return vnorm(hit_pos - c.a)
            elseif type == CapsuleHitTypes.pB
                return vnorm(hit_pos - c.b)
            elseif type == CapsuleHitTypes.body
                # Project the hit position onto the cylinder's central line;
                #    that's the closest point on the line to the hit pos.
                # The normal points from the line towards the hit pos.
                hit_projection_to_cylinder::v3f = hit_pos ⋅kW
                return vnorm(hit_pos - hit_projection_to_cylinder)
            else
                error("Unimplemented: ", type)
            end
        end
        function try_add_hit(t::F, type::E_CapsuleHitTypes)::Bool
            if (t >= min_t) && (t <= max_t)
                @set! hits[n_capsule_hits + 1] = t
                @set! hit_types[n_capsule_hits + 1] = type
                n_capsule_hits += 1
                return true
            else
                return false
            end
        end
        function calc_output()
            if hits[2] < hits[1]
                hits = (hits[2], hits[1])
                hit_types = (hit_types[2], hit_types[1])
            end
            hit_output = UpTo{2, F}(hits, n_capsule_hits)
            return ShouldCalcNormal ?
                     (hit_output, calc_normal(hits[1], hit_types[1])) :
                     hit_output
        end

        # Julia's syntax rules encourage us to declare some locals out here :(
        local inv_a::F,
              t::F,
              capsule_local_t::F

        # Cylindrical body:
        a::F = vlength_sqr(kD.xy)
        b::F = kP.xy ⋅ kD.xy
        c::F = vlength_sqr(kP.xy) - c_radius_sqr
        determinant = (b * b) - (a * c)
        if determinant < ZERO
            return NULL_RESULT
        end
        if determinant > atol
            # Line intersects the cylinder in two places,
            #    but we'll still need to check if those places
            #    are on the *capsule*.
            root = sqrt(determinant)
            inv_a = ONE / a
            for sign in (-1, 1)
                t = (-b + (sign * root)) * inv_a
                capsule_local_t = kP.z + (t * kD.z)
                if is_touching(capsule_local_z_range, capsule_local_t)
                    try_add_hit(t * fD_inv_length,
                                if capsule_local_t < center(capsule_local_z_range)
                                    CapsuleHitTypes.pA
                                else
                                    CapsuleHitTypes.pB
                                end)
                end
            end

            # If both intersections are on the capsule, then we're done.
            if n_capsule_hits == 2
                return calc_output()
            end
        else
            # Line is tangent to the cylinder.
            t = -b / a
            capsule_local_t = kP.z + (t * kD.z)
            if is_touching(capsule_local_z_range, capsule_local_t)
                hit::F = t * fD_inv_length
                if ShouldCalcNormal
                    return (UpTo{2, F}(hit),
                            calc_normal(hit, CapsuleHitTypes.body))
                else
                    return UpTo{2, F}(hit)
                end
            end
        end

        # Cap A:
        # a == 1 already
        b += kP.z * kD.z
        c += kP.z * kP.z
        determinant = (b * b) - c
        if determinant > ZERO
            root = sqrt(determinant)
            for sign in (-1, 1)
                t = -(b + (sign * root))
                cylinder_local_t = kP.z + (t * kD.z)
                if cylinder_local_t <= ZERO # Hit the bottom of the "sphere"?
                    added = try_add_hit(t * fD_inv_length, CapsuleHitTypes.pA)
                    if added && (n_capsule_hits == 2)
                        return calc_output()
                    end
                end
            end
        elseif determinant == ZERO
            t = -b
            cylinder_local_t = kP.z + (t * kD.z)
            if cylinder_local_t <= ZERO # Hit the bottom of the "sphere"?
                added = try_add_hit(t * fD_inv_length, CapsuleHitTypes.pA)
                if added && (n_capsule_hits == 2)
                    return calc_output()
                end
            end
        end

        # Cap B:
        # a == 1 already
        b -= kD.z * fW_length
        c += fW_length * (fW_length - (convert(F, 2) * kP.z))
        determinant = (b * b) - c
        if determinant > ZERO
            root = sqrt(determinant)
            for sign in (-1, 1)
                t = -b + (sign * root)
                cylinder_local_t = kP.z + (t * kD.z)
                if cylinder_local_t >= fW_length
                    added = try_add_hit(t * fD_inv_length, CapsuleHitTypes.pB)
                    if added && (n_capsule_hits == 2)
                        return calc_outputs()
                    end
                end
            end
        elseif determinant == ZERO
            t = -b
            cylinder_local_t = kP.z + (t * kD.z)
            if cylinder_local_t >= fW_length
                added = try_add_hit(t * fD_inv_length, CapsuleHitTypes.pB)
                if added && (n_capsule_hits == 2)
                    return calc_outputs()
                end
            end
        end

        # We didn't see a full two intersections.
        # Probably the ray is inside the capsule?
        if n_capsule_hits == 1
            hit_output = UpTo{2, F}(hits[1])
            return ShouldCalcNormal ?
                     (hit_output, calc_normal(hits[1], hit_types[1])) :
                     hit_output
        else
            @bp_math_assert(n_capsule_hits == 0)
            return NULL_RESULT
        end
    else
        error("Only implemented for 3 dimensions")
        #TODO: Implement at least the 2D case, ideally the general N-D case.
    end
end