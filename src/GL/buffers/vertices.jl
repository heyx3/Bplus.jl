#=
The types of Buffer data that can be read as input into the vertex shader are:
    * 1D - 4D vector of 32-bit int (or uint)
    * 1D - 4D vector of 64-bit doubles
    * 2x2 - 4x4 matrix of 32-bit floats or 64-bit doubles
    * 1D - 4D vector of 32-bit float

Most of these inputs are simple, except for the most common type: 32-bit float vectors.
Float vectors can be created from any of the following:
    * From 16-bit, 32-bit, or 64-bit float components
    * From int/uint components, normalized (or alternatively, casted from int to float)
    * From 32-bit fixed decimal components
    * From special pre-packed data formats
=#

"Some kind of vertex data that can be read from a Buffer."
abstract type AbstractVertexShaderInput end


"Gets the byte-size of one vertex input of the given type"
vertex_data_byte_size(i::AbstractVertexShaderInput) = error("vertex_data_byte_size() not implemented for ", i)

"Gets the OpenGL enum for the given type"
get_component_ogl_enum(i::AbstractVertexShaderInput) = error("get_component_ogl_enum() not implemented for ", i)

"
Gets the number of components in this type's OpenGL vertex attributes
   (i.e. the length of a vector, or the number of columns in a matrix).
"
count_components(i::AbstractVertexShaderInput) = error("count_components() not implemented for ", i)

"
Gets the number of OpenGL vertex attributes that are needed for this data type
   (i.e. 1 for a vector, or the number of rows in a matrix).
"
count_attribs(i::AbstractVertexShaderInput) = error("count_attribs() not implemented for ", i)


export AbstractVertexShaderInput
#


###############################
#         Basic Types         #
###############################

@bp_enum(VertexInputVectorDimensions, D1=1, D2=2, D3=3, D4=4)
@bp_enum(VertexInputMatrixDimensions, D2=2, D3=3, D4=4)

@bp_enum(VertexInputIntSizes, B8=1, B16=2, B32=4)
@bp_enum(VertexInputFloatSizes, B16=2, B32=4, B64=8)

@bp_enum(VertexInputMatrixTypes, float32=4, float64=8)


struct VSInput_IntVector <: AbstractVertexShaderInput
    n_dimensions::E_VertexInputVectorDimensions
    is_signed::Bool
    type::E_VertexInputIntSizes
end
struct VSInput_DoubleVector <: AbstractVertexShaderInput
    n_dimensions::E_VertexInputVectorDimensions
end

struct VSInput_Matrix <: AbstractVertexShaderInput
    n_rows::E_VertexInputMatrixDimensions
    n_columns::E_VertexInputMatrixDimensions
    type::E_VertexInputMatrixTypes
end

abstract type VSInput_FloatVector <: AbstractVertexShaderInput end


export VSInput_IntVector, VSInput_FloatVector, VSInput_DoubleVector, VSInput_Matrix


#########################################
#         Basic Implementations         #
#########################################

vertex_data_byte_size(i::VSInput_IntVector) = sizeof(i.is_signed ? Int32 : UInt32) * Int(i.n_dimensions)
vertex_data_byte_size(i::VSInput_DoubleVector) = sizeof(Float64) * Int(i.n_dimensions)
vertex_data_byte_size(i::VSInput_Matrix) = Int(i.type) * Int(i.n_rows) * Int(i.n_columns)

function get_component_ogl_enum(i::VSInput_IntVector)
    if i.is_signed
        if i.type == VertexInputIntSizes.B8
            return GL_BYTE
        elseif i.type == VertexInputIntSizes.B16
            return GL_SHORT
        elseif i.type == VertexInputIntSizes.B32
            return GL_INT
        else
            error("Unimplemented: signed ", i.type)
        end
    else # Unsigned
        if i.type == VertexInputIntSizes.B8
            return GL_UNSIGNED_BYTE
        elseif i.type == VertexInputIntSizes.B16
            return GL_UNSIGNED_SHORT
        elseif i.type == VertexInputIntSizes.B32
            return GL_UNSIGNED_INT
        else
            error("Unimplemented: unsigned ", i.type)
        end
    end
end
function get_component_ogl_enum(i::VSInput_DoubleVector)
    return GL_DOUBLE
end
function get_component_ogl_enum(i::VSInput_Matrix)
    if i.type == VertexInputMatrixTypes.float32
        return GL_FLOAT
    elseif i.type == VertexInputMatrixTypes.float64
        return GL_DOUBLE
    else
        error("Unhandled case: ", i.type)
    end
end

count_components(i::VSInput_IntVector) = Int(i.n_dimensions)
count_components(i::VSInput_DoubleVector) = Int(i.n_dimensions)
count_components(i::VSInput_Matrix) = Int(i.n_columns)

count_attribs(i::VSInput_IntVector) = 1
count_attribs(i::VSInput_DoubleVector) = 1
count_attribs(i::VSInput_Matrix) = Int(i.n_rows)


#########################################
#         Float Implementations         #
#########################################

fvector_data_is_normalized(i::VSInput_FloatVector) = error("Not implemented: ", typeof(i))

# Assume all sub-types have a field 'n_dimensions::E_VertexInputVectorDimensions'.
count_components(i::VSInput_FloatVector) = Int(i.n_dimensions)
count_attribs(::VSInput_FloatVector) = 1


"
Vertex data that comes in as some kind of float vector (e.x. Float16)
    and appears in the shader as a Float32 vector
"
struct VSInput_FVector_Plain <: VSInput_FloatVector
    n_dimensions::E_VertexInputVectorDimensions
    in_size::E_VertexInputFloatSizes
end
VSInput_FVector_Plain(n_dimensions::Integer, component_byte_size::Integer) = VSInput_FVector_Plain(
    E_VertexInputVectorDimensions(n_dimensions),
    E_VertexInputFloatSizes(component_byte_size)
)
VSInput_FVector_Plain(n_dimensions::Integer, component_type::Type{<:AbstractFloat}) = VSInput_FVector_Plain(
    E_VertexInputVectorDimensions(n_dimensions),
    E_VertexInputFloatSizes(sizeof(component_type))
)
fvector_data_is_normalized(i::VSInput_FVector_Plain) = false
vertex_data_byte_size(i::VSInput_FVector_Plain) = Int(i.in_size) * Int(i.n_dimensions)
function get_component_ogl_enum(i::VSInput_FVector_Plain)
    if i.in_size == VertexInputFloatSizes.B16
        return GL_HALF_FLOAT
    elseif i.in_size == VertexInputFloatSizes.B32
        return GL_FLOAT
    elseif i.in_size == VertexInputFloatSizes.B64
        return GL_DOUBLE
    else
        error("Unhandled case: ", i.in_size)
    end
end


"
Vertex data that comes in as some kind of integer vector,
    and gets interpreted as 32-bit float vector.

If 'normalized' is true, then the values are converted
    from the integer's range to the range [0, 1] or [-1, 1].
Otherwise, the values are simply casted to float.
"
struct VSInput_FVector_FromInt <: VSInput_FloatVector
    input::VSInput_IntVector
    normalized::Bool
end
fvector_data_is_normalized(i::VSInput_FVector_FromInt) = i.normalized
count_components(i::VSInput_FVector_FromInt) = count_components(i.input)
vertex_data_byte_size(i::VSInput_FVector_FromInt) = vertex_data_byte_size(i.input)
get_component_ogl_enum(i::VSInput_FVector_FromInt) = get_component_ogl_enum(i.input)


"
Vertex data that comes in as a vector of fixed-point decimals (16 integer bits, 16 fractional bits)
    and gets casted into float32
"
struct VSInput_FVector_Fixed <: VSInput_FloatVector
    n_dimensions::E_VertexInputVectorDimensions
end
fvector_data_is_normalized(i::VSInput_FVector_Fixed) = false
vertex_data_byte_size(i::VSInput_FVector_Fixed) = 4 * Int(i.n_dimensions)
get_component_ogl_enum(i::VSInput_FVector_Fixed) = GL_FIXED


"
Vertex data that comes in as a 32-bit uint, and gets interpreted as
    an RGB vector of 32-bit floats.
The uint stores 3 unsigned floats, in order:
  1. 10 bits for Blue/Z
  2. 11 bits for Green/Y
  3. 11 more for Red/X

#TODO: How many bits for mantissa vs exponent?
"
struct VSInput_FVector_Packed_UF_B10_G11_R11 <: VSInput_FloatVector
end
fvector_data_is_normalized(i::VSInput_FVector_Packed_UF_B10_G11_R11) = false
count_components(i::VSInput_FVector_Packed_UF_B10_G11_R11) = 3
vertex_data_byte_size(i::VSInput_FVector_Packed_UF_B10_G11_R11) = 4
get_component_ogl_enum(i::VSInput_FVector_Packed_UF_B10_G11_R11) = GL_UNSIGNED_INT_10F_11F_11F_REV


"
Vertex data that comes in as a 32-bit uint, and gets interpreted as
  an RGBA vector of 32-bit floats.
The uint stores the components as integers, optionally signed and/or normalized:
  2 bits for Alpha/W,
  10 bits for Blue/Z,
  10 bits for Green/Y,
  then 10 bis for Red/X.

The integer values for each component are either normalized to the 0-1 range,
    or simply casted to float.
"
struct VSInput_FVector_Packed_A2_BGR10 <: VSInput_FloatVector
    signed::Bool
    normalized::Bool
end
fvector_data_is_normalized(i::VSInput_FVector_Packed_A2_BGR10) = i.normalized
count_components(i::VSInput_FVector_Packed_A2_BGR10) = 4
vertex_data_byte_size(i::VSInput_FVector_Packed_A2_BGR10) = 4
get_component_ogl_enum(i::VSInput_FVector_Packed_A2_BGR10) = i.signed ?
                                                                 GL_INT_2_10_10_10_REV :
                                                                 GL_UNSIGNED_INT_2_10_10_10_REV


export VSInput_FVector_Plain, VSInput_FVector_FromInt, VSInput_FVector_Fixed,
       VSInput_FVector_Packed_A2_BGR10, VSInput_FVector_Packed_UF_B10_G11_R11


############################################
#         Convenience constructors         #
############################################


"Creates a 1D float- or int- or double-vector vertex input"
function VSInput(T::Type{<:Union{Float32, Float64, Integer}})
    dims = VertexInputVectorDimensions.D1
    if T == Float64
        return VSInput_DoubleVector(dims)
    elseif T <: AbstractFloat
        return VSInput_FVector_Plain(dims,
                                     E_VertexInputFloatSizes(sizeof(T)))
    elseif T <: Integer
        return VSInput_IntVector(dims,
                                 T <: Signed,
                                 E_VertexInputIntSizes(sizeof(T)))
    else
        error("Unexpected scalar type for VSInput(): ", T)
    end
end
"Creates a simple float- or int- or double-vector vertex input"
function VSInput(::Type{Vec{N, T}}) where {N, T}
    if T == Float64
        return VSInput_DoubleVector(E_VertexInputVectorDimensions(N))
    elseif T <: AbstractFloat
        return VSInput_FVector_Plain(E_VertexInputVectorDimensions(N),
                                     E_VertexInputFloatSizes(sizeof(T)))
    elseif T <: Integer
        return VSInput_IntVector(E_VertexInputVectorDimensions(N),
                                 T <: Signed,
                                 E_VertexInputIntSizes(sizeof(T)))
    else
        error("Unexpected vector type for VSInput(): ", Vec{N, T})
    end
end
"Creates a float or double matrix input"
function VSInput(::Type{<:Mat{C, R, F}}) where {C, R, F<:AbstractFloat}
    c = E_VertexInputMatrixDimensions(C)
    r = E_VertexInputMatrixDimensions(R)
    if F == Float32
        return VSInput_Matrix(r, c, VertexInputMatrixTypes.float32)
    elseif F == Float64
        return VSInput_Matrix(r, c, VertexInputMatrixTypes.float64)
    else
        error("Unsupported matrix type for VSInput(): ", Mat{C, R, F})
    end
end

"Creates a type for an integer vector that get casted or normalized into a float vector in the shader"
function VSInput_FVector(::Type{Vec{N, I}}, normalized::Bool) where {N, I<:Integer}
    return VSInput_FVector_FromInt(VSInput(Vec{N, I}), normalized)
end


VSInput_IntVector(n_dimensions::Integer, is_signed::Bool, component_byte_size::Integer) = VSInput_IntVector(
    E_VertexInputVectorDimensions(n_dimensions),
    is_signed,
    E_VertexInputIntSizes(component_byte_size)
)

VSInput_DoubleVector(n_dimensions::Integer) = VSInput_DoubleVector(
    E_VertexInputVectorDimensions(n_dimensions)
)

VSInput_FVector_Fixed(n_dimensions::Integer) = VSInput_FVector_Fixed(
    E_VertexInputVectorDimensions(n_dimensions)
)

VSInput_Matrix(n_rows::Integer, n_columns::Integer, float_byte_size::Integer) = VSInput_Matrix(
    E_VertexInputMatrixDimensions(n_rows),
    E_VertexInputMatrixDimensions(n_columns),
    E_VertexInputMatrixTypes(float_byte_size)
)
VSInput_Matrix(n_rows::Integer, n_columns::Integer, float_type::Type{<:AbstractFloat}) = VSInput_Matrix(
    E_VertexInputMatrixDimensions(n_rows),
    E_VertexInputMatrixDimensions(n_columns),
    E_VertexInputMatrixTypes(sizeof(float_type))
)

export VSInput, VSInput_FVector