"""
Quaternions are great for representing 3D rotations.
This Quaternion type is backed by a Vec4, so you can swizzle it.
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

fquat = Quaternion{Float32}
dquat = Quaternion{Float64}
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
@inline function Base.:(*)(q::Quaternion, v::Vec)

@inline function Base.:(-)(q::Quaternion)
    @bp_assert(is_normalized(q.data, 0.0001), "Quaternion isn't normalized")
    return Quaternion((-q.xyz)..., q.w)
end

#TODO: Quat-vector multiplication
#TODO: 'Apply to vector' function
#TODO: The three lerps
#TODO: to_matrix()