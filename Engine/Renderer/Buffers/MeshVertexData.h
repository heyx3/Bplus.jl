#pragma once

#include "Buffer.h"

//Vertex data coming from a Buffer can take numerous forms,
//    and appears in the mesh/shader as one of a handful of different data types.
//The types of vertex data that can be put into a mesh are:
//    * 1D - 4D vector of 32-bit float
//       : Coming from float/fixed-point data, requiring minimal conversion
//       : Coming from int/uint data, directly casted to 32-bit float
//       : Coming from int/uint data, normalized to 32-bit float (similar to normalized-integer textures)
//       : Coming from pre-packed data formats, like UInt_RGB10_A2 or UFloat_R11_G11_B10,
//             requiring various amounts of conversion
//    * 1D - 4D vector of 32-bit int or uint
//       : Coming from int/uint data, requiring minimal conversion
//    * 1D - 4D vector of 64-bit doubles
//       : Coming directly from 64-bit double data, no conversion needed

namespace Bplus::GL::Buffers::VertexData
{
    BETTER_ENUM(VectorSizes, uint8_t,
        X = 1,
        XY = 2,
        XYZ = 3,
        XYZW = 4
    );


    //Vertex data that gets interpreted as float or double matrices.
    template<typename TFloat>
    struct MatrixType
    {
        VectorSizes RowSize, ColSize;

        bool operator==(const MatrixType<TFloat>& other) const
        {
            return (RowSize == other.RowSize) &
                   (ColSize == other.ColSize);
        }
        bool operator!=(const MatrixType<TFloat>& other) const { return !operator==(other); }
    };
    using FMatrixType = MatrixType<float>;
    using DMatrixType = MatrixType<double>;


    #pragma region Vertex data that gets interpreted as 32-bit float vectors

    //The different possible types of float vertex data stored in a buffer,
    //    to be interpreted as 32-bit float vector components by a shader.
    BETTER_ENUM(SimpleFVectorTypes, GLenum,
        Float16 = GL_HALF_FLOAT,
        Float32 = GL_FLOAT,
                
        //This format is not recommended,
        //    since this data is getting converted into 32-bit floats anyway.
        Float64 = GL_DOUBLE,

        //A fixed-point decimal value, with 16 bits for the integer part
        //    and 16 bits for the decimal part.
        //TODO: What is the byte ordering for this?
        //This format is not recommended,
        //    since this data is getting converted into 32-bit floats anyway.
        Fixed32 = GL_FIXED
    );
    struct SimpleFVectorType
    {
        VectorSizes Size;
        SimpleFVectorTypes ComponentType;

        bool operator==(const SimpleFVectorType& other) const {
            return (Size == other.Size) & (ComponentType == other.ComponentType);
        }
        bool operator!=(const SimpleFVectorType& other) const { return !operator==(other); }
    };

    //The different possible types of integer vertex data stored in a buffer,
    //    to be interpreted as 32-bit float vector components by a shader.
    BETTER_ENUM(ConvertedFVectorTypes, GLenum,
        UInt8 = GL_UNSIGNED_BYTE,
        UInt16 = GL_UNSIGNED_SHORT,
        UInt32 = GL_UNSIGNED_INT,

        Int8 = GL_BYTE,
        Int16 = GL_SHORT,
        Int32 = GL_INT
    );
    struct ConvertedFVectorType
    {
        VectorSizes Size;
        ConvertedFVectorTypes ComponentType;
        //If true, then the integer data is normalized to the range [0, 1] or [-1, 1].
        //If false, then the data is simply casted to a float.
        bool Normalize;

        bool operator==(const ConvertedFVectorType& other) const {
            return (Size == other.Size) & (Normalize == other.Normalize) &
                   (ComponentType == other.ComponentType);
        }
        bool operator!=(const ConvertedFVectorType& other) const { return !operator==(other); }
    };

    //The different possible types of packed float vertex data stored in the buffer,
    //    to be interpreted as vectors of 32-bit floats by a shader.
    BETTER_ENUM(PackedFVectorTypes, GLenum,
        //A 4-byte uint representing a vector of 3 unsigned floats
        //    where the most significant 10 bits are the Blue/Z component,
        //    the next 11 bits are the Green/Y, then the last 11 for the Red/X.
        UFloat_B10_GR11 = GL_UNSIGNED_INT_10F_11F_11F_REV
    );

    //The different possible types of packed vertex data stored in the buffer,
    //    to be interpreted as vectors of 32-bit floats by a shader.
    BETTER_ENUM(PackedConvertedFVectorTypes, GLenum,
        //A 4-byte uint reprsenting a vector of 4 unsigned integers
        //    where the most significant 2 bits are the Alpha/W component,
        //    the next 10 bits are Blue/Z, then Green/Y, then Red/X.
        UInt_A2_BGR10 = GL_UNSIGNED_INT_2_10_10_10_REV,
        //A 4-byte uint representing a vector of 4 signed integers
        //    where the most significant 2 bits are the Alpha/W component,
        //    the next 10 bits are Blue/Z, then Green/Y, then Red/X.
        Int_A2_BGR10 = GL_INT_2_10_10_10_REV
    );
    struct PackedConvertedFVectorType
    {
        PackedConvertedFVectorTypes VectorType;
        //If true, then the integer data is normalized to the range [0, 1] or [-1, 1].
        //If false, then the data is simply casted to a float.
        bool Normalize;

        bool operator==(const PackedConvertedFVectorType& other) const {
            return (VectorType == other.VectorType) & (Normalize == other.Normalize);
        }
        bool operator!=(const PackedConvertedFVectorType& other) const { return !operator==(other); }
    };

    #pragma endregion

    #pragma region Vertex data that gets interpreted as 32-bit int or uint vectors

    BETTER_ENUM(IVectorTypes, GLenum,
        UInt8 = GL_UNSIGNED_BYTE,
        UInt16 = GL_UNSIGNED_SHORT,
        UInt32 = GL_UNSIGNED_INT,

        Int8 = GL_BYTE,
        Int16 = GL_SHORT,
        Int32 = GL_INT
    );

    struct IVectorType
    {
        VectorSizes Size;
        IVectorTypes ComponentType;

        bool operator==(const IVectorType& other) const {
            return (Size == other.Size) & (ComponentType == other.ComponentType);
        }
        bool operator!=(const IVectorType& other) const { return !operator==(other); }
    };

    #pragma endregion

    //Vertex data that gets interpreted as 64-bit double vectors:
    struct DVectorType
    {
        VectorSizes Size;

        bool operator==(const DVectorType& other) const { return Size == other.Size; }
        bool operator!=(const DVectorType& other) const { return !operator==(other); }
    };


    //The types of data that can be in a mesh, from the shader's point of view.
    BETTER_ENUM(LogicalTypes, uint8_t,
        Float32,
        Float64,

        //Signed OR unsigned integer (it could appear as either depending on the shader).
        S_or_U_Int32
    );
    //The formats that mesh data can appear in from a shader's point of view
    //    (i.e. Vector or Matrix).
    BETTER_ENUM(LogicalFormats, uint8_t, Vector, Matrix);

}

#pragma region Add hashing support for the above types

//Hashes for BETTER_ENUM() enums:
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::VectorSizes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::SimpleFVectorTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::PackedFVectorTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::ConvertedFVectorTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::PackedConvertedFVectorTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::IVectorTypes);

//Hashes for data structures:
BP_HASHABLE_SIMPLE_FULL(typename T, Bplus::GL::Buffers::VertexData::MatrixType<T>,
                        d.RowSize, d.ColSize)
BP_HASHABLE_SIMPLE(Bplus::GL::Buffers::VertexData::SimpleFVectorType,
                   d.Size, d.ComponentType)
BP_HASHABLE_SIMPLE(Bplus::GL::Buffers::VertexData::ConvertedFVectorType,
                   d.Size, d.ComponentType, d.Normalize)
BP_HASHABLE_SIMPLE(Bplus::GL::Buffers::VertexData::PackedConvertedFVectorType,
                   d.VectorType, d.Normalize)
BP_HASHABLE_SIMPLE(Bplus::GL::Buffers::VertexData::IVectorType,
                   d.Size, d.ComponentType)
BP_HASHABLE_SIMPLE(Bplus::GL::Buffers::VertexData::DVectorType,
                   d.Size)

#pragma endregion

namespace Bplus::GL::Buffers::VertexData
{
    //Some kind of vertex data coming from a Buffer,
    //    interpreted into a specific format for the mesh/shader.
    struct BP_API Type
    {
        #pragma region Constructors and getters for each type in the union

        Type(FMatrixType d) : data(d) { }
        bool IsFMatrix() const { return std::holds_alternative<FMatrixType>(data); }
        FMatrixType AsFMatrix() const { return std::get<FMatrixType>(data); }

        Type(DMatrixType d) : data(d) { }
        bool IsDMatrix() const { return std::holds_alternative<DMatrixType>(data); }
        DMatrixType AsDMatrix() const { return std::get<DMatrixType>(data); }

        Type(SimpleFVectorType d) : data(d) { }
        bool IsSimpleFVector() const { return std::holds_alternative<SimpleFVectorType>(data); }
        SimpleFVectorType AsSimpleFVector() const { return std::get<SimpleFVectorType>(data); }

        Type(ConvertedFVectorType d) : data(d) { }
        bool IsConvertedFVector() const { return std::holds_alternative<ConvertedFVectorType>(data); }
        ConvertedFVectorType AsConvertedFVector() const { return std::get<ConvertedFVectorType>(data); }

        Type(PackedFVectorTypes d) : data(d) { }
        bool IsPackedFVector() const { return std::holds_alternative<PackedFVectorTypes>(data); }
        PackedFVectorTypes AsPackedFVector() const { return std::get<PackedFVectorTypes>(data); }

        Type(PackedConvertedFVectorType d) : data(d) { }
        bool IsPackedConvertedFVector() const { return std::holds_alternative<PackedConvertedFVectorType>(data); }
        PackedConvertedFVectorType AsPackedConvertedFVector() const { return std::get<PackedConvertedFVectorType>(data); }

        Type(IVectorType d) : data(d) { }
        bool IsIVector() const { return std::holds_alternative<IVectorType>(data); }
        IVectorType AsIVector() const { return std::get<IVectorType>(data); }

        Type(DVectorType d) : data(d) { }
        bool IsDVector() const { return std::holds_alternative<DVectorType>(data); }
        DVectorType AsDVector() const { return std::get<DVectorType>(data); }

        #pragma endregion

        LogicalTypes GetLogicalType() const;
        LogicalFormats GetLogicalFormat() const;

        //Gets the number of components in this type.
        //For a vector, this is its size.
        //For a matrix, this is the number of rows it has.
        VectorSizes GetNComponents() const;
        //Gets the number of individual vertex attributes that will be needed
        //    to represent this type.
        //For a vector, this is 1.
        //For a matrix, this is the number of columns it has.
        uint8_t GetNAttributes() const;
        //Gets the OpenGL enum value representing this type.
        GLenum GetOglEnum() const;


        bool operator==(const Type& other) const { return data == other.data; }
        bool operator!=(const Type& other) const { return !operator==(other); }
        size_t GetHash() const { using std::hash; return hash<TypeUnion>()(data); }

    private:
        using TypeUnion = std::variant<FMatrixType,
                                       DMatrixType,
                                       SimpleFVectorType,
                                       ConvertedFVectorType,
                                       PackedFVectorTypes,
                                       PackedConvertedFVectorType,
                                       IVectorType,
                                       DVectorType>;
        TypeUnion data;
    };
}
BP_HASHABLE_START(Bplus::GL::Buffers::VertexData::Type)
    return d.GetHash();
BP_HASHABLE_END