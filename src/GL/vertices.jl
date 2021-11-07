#=
The types of data that can be read as input into the vertex shader are:
    * 1D - 4D vector of 32-bit int (or uint)
    * 1D - 4D vector of 64-bit doubles
    * 2x2 - 4x4 matrix of 32-bit floats or 64-bit doubles
    * 1D - 4D vector of 32-bit float
Most of these inputs are simple, except for the most common type: 32-bit float vectors.
Float vectors can be created from all sorts of Buffer data:
    * It can come from 16-bit, 32-bit, or 64-bit float components
    * It can come from int/uint components, normalized (or alternatively, just casted fromt int to float)
    * It can come from 32-bit fixed decimal components
    * It can come from special pre-packed data formats
=#


###################################
#        Vertex Data Types        #
###################################

"The root type for any kind of vertex data coming from a Buffer into a vertex shader"
abstract type VertexData end

"Gets the byte-size of one element of the given VertexData type"
vertex_data_byte_size(t::Type{<:VertexData}) = error("vertex_data_byte_size() not implemented for ", t)

"Gets the OpenGL enum for the given VertexData type"
get_component_ogl_enum(t::Type{<:VertexData}) = error("get_component_ogl_enum() not implemented for ", t)

"
Gets the number of components in this type's OpenGL vertex attributes
   (i.e. the length of a vector, or the number of columns in a matrix).
"
count_components(t::Type{<:VertexData}) = error("count_components() not implemented for ", t)

"
Gets the number of OpenGL vertex attributes that are needed for this data type
   (i.e. 1 for a vector, or the number of rows in a matrix).
"
count_attribs(t::Type{<:VertexData}) = error("count_attribs() not implemented for ", t)

export VertexData,
       vertex_data_byte_size, get_component_ogl_enum, count_attribs


const VertexData_VecSizes = Union{Val{1}, Val{2}, Val{3}, Val{4}}
const VertexData_MatrixSizes = Union{Val{2}, Val{3}, Val{4}}
const VertexData_Bool = Union{Val{true}, Val{false}}


#####################################
#        Vertex Data: Matrix        #
#####################################

"Vertex data that gets interpreted as matrices of floats"
abstract type VertexData_Matrix{ Cols<:VertexData_MatrixSizes,
                                 Rows<:VertexData_MatrixSizes,
                                 F<:Union{Float32, Float64}
                               } <: VertexData
end
@inline VertexData_Matrix(cols::Integer, rows::Integer, F::Type{<:AbstractFloat}) = VertexData_Matrix{Val(Int(cols)), Val(Int(rows)), F}

const VertexData_MatrixF{Col, Row} = VertexData_Matrix{Col, Row, Float32}
const VertexData_MatrixD{Col, Row} = VertexData_Matrix{Col, Row, Float64}

export VertexData_Matrix, VertexData_MatrixF, VertexData_MatrixD


vertex_data_byte_size(::Type{VertexData_Matrix{Val{C}, Val{R}, F}}) where {C, R, F} = (C * R * sizeof(F))
function get_component_ogl_enum(::Type{VertexData_Matrix{Val{C}, Val{R}, F}}) where {C, R, F}
    if F == Float32
        return GL_FLOAT
    elseif F == Float64
        return GL_DOUBLE
    else
        error("Unhandled case: ", F)
    end
end
count_components(::Type{VertexData_Matrix{Val{C}, Val{R}, F}}) where {C, R, F} = C
count_attribs(::Type{VertexData_Matrix{Val{C}, Val{R}, F}}) where {C, R, F} = R


######################################
#        Vertex Data: IVector        #
######################################

"
Vertex data that comes in as 1D-4D vectors of 32-bit ints or uints,
   and comes out that way in the shader.
"
abstract type VertexData_IVector{ N<:VertexData_VecSizes,
                                  I<:Union{Int8, Int16, Int32,
                                           UInt8, UInt16, UInt32}
                                } <: VertexData
end

VertexData_IVector(n::Integer, i::Type{<:Integer}) = VertexData_IVector{Val(Int(n)), i}

export VertexData_IVector


vertex_data_byte_size(::Type{VertexData_IVector{Val{N}, I}}) where {N, I} = (N * sizeof(I))
function get_component_ogl_enum(::Type{VertexData_IVector{Val{N}, I}}) where {N, I}
    if I == Int8
        return GL_BYTE
    elseif I == Int16
        return GL_SHORT
    elseif I == Int32
        return GL_INT
    elseif I == UInt8
        return GL_UNSIGNED_BYTE
    elseif I == UInt16
        return GL_UNSIGNED_SHORT
    elseif I == UInt32
        return GL_UNSIGNED_INT
    else
        error("Unhandled case: ", I)
    end
end
count_components(::Type{VertexData_IVector{Val{N}, I}}) where {N, I} = N
count_attribs(::Type{VertexData_IVector{Val{N}, I}}) where {N, I} = 1


######################################
#        Vertex Data: DVector        #
######################################

"Vertex data that comes in as 64-bit float vectors, and come out in the same way"
abstract type VertexData_DVector{N<:VertexData_VecSizes} <: VertexData end

export VertexData_DVector


vertex_data_byte_size(::Type{VertexData_DVector{N}}) where {N} = N * sizeof(Float64)
get_component_ogl_enum(::Type{<:VertexData_DVector}) = GL_DOUBLE
count_components(::Type{VertexData_DVector{Val{N}}}) where {N, I} = N
count_attribs(::Type{VertexData_DVector{Val{N}}}) where {N, I} = 1


######################################
#        Vertex Data: FVector        #
######################################

"Vertex data that gets interpreted as 32-bit float vectors"
abstract type VertexData_FVector <: VertexData end


"
Vertex data that comes in as some kind of float vector,
  and gets interpreted as 32-bit float vector.
"
abstract type VertexData_FVector_Simple{ N<:VertexData_VecSizes,
                                         FIn<:Union{Float16, Float32, Float64}
                                       } <: VertexData_FVector
end
vertex_data_byte_size(::Type{VertexData_FVector_Simple{Val{N}, FIn}}) where {N, FIn} = (N * sizeof(FIn))
function get_component_ogl_enum(::Type{VertexData_FVector_Simple{Val{N}, FIn}}) where {N, FIn}
    if FIn == Float16
        return GL_HALF_FLOAT
    elseif FIn == Float32
        return GL_FLOAT
    elseif FIn == Float64
        return GL_DOUBLE
    else
        error("Unhandled case: ", FIn)
    end
end
count_components(::Type{VertexData_FVector_Simple{Val{N}, F}}) where {N, F} = N
count_attribs(::Type{VertexData_FVector_Simple{Val{N}, F}}) where {N, F} = 1


"
Vertex data that comes in as some kind of integer vector,
  and gets interpreted as 32-bit float vector.
If 'Normalize' is off, then the values are converted with a simple cast.
If 'Normalize' is on, then the values are converted from the integer's range to the range [0, 1] or [-1, 1].
"
abstract type VertexData_FVector_Int{ N<:VertexData_VecSizes,
                                      FIn<:Union{Int8, Int16, Int32, UInt8, UInt16, UInt32},
                                      Normalize<:VertexData_Bool
                                    } <: VertexData_FVector
end
vertex_data_byte_size(::Type{VertexData_FVector_Int{Val{N}, FIn, Normalize}}) where {N, FIn, Normalize} =
    (N * sizeof(FIn))
function get_component_ogl_enum(::Type{VertexData_FVector_Int{Val{N}, FIn, Normalize}}) where {N, FIn, Normalize}
    if FIn == Int8
        return GL_BYTE
    elseif FIn == Int16
        return GL_SHORT
    elseif FIn == Int32
        return GL_INT
    elseif FIn == UInt8
        return GL_UNSIGNED_BYTE
    elseif FIn == UInt16
        return GL_UNSIGNED_SHORT
    elseif FIn == UInt32
        return GL_UNSIGNED_INT
    else
        error("Unhandled case: ", FIn)
    end
end
count_components(::Type{VertexData_FVector_Int{Val{N}, F}}) where {N, F} = N
count_attribs(::Type{VertexData_FVector_Int{Val{N}, F}}) where {N, F} = 1


"
Vertex data that comes in as a vector of fixed-point decimals
   (16 bits for integer, and 16 for fractional),
   and gets interpreted as a vector of 32-bit float.
"
abstract type VertexData_FVector_Fixed{N<:VertexData_VecSizes} <: VertexData_FVector end
VertexData_FVector_Fixed(n::Integer) = VertexData_FVector_Fixed{Val(Int(n))}
vertex_data_byte_size(::Type{VertexData_FVector_Fixed{Val{N}}}) where {N} = (N * 4)
get_component_ogl_enum(::Type{<:VertexData_FVector_Fixed}) = GL_FIXED
count_components(::Type{VertexData_FVector_Fixed{Val{N}}}) where {N} = N
count_attribs(::Type{VertexData_FVector_Fixed{Val{N}}}) where {N} = 1


"
Vertex data that comes in as a 32-bit uint, and gets interpreted as
    an RGB vector of 32-bit floats.
The uint stores 3 unsigned floats, with 10 bits for Blue/Z,
    then 11 for Green/Y, then 11 more for Red/X.
"
abstract type VertexData_FVector_Pack_UFloat_B10_G11_R11 <: VertexData_FVector end
vertex_data_byte_size(::Type{VertexData_FVector_Pack_UFloat_B10_G11_R11}) = 4
get_component_ogl_enum(::Type{VertexData_FVector_Pack_UFloat_B10_G11_R11}) = GL_UNSIGNED_INT_10F_11F_11F_REV
count_components(::Type{VertexData_FVector_Pack_UFloat_B10_G11_R11}) = 3
count_attribs(::Type{VertexData_FVector_Pack_UFloat_B10_G11_R11}) = 1

"
Vertex data that comes in as a 32-bit uint, and gets interpreted as
  an RGBA vector of 32-bit floats.
The uint stores the components as integers, optionally signed and/or normallized:
  2 bits for Alpha/W,
  10 bits for Blue/Z,
  10 bits for Green/Y,
  then 10 bis for Red/X.
"
abstract type VertexData_FVector_Pack_Integer_A2_BGR10{ Signed<:VertexData_Bool,
                                                        Normalized<:VertexData_Bool
                                                      } <: VertexData_FVector end
VertexData_FVector_Pack_Integer_A2_BGR10(signed = false, normalized = true) = VertexData_FVector_Pack_Integer_A2_BGR10{Val(signed), Val(normalized)}
vertex_data_byte_size(::Type{VertexData_FVector_Pack_Integer_A2_BGR10}) = 4
function get_component_ogl_enum(::Type{VertexData_FVector_Pack_Integer_A2_BGR10{Val{Sign}, Val{Norm}}}) where {Sign, Norm}
    if Sign
        return GL_INT_2_10_10_10_REV
    else
        return GL_UNSIGNED_INT_2_10_10_10_REV
    end
end
count_components(::Type{<:VertexData_FVector_Pack_Integer_A2_BGR10}) = 4
count_attribs(::Type{<:VertexData_FVector_Pack_Integer_A2_BGR10}) = 1


VertexData_FVector(n::Integer, f_in::Type{<:AbstractFloat}) = VertexData_FVector_Simple{Val(Int(n)), f_in}
VertexData_FVector(n::Integer, f_in::Type{<:Integer}, normalize::Bool) = VertexData_FVector_Int{Val(Int(n)), f_in, Val(normalize)}

vert_data_is_normalized(::Type{VertexData_FVector_Int{Val{N}, FIn, Val{Norm}}}) where {N, FIn, Norm} = Norm
vert_data_is_normalized(::Type{VertexData_FVector_Pack_Integer_A2_BGR10{Val{Sign}, Val{Norm}}}) where {Sign, Norm} = Norm



export VertexData_FVector,
       VertexData_FVector_Simple, VertexData_FVector_Int,
       VertexData_FVector_Fixed,
       VertexData_FVector_Pack_UFloat_B10_G11_R11,
       VertexData_FVector_Pack_Integer_A2_BGR10