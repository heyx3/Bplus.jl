using Setfield, StaticArrays

# Fortunately, both GLSL and StaticArrays use a column-major order for matrices in memory.
# This saves us from having to roll our own matrix struct.

# However, the way GLSL names matrices is with their column count THEN row count,
#    while SMatrix uses row count THEN column count.
# Hopefully this doesn't result in much confusion.

mat{C, R, F} = SMatrix{R, C, F}

fmat{C, R} = mat{C, R, Float32}
fmat2x2 = fmat{2, 2}
fmat3x3 = fmat{3, 3}
fmat4x4 = fmat{4, 4}
fmat2x3 = fmat{2, 3}
fmat2x4 = fmat{2, 4}
fmat3x2 = fmat{3, 2}
fmat3x4 = fmat{3, 4}
fmat4x2 = fmat{4, 2}
fmat4x3 = fmat{4, 3}
fmat2 = fmat2x2
fmat3 = fmat3x3
fmat4 = fmat4x4

dmat{C, R} = mat{C, R, Float64}
dmat2x2 = dmat{2, 2}
dmat3x3 = dmat{3, 3}
dmat4x4 = dmat{4, 4}
dmat2x3 = dmat{2, 3}
dmat2x4 = dmat{2, 4}
dmat3x2 = dmat{3, 2}
dmat3x4 = dmat{3, 4}
dmat4x2 = dmat{4, 2}
dmat4x3 = dmat{4, 3}
dmat2 = dmat2x2
dmat3 = dmat3x3
dmat4 = dmat4x4

export mat, fmat, dmat,
       fmat2, fmat3, fmat4, dmat2, dmat3, dmat4,
       fmat2x2, fmat2x3, fmat2x4, fmat3x2, fmat3x3, fmat3x4, fmat4x2, fmat4x3, fmat4x4,
       dmat2x2, dmat2x3, dmat2x4, dmat3x2, dmat3x3, dmat3x4, dmat4x2, dmat4x3, dmat4x4

       
# Expose the various matrix-related functions from StaticArrays:
export inv


#TODO: another file 'transformations.jl' handling world, view, and projection math.