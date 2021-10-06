using Setfield, StaticArrays

# Fortunately, both GLSL and StaticArrays use a column-major order for matrices in memory.
# This saves us from having to roll our own matrix struct.

# However, the way GLSL names matrices is with their column count THEN row count,
#    while SMatrix uses row count THEN column count.
# Hopefully this doesn't result in much confusion.

const mat{C, R, F} = SMatrix{R, C, F}

const fmat{C, R} = mat{C, R, Float32}
const fmat2x2 = fmat{2, 2}
const fmat3x3 = fmat{3, 3}
const fmat4x4 = fmat{4, 4}
const fmat2x3 = fmat{2, 3}
const fmat2x4 = fmat{2, 4}
const fmat3x2 = fmat{3, 2}
const fmat3x4 = fmat{3, 4}
const fmat4x2 = fmat{4, 2}
const fmat4x3 = fmat{4, 3}
const fmat2 = fmat2x2
const fmat3 = fmat3x3
const fmat4 = fmat4x4

const dmat{C, R} = mat{C, R, Float64}
const dmat2x2 = dmat{2, 2}
const dmat3x3 = dmat{3, 3}
const dmat4x4 = dmat{4, 4}
const dmat2x3 = dmat{2, 3}
const dmat2x4 = dmat{2, 4}
const dmat3x2 = dmat{3, 2}
const dmat3x4 = dmat{3, 4}
const dmat4x2 = dmat{4, 2}
const dmat4x3 = dmat{4, 3}
const dmat2 = dmat2x2
const dmat3 = dmat3x3
const dmat4 = dmat4x4

export mat, fmat, dmat,
       fmat2, fmat3, fmat4, dmat2, dmat3, dmat4,
       fmat2x2, fmat2x3, fmat2x4, fmat3x2, fmat3x3, fmat3x4, fmat4x2, fmat4x3, fmat4x4,
       dmat2x2, dmat2x3, dmat2x4, dmat3x2, dmat3x3, dmat3x4, dmat4x2, dmat4x3, dmat4x4


"Inverts the given matrix"
const m_invert = StaticArrays.inv
export m_invert

"Creates an identity matrix"
m_identity(n_cols::Int, n_rows::Int, F::Type{<:AbstractFloat}) = one(mat{n_cols, n_rows, F})
"Creates an identity matrix with Float32 components"
m_identityf(n_cols::Int, n_rows::Int) = m_identity(n_cols, n_rows, Float32)
"Creates an identity matrix with Float64 components"
m_identityd(n_cols::Int, n_rows::Int) = m_identity(n_cols, n_rows, Float64)
export m_identity, m_identityf, m_identityd

"""
Embeds a 3x3 matrix into a 4x4 matrix.
A 4x4 matrix is needed to represent 3D translations.
"""
to_mat4x4(m::mat{3, 3, F}) where {F} =
    @SMatrix [ m[1]    m[4]    m[7]    zero(F)
               m[2]    m[5]    m[8]    zero(F)
               m[3]    m[6]    m[9]    zero(F)
               zero(F) zero(F) zero(F) one(F)  ]
export to_mat4x4



#TODO: another file 'transformations.jl' handling world, view, and projection math.