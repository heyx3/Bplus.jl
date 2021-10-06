"""
Quaternions are great for representing 3D rotations.
It is assumed to always be normalized, to optimize the math
  when reversing Quaternions and rotating vectors.
"""
struct Quaternion{F}
    data::Vec4{F}
    
    Quaternion(x::F, y::F, z::F, w::F) where {F} = new{F}(Vec(x, y, z, w))
    Quaternion(components::Vec4{F}) where {F} = new{F}(components)

    "Creates an identity quaternion (i.e. no rotation)."
    Quaternion{F}() where {F} = new{F}(zero(F), zero(F), zero(F), one(F))

    """
    Creates a rotation around the given axis, by the given angle in radians.
    The axis should be normalized.
    """
    function Quaternion(axis::Vec3{F}, angle_radians::F) where {F}
        (sine::F, cos::F) = sincos(angle_radians * convert(F, 0.5))
        return new{F}((axis * sine)..., cos)
    end
    
    """
    Creates a rotation that transforms the first vector into the second.
    The vectors should be normalized.
    """
    Quaternion(from::Vec3{F}, to::Vec3{F}) where {F} = Quaternion{F}(from, to)
    function Quaternion{F}(from::Vec3{F2}, to::Vec3{F2}) where {F, F2}
        @bp_assert(is_normalized(from, convert(F2, 0.001)), "'from' vector isn't normalized")
        @bp_assert(is_normalized(to, convert(F2, 0.001)), "'to' vector isn't normalized")
        cos_angle::F2 = vdot(from, to)

        if isapprox(cos_angle, one(F2); atol=F2(0.001))
            return Quaternion{F}()
        elseif isapprox(cos_angle, -one(F2); atol=F2(0.001))
            # Get an arbitrary perpendicular axis to rotate around.
            axis::Vec3{F2} = (abs(from.x) < 1) ?
                                Vec3{F2}(1, 0, 0) :
                                Vec3{F2}(0, 1, 0)
            axis = vnorm(axis × from)
            return new{F}(map(F, axis), convert(F, π))
        else
            # The axis is the cross product of the vectors.
            norm::Vec3{F} = map(F, from × to)
            theta::F = convert(F, 1) + convert(F, cos_angle)
            return new{F}(vnorm(Vec(norm, theta)))
        end
    end

    "Creates a rotation by combining a sequence of rotations together."
    Quaternion(rots_in_order::Quaternion...) = foldl(*, rots_in_order)
end
Base.getproperty(q::Quaternion, n::Symbol) = getproperty(q.data, n)
export Quaternion

const fquat = Quaternion{Float32}
const dquat = Quaternion{Float64}
export fquat, dquat

@inline function Base.:(*)(q1::Quaternion{F1}, a2::Quaternion{F2}) where {F1, F2}
    F3::DataType = promote_type(F1, F2)
    xyz::Vec3{F3} = (q1.xyz × q2.xyz) +
                    (q1.w * q2.xyz) +
                    (q2.w * q1.xyz)
    w::F3 = (q1.w * q2.w) -
            (q1.xyz ⋅ q2.xyz)
    return Quaternion(xyz..., w)
end
@inline Base.:(*)(q::Quaternion, v::Vec3{T}) where {T} = (q * Quaternion(v..., zero(T)))

@inline function Base.:(-)(q::Quaternion)
    @bp_assert(is_normalized(q.data, 0.0001), "Quaternion isn't normalized")
    return Quaternion((-q.xyz)..., q.w)
end

"""
Normalizes a quaternion.
You normally don't need to do this, but floating-point error can accumulate over time,
   causing quaternions to "drift" away from length 1.
"""
qnorm(q::Quaternion) = Quaternion(vnorm(q.data))
export qnorm

"Applies a Quaternion rotation to a vector"
q_apply(q::Quaternion, v::Vec3) = (q * v) * -q
export q_apply

"""
Interpolation between two rotations.
The interpolation is nonlinear, following the surface of the unit sphere
   instead of a straight line from rotation 1 to rotation 2,
   but is a bit expensive to compute.
The quaternions' 4 components must be normalized.
"""
@inline function q_slerp(a::Quaternion{F}, b::Quaternion{F}, t::F) where {F}
    @bp_assert(is_normalized(a, convert(F, 0.001)))
    @bp_assert(is_normalized(b, convert(F, 0.001)))

    cos_angle::F = vdot(a.data, b.data)
    angle::F = acos(cos_angle) * t

    output::Quaternion{F} = Quaternion(vnorm(b.data - (one.data * cos_angle)))
    (sin_angle::F, cos_angle) = sincos(angle)
    return Quaternion((a.data * cos_angle) + (output.data * sin_angle))
end
export q_slerp

"""
Interpolation between two quaternions.
It's simple (and fast) linear interpolation,
  which can lead to strange animations when tweening.
Use q_slerp() instead for smoother rotations.
"""
lerp(a::Quaternion, b::Quaternion, t) = vnorm(Quaternion(lerp(a.data, b.data, t)))

"Converts a quaternion to a 3x3 transformation matrix"
function q_mat3x3(q::Quaternion{F}) where {F}
    # Source: https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm

    v::Vec3{F} = q.xyz
    v_sqr::Vec3{F} = map(f->f*f, v)

    ONE::F = one(F)
    TWO::F = convert(F, 2)

    xy2::F = v.x * v.y * TWO
    xz2::F = v.x * v.z * TWO
    yz2::F = v.y * v.z * TWO
    vw2::Vec3{F} = v * (w * TWO)

    return @SMatrix [ (ONE - (TWO * (v_sqr.y + v_sqr.z)))               (xy2 - vw2.z)                       (xz2 + vw2.y)
                                (xy2 + vw2.z)               (ONE - (TWO * (v_sqr.x + v_sqr.z)))             (yz2 - vw2.x)
                                (xz2 - vw2.y)                           (yz2 + vw2.x)             (ONE - (TWO * (v_sqr.x + v_sqr.y)))
                    ]
end
"Converts a quaternion to a 4x4 transformation matrix"
q_mat4x4(q::Quaternion{F}) where {F} = to_mat4x4(q_mat3x3(q))
export q_mat3x3, q_mat4x4


#TODO: Double-check slerp, and implement squad, using this article: https://www.3dgep.com/understanding-quaternions/#SLERP