using Setfield, StaticArrays

# Fortunately, both GLSL and StaticArrays use a column-major order for matrices in memory.
# This saves us from having to roll our own matrix struct.

# However, the way GLSL names matrices is with their column count THEN row count,
#    while SMatrix uses row count THEN column count.
# We will follow the GLSL convention; hopefully this doesn't result in much confusion.

# Unfortunately, due to the nature of Julia and SMatrix, all matrices need an extra type parameter,
#    for the total length of the matrix.
# There is no feasible way of avoiding this, except for the concrete aliases.

"
The type for a matrix with the given rows, columns, and component type.
The last parameter must be the number of elements (a.k.a. R*C).
"
const Mat{C, R, F, Len} = SMatrix{R, C, F, Len}

"
Short-hand for matrix types that calculates the last type parameter automatically.
For example: `@Mat(2, 4, Float32)` makes `fmat2x4`.
"
#TODO: Change syntax to @Mat{C, R, F}
macro Mat(C, R, F)
    return esc(:( Mat{$C, $R, $F, $C * $R} ))
end


"Splits a matrix type into its important type arguments: Columns, Rows, and Component"
mat_params(::Type{Mat{C, R, F, Len}}) where {C, R, F, Len} = (C, R, F)


const MatT{F, C, R, Len} = Mat{C, R, F, Len}

const MatF{C, R, Len} = Mat{C, R, Float32, Len}
const fmat2x2 = MatF{2, 2, 2*2}
const fmat3x3 = MatF{3, 3, 3*3}
const fmat4x4 = MatF{4, 4, 4*4}
const fmat2x3 = MatF{2, 3, 2*3}
const fmat2x4 = MatF{2, 4, 2*4}
const fmat3x2 = MatF{3, 2, 3*2}
const fmat3x4 = MatF{3, 4, 3*4}
const fmat4x2 = MatF{4, 2, 4*2}
const fmat4x3 = MatF{4, 3, 4*3}
const fmat2 = fmat2x2
const fmat3 = fmat3x3
const fmat4 = fmat4x4

const MatD{C, R, Len} = Mat{C, R, Float64, Len}
const dmat2x2 = MatD{2, 2, 2*2}
const dmat3x3 = MatD{3, 3, 3*3}
const dmat4x4 = MatD{4, 4, 4*4}
const dmat2x3 = MatD{2, 3, 2*3}
const dmat2x4 = MatD{2, 4, 2*4}
const dmat3x2 = MatD{3, 2, 3*2}
const dmat3x4 = MatD{3, 4, 3*4}
const dmat4x2 = MatD{4, 2, 4*2}
const dmat4x3 = MatD{4, 3, 4*3}
const dmat2 = dmat2x2
const dmat3 = dmat3x3
const dmat4 = dmat4x4

const Mat2{F} = Mat{2, 2, F, 2*2}
const Mat3{F} = Mat{3, 3, F, 3*3}
const Mat4{F} = Mat{4, 4, F, 4*4}

export Mat, @Mat, MatT, MatF, MatD, Mat2, Mat3, Mat4,
       mat_params,
       fmat2, fmat3, fmat4, dmat2, dmat3, dmat4,
       fmat2x2, fmat2x3, fmat2x4, fmat3x2, fmat3x3, fmat3x4, fmat4x2, fmat4x3, fmat4x4,
       dmat2x2, dmat2x3, dmat2x4, dmat3x2, dmat3x3, dmat3x4, dmat4x2, dmat4x3, dmat4x4
#


# Keep in mind that the matrix elements are grouped by column,
#    which looks confusing because they're written out in rows.
# E.x. the matrix Mat{2, 3, Float32}(1, 2, 3,
#                                    4, 5, 6)
#    has a column of "1, 2, 3" and a column of "4, 5, 6".


@inline Base.:*(m::Mat, v::Vec) = Vec((m * SVector(v...))...)
#NOTE: While we can legally define v*m as well,
#        it creates ambiguity as to whether B+ is post- or pre-multiplied.
#      You can achieve it manually pretty easily anyway.
@inline Base.:*(v::Vec, m::Mat) = error("B+ uses pre-multiplication. Instead of doing v*m, do m'*v.")

"Applies a transform matrix to the given coordinate."
function m_apply_point(m::Mat{4, 4, F1}, v::Vec{3, F2})::Vec{3} where {F1, F2}
    v4::Vec{4, F2} = vappend(v, one(F2))
    u4::Vec{4, promote_type(F1, F2)} = m * v4
    return u4.xyz / u4.w
end
function m_apply_point(m::Mat{3, 3, F1}, v::Vec{2, F2})::Vec{2} where {F1, F2}
    v3::Vec{3, F2} = vappend(v, one(F2))
    u3::Vec{3, promote_type(F1, F2)} = m * v3
    return u3.xy / u3.z
end
m_apply_point(m::Mat{N, N}, v::Vec{N}) where {N} = m * v
"
Applies a transform matrix to the given coordinate, assuming the homogeneous component stays at 1.0
    and does not need to be processed.
This is generally true for world and view matrices, but not projection matrices.
"
function m_apply_point_affine(m::Mat{3, 3, F1}, v::Vec{2, F2}) where {F1, F2}
    return (m * vappend(v, one(F2))).xy
end
function m_apply_point_affine(m::Mat{4, 4, F1}, v::Vec{3, F2}) where {F1, F2}
    return (m * vappend(v, one(F2))).xyz
end
"Applies a transform matrix to the given vector (i.e. ignoring translation)."
function m_apply_vector(m::Mat{4, 4, F1}, v::Vec{3, F2}) where {F1, F2}
    v4::Vec{4, F2} = vappend(v, zero(F2))
    u4::Vec{4, promote_type(F1, F2)} = m * v4
    return u4.xyz / u4.w
end
function m_apply_vector(m::Mat{3, 3, F1}, v::Vec{2, F2}) where {F1, F2}
    v3::Vec{3, F2} = vappend(v, zero(F2))
    u3::Vec{3, promote_type(F1, F2)} = m * v3
    return u3.xy / u3.z
end
m_apply_vector(m::Mat{N, N}, v::Vec{N}) where {N} = m * v
"
Applies a transform matrix to the given vector (i.e. ignoring translation),
    assuming the homogeneous component stays at 1.0 and does not need to be processed.
This is generally true for world and view matrices, but not projection matrices.
"
function m_apply_vector_affine(m::Mat{3, 3, F1}, v::Vec{2, F2}) where {F1, F2}
    # No translation *and* no skew means we can just drop the last row and column.
    # Unfortunately, slicing a static array does not yield another static array.
    m2 = @Mat(2, 2, F1)(m[1], m[2],
                        m[4], m[5])
    return m2 * v
end
function m_apply_vector_affine(m::Mat{4, 4, F1}, v::Vec{3, F2}) where {F1, F2}
    # No translation *and* no skew means we can just drop the last row and column.
    # Unfortunately, slicing a static array does not yield another static array.
    m3 = @Mat(3, 3, F1)(m[1], m[2], m[3],
                        m[5], m[6], m[7],
                        m[9], m[10], m[11])
    return m3 * v
end
export m_apply_point, m_apply_vector,
       m_apply_point_affine, m_apply_vector_affine

"Combines transform matrices in chronologicl order."
@inline m_combine(first::Mat, rest::Mat...) = m_combine(rest...) * first
@inline m_combine(m::Mat) = m
export m_combine

"Inverts the given matrix"
const m_invert = StaticArrays.inv
export m_invert

"Transposes the given matrix"
const m_transpose = StaticArrays.transpose
export m_transpose

"Creates an identity matrix"
m_identity(n_cols::Int, n_rows::Int, F::Type{<:AbstractFloat}) = one(Mat{n_cols, n_rows, F, n_cols*n_rows})
"Creates an identity matrix with Float32 components"
m_identityf(n_cols::Int, n_rows::Int) = m_identity(n_cols, n_rows, Float32)
"Creates an identity matrix with Float64 components"
m_identityd(n_cols::Int, n_rows::Int) = m_identity(n_cols, n_rows, Float64)
export m_identity, m_identityf, m_identityd

"Embeds a 3x3 matrix into a 4x4 matrix, using 0 for the new components except for 1 at {4, 4}"
@inline m_to_mat4x4(m::Mat{3, 3, F, 9}) where {F} = Mat{4, 4}(
    m[:, 1]..., zero(F),
    m[:, 2]..., zero(F),
    m[:, 3]..., zero(F),
    zero(F), zero(F), zero(F), one(F)
)
"Strips out the last row and column of a 4x4 matrix, to get a 3x3 matrix"
@inline m_to_mat3x3(m::Mat{4, 4, F, 16}) where {F} = Mat{3, 3}(
    m[1:3, 1]...,
    m[1:3, 2]...,
    m[1:3, 3]...
)
export m_to_mat4x4, m_to_mat3x3