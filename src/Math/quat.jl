"""
Quaternions are great for representing 3D rotations.
It is assumed to always be normalized, to optimize the math
  when reversing Quaternions and rotating vectors.
"""
struct Quaternion{F}
    data::Vec4{F}

    # Constructors that take the components and no type param:
    @inline Quaternion(x, y, z, w) = Quaternion(promote(x, y, z, w))
    @inline Quaternion(x::F, y::F, z::F, w::F) where {F} = new{F}(Vec{4, F}(x, y, z, w))
    @inline Quaternion(components::NTuple{4, F}) where {F} = new{F}(Vec4{F}(components...))
    @inline Quaternion(components::Vec4{F}) where {F} = new{F}(components)

    # Constructors that take the components and a type param:
    @inline Quaternion{F}(x, y, z, w) where {F} = new{F}(Vec4{F}(x, y, z, w))
    @inline Quaternion{F}(components::NTuple{4, F2}) where {F, F2} = new{F}(Vec4{F}(components...))
    @inline Quaternion{F}(components::Vec4{F2}) where {F, F2} = new{F}(convert(Vec4{F}, components))

    "Creates an identity quaternion (i.e. no rotation)."
    @inline Quaternion{F}() where {F} = new{F}(Vec4{F}(0, 0, 0, 1))

    """
    Creates a rotation around the given axis, by the given angle in radians.
    The axis should be normalized.
    """
    @inline function Quaternion(axis::Vec3{F}, angle_radians::F2) where {F, F2}
        @bp_math_assert(is_normalized(axis), "Axis of Quaternion rotation isn't normalized: ", axis)
        (sine::F, cos::F) = sincos(convert(F, angle_radians * 0.5))
        return new{F}(Vec4{F}((axis * sine)..., cos))
    end
    @inline Quaternion{F}(axis::Vec3{F2}, angle_radians::F3) where {F, F2, F3} =
        Quaternion(convert(Vec3{F}, axis), convert(F, angle_radians))

    """
    Creates a rotation that transforms the first vector into the second.
    The vectors should be normalized.
    """
    @inline Quaternion(from::Vec3{F}, to::Vec3{F}) where {F} = Quaternion{F}(from, to)
    @inline Quaternion(from::Vec3{F1}, to::Vec3{F2}) where {F1, F2} = Quaternion{promote_type(F, F2)}(from, to)
    function Quaternion{F}(from::Vec3{F2}, to::Vec3{F3}) where {F, F2, F3}
        if !(F2<:Integer)
            @bp_math_assert(is_normalized(from, convert(F2, 0.001)), "'from' vector isn't normalized")
        end
        if !(F3<:Integer)
            @bp_math_assert(is_normalized(to, convert(F3, 0.001)), "'to' vector isn't normalized")
        end
        cos_angle::F = convert(F, vdot(from, to))

        if isapprox(cos_angle, one(F); atol=0.001)
            return Quaternion{F}()
        elseif isapprox(cos_angle, -one(F); atol=0.001)
            # There is no rotation.
            # Get an arbitrary perpendicular axis to rotate around.
            return Quaternion{F}(map(F, pick_arbitrary_rot_axis(from)), convert(F, π))
        else
            # The axis is the cross product of the vectors.
            return new{F}(pick_rot_axis(from, to, cos_angle))
        end
    end

    "Creates a rotation from a rotation matrix"
    function Quaternion(rot_mat::SMatrix{3, 3, F, 9}) where {F}
        # Reference: https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/

        trace = rot_mat[1, 1] + rot_mat[2, 2] + rot_mat[3, 3]
        local qw::F,
              qx::F,
              qy::F,
              qz::F,
              s::F
        @inline f(x) = convert(F, x)
    
        if trace > 0
            s = f(0.5) / sqrt(trace + f(1))
            qw = f(0.25) / s
            qx = (rot_mat[3, 2] - rot_mat[2, 3]) * s
            qy = (rot_mat[1, 3] - rot_mat[3, 1]) * s
            qz = (rot_mat[2, 1] - rot_mat[1, 2]) * s
        elseif (rot_mat[1, 1] > rot_mat[2, 2]) && (rot_mat[1, 1] > rot_mat[3, 3])
            s = f(2) * sqrt(f(1) + rot_mat[1, 1] - rot_mat[2, 2] - rot_mat[3, 3])
            qw = (rot_mat[3, 2] - rot_mat[2, 3]) / s
            qx = f(0.25) * s
            qy = (rot_mat[1, 2] + rot_mat[2, 1]) / s
            qz = (rot_mat[1, 3] + rot_mat[3, 1]) / s
        elseif (rot_mat[2, 2] > rot_mat[3, 3])
            s = f(2) * sqrt(f(1) + rot_mat[2, 2] - rot_mat[1, 1] - rot_mat[3, 3])
            qw = (rot_mat[1, 3] - rot_mat[3, 1]) / s
            qx = (rot_mat[1, 2] + rot_mat[2, 1]) / s
            qy = f(0.25) * s
            qz = (rot_mat[2, 3] + rot_mat[3, 2]) / s
        else
            s = f(2) * sqrt(f(1) + rot_mat[3, 3] - rot_mat[1, 1] - rot_mat[2, 2])
            qw = (rot_mat[2, 1] - rot_mat[1, 2]) / s
            qx = (rot_mat[1, 3] + rot_mat[3, 1]) / s
            qy = (rot_mat[2, 3] + rot_mat[3, 2]) / s
            qz = f(0.25) * s
        end

        # Normalize the quaternion before constructing.
        return new{F}(vnorm(Vec(qx, qy, qz, qw)))
    end
    @inline Quaternion{F}(rot_mat::SMatrix{3, 3, F2, 9}) where {F, F2} =
        Quaternion(convert(similar_type(rot_mat, F), rot_mat))

    "Creates a rotation representing a sequence of other rotations."
    @inline Quaternion(rots_in_order::Quaternion...) = foldl(*, rots_in_order)
    @inline Quaternion{F}(rots_in_order::Quaternion...) where {F} = foldl(*, rots_in_order)
end
export Quaternion

@inline Base.getproperty(q::Quaternion, n::Symbol) = getproperty(getfield(q, :data), n)

@inline Base.convert(::Type{Quaternion{F2}}, q::Quaternion) where {F2} = Quaternion{F2}(q.data)


###############
#   Aliases   #
###############

# See the notes in the "Aliases" section for Vec.

const fquat = Quaternion{Float32}
const dquat = Quaternion{Float64}

export fquat, dquat


################
#   Printing   #
################

# Base.show() prints the quat with a certain number of digits.

QUAT_AXIS_DIGITS = 2
QUAT_ANGLE_DIGITS = 3

function Base.show(io::IO, ::MIME"text/plain", q::Quaternion{F}) where {F}
    qdata::Vec4{F} = getfield(q, :data)
    length::Optional{F} = vlength(qdata)
    is_normalized::Bool = isapprox(length, 1; atol=0.00001)
    (axis::Vec3{F}, angle::F) = q_axisangle(qnorm(q))

    # Use the Vec printing work to simplify.
    n_digits_axis::Int = QUAT_AXIS_DIGITS
    n_digits_angle::Int = QUAT_ANGLE_DIGITS
    use_vec_digits(n_digits_axis) do

        print(io, "<")
        print(io, "axis=", axis, "  ")
        print(io, "θ=", printable_component(angle, n_digits_angle))
        if !is_normalized
            print(io, "  len=", length)
        end
        print(io, ">")
    end
end
function Base.print(io::IO, q::Quaternion{F}) where {F}
    n_digits_axis::Int = QUAT_AXIS_DIGITS
    n_digits_angle::Int = QUAT_ANGLE_DIGITS
    if q isa fquat
        print(io, "fquat")
    elseif q isa dquat
        print(io, "dquat")
    else
        print(io, "Quaternion")
    end

    print(io, '(')
    for i in 1:4
        if i > 1
            print(io, ", ")
        end
        f::F = printable_component(q.data[i],
                                   (i == 4) ?
                                     n_digits_angle :
                                     n_digits_axis)
        print(io, f)
    end
    print(io, ')')
end

"""
Runs the given code with QUAT_ANGLE_DIGITS and QUAT_AXIS_DIGITS
  temporarily changed to the given values.
"""
function use_quat_digits(to_do::Base.Callable, n_axis_digits::Int, n_angle_digits::Int)
    global QUAT_ANGLE_DIGITS, QUAT_AXIS_DIGITS
    old::Tuple{Int, Int} = QUAT_ANGLE_DIGITS, QUAT_AXIS_DIGITS
    QUAT_ANGLE_DIGITS = n_angle_digits
    QUAT_AXIS_DIGITS = n_axis_digits

    try
        to_do()
    catch e
        rethrow(e)
    finally
        QUAT_ANGLE_DIGITS, QUAT_AXIS_DIGITS = old
    end
end
export use_quat_digits

"Pretty-prints a Quaternion using the given number of digits."
function show_quat(io::IO, q::Quaternion, n_axis_digits::Int, n_angle_digits::Int)
    use_quat_digits(n_axis_digits, n_angle_digits) do
        show(io, v)
    end
end
export show_quat


##################
#   Arithmetic   #
##################

function Base.:(*)(q1::Quaternion{F1}, q2::Quaternion{F2}) where {F1, F2}
    F3::DataType = promote_type(F1, F2)

    xyz::Vec3{F3} = v_rightward(q1.xyz, q2.xyz) +
                    (q1.w * q2.xyz) +
                    (q2.w * q1.xyz)
    w::F3 = (q1.w * q2.w) -
            (q1.xyz ⋅ q2.xyz)
    return Quaternion(xyz..., w)
end
function Base.:(*)( q::Quaternion{F},
                    v::Vec3{T}
                  ) where {F, T}
    return (q * Quaternion(promote(v..., zero(F))))
end

function Base.:(-)(q::Quaternion{F}) where {F}
    @bp_math_assert(is_normalized(q), "Quaternion isn't normalized: ", q)
    return Quaternion{F}((-q.xyz)..., q.w)
end

@inline Base.:(==)(q1::Quaternion, q2::Quaternion)::Bool = (q1.data == q2.data)

# See my notes about isapprox() for Vec
Base.isapprox(a::Quaternion{F}, b::Quaternion{F2}) where {F, F2} =
    all(t -> isapprox(t[1], t[2]), zip(a.data, b.data))
Base.isapprox(a::Quaternion{F}, b::Quaternion{F2}, atol) where {F, F2} =
    all(t -> isapprox(t[1], t[2]; atol=atol), zip(a.data, b.data))
#


######################
#   Quaternion ops   #
######################

is_normalized(q::Quaternion{F}, atol::F2 = 0.0) where {F, F2} = is_normalized(getfield(q, :data), atol)
# No export needed for is_normalized(), because it's an overload of the Vec version.

"""
Normalizes a quaternion.
You normally don't need to do this, but floating-point error can accumulate over time,
   causing quaternions to "drift" away from length 1.
"""
qnorm(q::Quaternion) = Quaternion(vnorm(getfield(q, :data)))
export qnorm

"Applies a Quaternion rotation to a vector"
@inline q_apply(q::Quaternion, v::Vec3) = ((q * v) * -q).xyz
export q_apply

"""
Interpolation between two rotations.
The interpolation is nonlinear, following the surface of the unit sphere
   instead of a straight line from rotation 1 to rotation 2,
   but is a bit expensive to compute.
The quaternions' 4 components must be normalized.
"""
@inline function q_slerp(a::Quaternion{F}, b::Quaternion{F}, t::F) where {F}
    @bp_math_assert(is_normalized(a, convert(F, 0.001)))
    @bp_math_assert(is_normalized(b, convert(F, 0.001)))

    cos_angle::F = vdot(a.data, b.data)
    angle::F = acos(cos_angle) * t

    output::Quaternion{F} = Quaternion(vnorm(b.data - (one.data * cos_angle)))
    (sin_angle::F, cos_angle) = sincos(angle)
    return Quaternion((a.data * cos_angle) + (output.data * sin_angle))
end
export q_slerp

"Gets whether a quaternion represents 'no rotation'."
@inline q_is_identity(q::Quaternion{F}) where {F} = (q == Quaternion{F}())
export q_is_identity

"""
Interpolation between two quaternions.
It's simple (and fast) linear interpolation,
  which can lead to strange animations when tweening.
Use q_slerp() instead for smoother rotations.
"""
@inline lerp(a::Quaternion, b::Quaternion, t) = vnorm(Quaternion(lerp(a.data, b.data, t)))
# No export needed for lerp(), because it's an overload of an existing function.


###################
#   Conversions   #
###################

"Gets the quaternion's rotation as an axis and an angle (in radians)"
function q_axisangle(q::Quaternion{F})::Tuple{Vec3{F}, F} where {F}
    @bp_math_assert(q.w <= 1.0, "Quaternion isn't normalized, which breaks q_axisangle(): ", q)

    # For the 'identity' Quaternion, the angle is 0 and the axis is arbitrary.
    if q.w >= 1
        return (get_up_vector(F), zero(F))
    end
    # In all other cases (including an angle of 180), the math works out fine.
    return (q.xyz / convert(F, sqrt(1 - (q.w * q.w))),
            convert(F, 2 * acos(q.w)))
end
export q_axisangle

"Converts a quaternion to a 3x3 transformation matrix"
function q_mat3x3(q::Quaternion{F})::Mat(3, 3, F) where {F}
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
export q_mat3x3

"Converts a quaternion to a 4x4 transformation matrix"
@inline q_mat4x4(q::Quaternion) = m_to_mat4x4(q_mat3x3(q))
export q_mat4x4


#################
##   Snippets  ##
#################

# Breaking out some code into small functions to help with type inference.

function pick_arbitrary_rot_axis(v::Vec3{F}) where {F}
    axis::Vec3{F} = (abs(v.x) < one(F)) ?
                       Vec3{F}(1, 0, 0) :
                       Vec3{F}(0, 1, 0)
    axis = vnorm(v_rightward(axis, from))
end

function pick_rot_axis(from::Vec3{F1}, to::Vec3{F2}, cos_angle::F) where {F, F1, F2}
    norm::Vec3{F} = map(F, v_rightward(from, to))
    theta::F = one(F) + convert(F, cos_angle)
    return vnorm(Vec4{F}(norm..., theta))
end

###################
