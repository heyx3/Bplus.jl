Part of the [`Bplus.Math` module](Math.md). The core game math types are: `Vec{N, T}`, [`Quaternion{F}`](Quat.md), and [`Mat{C, R, F}`](Matrix.md).

# `Mat{C, R, F}`

A statically-sized transform matrix.

## Aliases

Aliases are particularly important for `Mat` to avoid confusion (see the section [StaticArrays](#StaticArrays) below).

Matrices are named very similarly to HLSL, with the prefix `f` for Float32 and `d` for Float64, and the suffix `[columns]x[rows]`. If the row and column count is the same, you may abbreviate the suffix with just one number.

For example:
* `fmat2x2` is a 2x2 Float32 matrix.
* `dmat3x4` is a 3-column, 4-row Float64 matrix.
* `fmat3` is a 3x3 Float32 matrix.

There are also aliases for specific matrix sizes:
* `Mat2{F}` is a 2x2 matrix.
* `Mat3{F}` is a 3x3 matrix.
* `Mat4{F}` is a 4x4 matrix.

If you need to manually specify a new matrix type, it's highly recommended to use the macro `@Mat(C, R, F)` because Julia's type system actually requires a fourth parameter which is `C * R`.

## Usage

In keeping with vectors and quaternions, matrix operations are prefixed with `m_`:

* To apply a matrix transform to a point, use one of the following:
  * `m_apply_point(m, p)` for coordinates
  * `m_apply_vector(m, v)` for vectors (ignores translation)
  * If your matrix is affine (almost always true for world and view matrices, but not for a perspective projection matrix), you can use optimized versions:
    * `m_apply_point_affine(m, p)` for coordinates and affine matrices.
    * `m_apply_vector_affine(m, v)` for vectors and affine matrices.
* `m_combine(a, b...)` multiplies matrices together in chronological order, to produce a transformation of "a, then b, then ..."
* `m_invert(m)` inverts a matrix.
* `m_tranpose(m)` transposes a matrix.
  * You can also use the `'` operator, for example `m = m'`.
* `m_identity(C, R, F)` creates an identity matrix.
  * `m_identityf(C, R)` creates a Float32 matrix.
  * `m_identityd(C, R)` creates a Float64 matrix.
* `m_to_mat4x4(m)` converts a 3x3 matrix to 4x4.
* `m_to_mat3x3(m)` drops the last row and column off a 4x4 matrix.
* Operator `*` performs multiplication with `Vec`, or other `Mat` instances. You can use it to transform a point/vector by a matrix, but the above explicit functions are preferred.
  * Post-multiplication (i.e. `v*M`) is not legal, in order to make it clear that B+ uses pre-multiplication.

## Transforms

* `m3_translate(delta)` and `m4_translate(delta)` make 2D and 3D translation matrices, respectively.
* `m_scale(v)` and `m_scale(f, N)` creates a scale matrix using a vector or a scalar spread over N dimensions, respectively.
* `m[N]_rotate[A](radians)` makes a rotation matrix around axis `A`, in 3x3 or 4x4 format `N`.
  * For example, `m3_rotateX(radians)`.
* `m3_rotate(q)` and `m4_rotate(q)` make a rotation matrix for a quaternion.
* `m4_world(pos, rot, scale)` makes a typical World transform matrix.
* `m4_look_at(cam_pos, target_pos, up)` makes a typical View transform matrix.
* `m3_look_at(forward, up, right; [options])` makes a rotation matrix to turn the given basis into a typical View basis (by default: +X right, +Y up, -Z forward).
  * You can also think of this as a camera View matrix without the translation.
* `m3_look_at(from::VBasis, to::VBasis)` makes a rotation matrix that turns the "from" vector basis into the "to" vector basis.
* `m4_projection(near_clip, far_clip, aspect_width_over_height, fov_degrees)` creates a typical OpenGL perspective matrix.
* `m4_ortho(range::Box3)` makes an orthographic projection matrix, mapping the given axis-aligned box to the range -1 => +1 along each axis.

## Component access

`Mat` has the same access patterns as other multidimensional arrays.

Unfortunately, indexing with integer `Vec` values is counter-intuitive: the X component is the row, and the Y component is the column.

## StaticArrays

Under the hood, `Mat` is just an alias for `StaticMatrix`, or `SMatrix`, from the *StaticArrays* package. However it has different convention for the type parameters in order to be aligned with OpenGL -- Column then Row, whereas  `SMatrix` specifies Row then Column.

Fortunately, the memory order of matrices is already in sync between *StaticArrays* and OpenGL -- column-major, meaning the second element in memory (in Julia, `my_matrix[2]`) is column-1, row-2.

Due to the nature of Julia's type system, all `SMatrix` types come with a fourth type parameter, which is the total length (row count * column count), even though this can be computed trivially from `C` and `R`. So for convenience, the macro `@Mat(C, R, F)` is provided.

And of course, there are plenty of aliases to cover 99% of cases, [mentioned above](#Aliases).