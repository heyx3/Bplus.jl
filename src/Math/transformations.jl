# Matrix transformation utilities.

# Keep in mind that the matrix elements are grouped by column,
#    which looks confusing because they're written out in rows.
# E.x. the matrix Mat{2, 3, Float32}(1, 2, 3,
#                                    4, 5, 6)
#    has a first column of "1, 2, 3" and a second column of "4, 5, 6".

# Also keep in mind that matrices are generally expected to be premultiplied --
#    "mat * vec" rather than "vec * mat".

"Generates a 2D translation matrix"
@inline m3_translate(amount::Vec{2, F}) where {F<:AbstractFloat} = Mat{3, 3, F}(
    1, 0, 0,
    0, 1, 0,
    amount..., 1
)
"Generates a 3D translation matrix"
@inline m4_translate(amount::Vec{3, F}) where {F<:AbstractFloat} = Mat{4, 4, F}(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    amount..., 1
)

"Generates a scale matrix"
m_scale(amount::Vec{N, F}) where {N, F} = let output = zero(MMatrix{N, N, F})
    for i in 1:N
        output[i, i] = amount[i]
    end
    return SMatrix(output)
end

"Generates a 3x3 rotation matrix around the X axis, in radians"
@inline m3_rotateX(rad::F) where {F<:AbstractFloat} = let sico = sincos(rad)
    @SMatrix [
        1    0        0
        0  sico[2]  -sico[1]
        0  sico[1]  sico[2]
    ]
end
"Generates a 4x4 rotation matrix around the X axis"
@inline m4_rotateX(rad::F) where {F<:AbstractFloat} = m_to_mat4x4(m3_rotateX(rad))

"Generates a 3x3 rotation matrix around the Y axis, in radians"
@inline m3_rotateY(rad::F) where {F<:AbstractFloat} = let sico = sincos(rad)
    @SMatrix [
        sico[2]   0   sico[1]
          0       1     0
       -sico[1]   0   sico[2]
    ]
end
"Generates a 4x4 rotation matrix around the Y axis"
@inline m4_rotateY(rad::F) where {F<:AbstractFloat} = m_to_mat4x4(m3_rotateY(rad))

"Generates a 3x3 rotation matrix around the Z axis, in radians"
@inline m3_rotateZ(rad::F) where {F<:AbstractFloat} = let sico = sincos(rad)
    @SMatrix [
        sico[2]  -sico[1]   0
        sico[1]   sico[2]   0
          0         0       1
    ]
end
"Generates a 4x4 rotation matrix around the Z axis"
@inline m4_rotateZ(rad::F) where {F<:AbstractFloat} = m_to_mat4x4(m3_rotateZ(rad))

"Generates a 3x3 rotation matrix from a quaternion"
function m3_rotate(q::Quaternion{F}) where {F}
    # Source: https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
    # There is a second equation for this out on the internet,
    #    but it assumes the Quaternion is normalized so I went with this one.

    v::Vec3{F} = q.xyz
    v_sqr::Vec3{F} = map(f->f*f, v)

    ONE::F = one(F)
    TWO::F = convert(F, 2)

    xy2::F = v.x * v.y * TWO
    xz2::F = v.x * v.z * TWO
    yz2::F = v.y * v.z * TWO
    vw2::Vec3{F} = v * (w * TWO) 

    return @SMatrix [
        (ONE - (TWO * (v_sqr.y + v_sqr.z)))               (xy2 - vw2.z)                       (xz2 + vw2.y)
                  (xy2 + vw2.z)               (ONE - (TWO * (v_sqr.x + v_sqr.z)))             (yz2 - vw2.x)
                  (xz2 - vw2.y)                           (yz2 + vw2.x)             (ONE - (TWO * (v_sqr.x + v_sqr.y)))
    ]
end
"Generates a 4x4 rotation matrix from a quaternion"
@inline m4_rotate(q::Quaternion{F}) where {F} = m_to_mat4x4(m3_rotate(q))

"Builds the world-space matrix for an object with the given position, rotation, and scale."
@inline function m4_world( pos::Vec3{F},
                           rot::Quaternion{F},
                           scale::Vec3{F}
                         )::Mat{4, 4, F} where {F}
    # Reference: https://old.reddit.com/r/Unity3D/comments/flwreg/how_do_i_make_a_trs_matrix_manually/fl1m45x/

    rotv = Vec4{F}(rot.data...)
    rot_mul_sqr::Vec3{F} = rotv.xyz * rotv.xyz
    rot_mul_w::Vec3{F} = rotv.xyz * rotv.w

    ONE::F = one(F)
    TWO::F = convert(F, 2)

    return Mat{4, 4, F}(
        (ONE - (TWO * (rot_mul_sqr.y + rot_mul_sqr.z))) * scale.x,
          ((rotv.x * rotv.y) + rot_mul_w.z) * scale.x * TWO,
          ((rotv.x * rotv.z) - rot_mul_w.y) * scale.x * TWO,
          zero(F),

        ((rotv.x * rotv.y) - rot_mul_w.z) * scale.y * TWO,
          (ONE - (TWO * (rot_mul_sqr.x + rot_mul_sqr.z))) * scale.y,
          ((rotv.y * rotv.z) + rot_mul_w.x) * scale.y * TWO,
          zero(F),

        ((rotv.x * rotv.z) + rot_mul_w.y) * scale.z * TWO,
          ((rotv.y * rotv.z) - rot_mul_w.x) * scale.z * TWO,
          (ONE - (TWO * (rot_mul_sqr.x + rot_mul_sqr.y))) * scale.z,
          zero(F),

        pos..., ONE
    )
end

"Builds the view matrix for a camera looking at the given position."
@inline function m4_look_at( cam_pos::Vec3{F},
                             target_pos::Vec3{F},
                             up::Vec3{F}
                             ;
                             forward_axis::Int = get_horz_axes()[1],
                             forward_dir::Int = 1,
                             upward_axis::Int = get_up_axis(),
                             upward_dir::Int = get_up_sign(),
                             rightward_axis::Int = get_horz_axes()[2],
                             rightward_dir::Int = 1
                           )::Mat{4, 4, F} where {F}
    # Reference: https://www.geertarien.com/blog/2017/07/30/breakdown-of-the-lookAt-function-in-OpenGL/

    look_dir::Vec3{F} = vnorm(target_pos - cam_pos)
    basis = vbasis(look_dir, up)

    @inline copy_sign(v::Vec, dir::Int) = (dir < 0) ? -v : v
    row_rightward = copy_sign(vappend(basis.right,
                                      -vdot(basis.right, cam_pos)),
                              rightward_dir)
    row_upward = copy_sign(vappend(basis.up,
                                   -vdot(basis.up, cam_pos)),
                           upward_dir)
    row_forward = copy_sign(vappend(basis.forward,
                                    -vdot(basis.forward, cam_pos)),
                            forward_dir)

    get_row(axis::Int)::Vec{4, F} =
        if forward_axis == axis
            row_forward
        elseif upward_axis == axis
            row_upward
        elseif rightward_axis == axis
            row_rightward
        else
            error("Invalid axis setup: ", forward_axis, "|", upward_axis, "|", rightward_axis)
        end
    return m_transpose(Mat{4, 4, F}( # Notice the call to transpose --
                                     #    these are the rows, not the columns.
        get_row(1)...,
        get_row(2)...,
        get_row(3)...,
        zero(F), zero(F), zero(F), one(F)
    ))
end
@inline function m3_look_at(forward::Vec3{F},
                            up::Vec3{F},
                            right::Vec3{F} = v_rightward(forward, up)
                            ;
                            forward_axis::Int = get_horz_axes()[1],
                            forward_dir::Int = 1,
                            upward_axis::Int = get_up_axis(),
                            upward_dir::Int = get_up_sign(),
                            rightward_axis::Int = get_horz_axes()[2],
                            rightward_dir::Int = 1
                           )::Mat{3, 3, F} where {F}
    @inline copy_sign(v::Vec3{F}, dir::Int) = (dir < 0) ? -v : v
    row_rightward = copy_sign(right, rightward_dir)
    row_upward = copy_sign(up, upward_dir)
    row_forward = copy_sign(forward, forward_dir)

    get_row(axis::Int) = if forward_axis == axis
                             row_forward
                         elseif upward_axis == axis
                             row_upward
                         elseif rightward_axis == axis
                             row_rightward
                         else
                             error("Invalid axis setup: ", forward_axis, "|", upward_axis, "|", rightward_axis)
                         end
    return m_transpose(Mat{3, 3, F}( # Notice the call to transpose --
                                     #    these are the rows, not the columns.
        get_row(1)...,
        get_row(2)...,
        get_row(3)...,
    ))
end

"Generates the standard OpenGL perspective matrix"
@inline function m4_projection( near_clip::F, far_clip::F,
                                aspect_width_over_height::F,
                                field_of_view_degrees::F
                              ) where {F}
    tan_fov::F = tan(deg2rad(field_of_view_degrees / convert(F, 2)))
    scale_along_clip_planes::F = 1 / (far_clip - near_clip)
    return Mat{4, 4, F}(
        1/(tan_fov * aspect_width_over_height), 0, 0, 0,
        0, 1/tan_fov, 0, 0,
        0, 0, -(far_clip + near_clip) * scale_along_clip_planes, -one(F),
        0, 0, convert(F, -2) * far_clip * near_clip * scale_along_clip_planes, 0
    )
end

"Generates an orthographic projection matrix"
@inline function m4_ortho(min::Vec3{F}, max::Vec3{F})::@Mat(4, 4, F) where {F}
    # A good reference for the derivation of this:
    #    https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/orthographic-projection-matrix

    ZERO = zero(F)
    ONE = one(F)
    TWO = convert(F, 2)

    range = max - min
    sum = min + max
    denom = ONE / range
    offset = (-sum * denom)
    @set! offset.z = -offset.z

    return Mat{4, 4, F}(
        TWO * denom.x,   ZERO,            ZERO,            ZERO,
        ZERO,            TWO * denom.y,   ZERO,            ZERO,
        ZERO,            ZERO,            -TWO * denom.z,   ZERO,
        offset...,                                         ONE
    )
end


export m3_translate, m4_translate, m_scale,
       m3_rotateX, m3_rotateY, m3_rotateZ,
       m4_rotateX, m4_rotateY, m4_rotateZ,
       m3_rotate, m4_rotate,
       m4_world, m3_look_at, m4_look_at,
       m4_projection, m4_ortho