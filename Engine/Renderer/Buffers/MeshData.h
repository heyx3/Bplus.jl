#pragma once

#include "Buffer.h"


namespace Bplus::GL::Buffers
{
    //A reference to a Buffer which contains an array of some data
    //    (presumably vertices or indices).
    struct BP_API MeshDataSource
    {
        const Buffer* Buf;
        uint32_t DataStructSize, InitialByteOffset;
    };


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
    namespace VertexData
    {

        //TODO: Vertex data that gets interpreted as 32-bit float matrices.
        //TODO: Vertex data that gets interpreted as 64-bit double matrices.

        //The different possible sizes of incoming vertex data.
        BETTER_ENUM(Components, uint8_t,
            X = 1,
            XY = 2,
            XYZ = 3,
            XYZW = 4
        );

        #pragma region Vertex data that gets interpreted as 32-bit float vectors

        //The different possible types of float vertex data stored in a buffer,
        //    to be interpreted as 32-bit float vector components by a shader.
        BETTER_ENUM(SimpleFloatTypes, GLenum,
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
        struct SimpleFloatType {
            Components Size;
            SimpleFloatTypes ComponentType;

            bool operator==(const SimpleFloatType& other) const {
                return (Size == other.Size) & (ComponentType == other.ComponentType);
            }
            bool operator!=(const SimpleFloatType& other) const { return !operator==(other); }
        };

        //The different possible types of integer vertex data stored in a buffer,
        //    to be interpreted as 32-bit float vector components by a shader.
        BETTER_ENUM(ConvertedFloatTypes, GLenum,
            UInt8 = GL_UNSIGNED_BYTE,
            UInt16 = GL_UNSIGNED_SHORT,
            UInt32 = GL_UNSIGNED_INT,

            Int8 = GL_BYTE,
            Int16 = GL_SHORT,
            Int32 = GL_INT
        );
        struct ConvertedFloatType {
            Components Size;
            ConvertedFloatTypes ComponentType;
            //If true, then the integer data is normalized to the range [0, 1] or [-1, 1].
            //If false, then the data is simply casted to a float.
            bool Normalize;

            bool operator==(const ConvertedFloatType& other) const {
                return (Size == other.Size) & (Normalize == other.Normalize) &
                       (ComponentType == other.ComponentType);
            }
            bool operator!=(const ConvertedFloatType& other) const { return !operator==(other); }
        };

        //The different possible types of packed float vertex data stored in the buffer,
        //    to be interpreted as vectors of 32-bit floats by a shader.
        BETTER_ENUM(PackedFloatTypes, GLenum,
            //A 4-byte uint representing a vector of 3 unsigned floats
            //    where the most significant 10 bits are the Blue/Z component,
            //    the next 11 bits are the Green/Y, then the last 11 for the Red/X.
            UFloat_B10_GR11 = GL_UNSIGNED_INT_10F_11F_11F_REV
        );

        //The different possible types of packed vertex data stored in the buffer,
        //    to be interpreted as vectors of 32-bit floats by a shader.
        BETTER_ENUM(PackedConvertedFloatTypes, GLenum,
            //A 4-byte uint reprsenting a vector of 4 unsigned integers
            //    where the most significant 2 bits are the Alpha/W component,
            //    the next 10 bits are Blue/Z, then Green/Y, then Red/X.
            UInt_A2_BGR10 = GL_UNSIGNED_INT_2_10_10_10_REV,
            //A 4-byte uint representing a vector of 4 signed integers
            //    where the most significant 2 bits are the Alpha/W component,
            //    the next 10 bits are Blue/Z, then Green/Y, then Red/X.
            Int_A2_BGR10 = GL_INT_2_10_10_10_REV
        );
        struct PackedConvertedFloatType {
            PackedConvertedFloatTypes VectorType;
            //If true, then the integer data is normalized to the range [0, 1] or [-1, 1].
            //If false, then the data is simply casted to a float.
            bool Normalize;

            bool operator==(const PackedConvertedFloatType& other) const {
                return (VectorType == other.VectorType) & (Normalize == other.Normalize);
            }
            bool operator!=(const PackedConvertedFloatType& other) const { return !operator==(other); }
        };

        #pragma endregion

        #pragma region Vertex data that gets interpreted as 32-bit int or uint vectors

        BETTER_ENUM(IntegerTypes, GLenum,
            UInt8 = GL_UNSIGNED_BYTE,
            UInt16 = GL_UNSIGNED_SHORT,
            UInt32 = GL_UNSIGNED_INT,

            Int8 = GL_BYTE,
            Int16 = GL_SHORT,
            Int32 = GL_INT
        );

        struct IntegerType {
            Components Size;
            IntegerTypes ComponentType;

            bool operator==(const IntegerType& other) const {
                return (Size == other.Size) & (ComponentType == other.ComponentType);
            }
            bool operator!=(const IntegerType& other) const { return !operator==(other); }
        };

        #pragma endregion

        //Vertex data that gets interpreted as 64-bit double vectors:
        //TODO: Replace with a simple struct.
        strong_typedef_start(DoubleType, Components)
            strong_typedef_equatable
        strong_typedef_end;


        //Some kind of vertex data coming from a Buffer,
        //    interpreted into a specific format for the mesh/shader.
        struct BP_API Type
        {
            #pragma region Constructors and getters for each type in the union

            Type(SimpleFloatType d) : data(d) { }
            bool IsSimpleFloat() const { return std::holds_alternative<SimpleFloatType>(data); }
            SimpleFloatType AsSimpleFloat() const { return std::get<SimpleFloatType>(data); }

            Type(ConvertedFloatType d) : data(d) { }
            bool IsConvertedFloat() const { return std::holds_alternative<ConvertedFloatType>(data); }
            ConvertedFloatType AsConvertedFloat() const { return std::get<ConvertedFloatType>(data); }

            Type(PackedFloatTypes d) : data(d) { }
            bool IsPackedFloat() const { return std::holds_alternative<PackedFloatTypes>(data); }
            PackedFloatTypes AsPackedFloat() const { return std::get<PackedFloatTypes>(data); }

            Type(PackedConvertedFloatType d) : data(d) { }
            bool IsPackedConvertedFloat() const { return std::holds_alternative<PackedConvertedFloatType>(data); }
            PackedConvertedFloatType AsPackedConvertedFloat() const { return std::get<PackedConvertedFloatType>(data); }

            Type(IntegerType d) : data(d) { }
            bool IsInteger() const { return std::holds_alternative<IntegerType>(data); }
            IntegerType AsInteger() const { return std::get<IntegerType>(data); }

            Type(DoubleType d) : data(d) { }
            bool IsDouble() const { return std::holds_alternative<DoubleType>(data); }
            Components AsDouble() const { return std::get<DoubleType>(data).Get(); }

            #pragma endregion

            //Gets whether this data will be seen in the shader as 32-bit floats/vectors,
            //    regardless of what format that data comes from.
            bool IsFloat() const { return IsSimpleFloat() |
                                          IsConvertedFloat() |
                                          IsPackedFloat() |
                                          IsPackedConvertedFloat(); }

            uint8_t GetNComponents() const;
            GLenum GetOglEnum() const;


            bool operator==(const Type& other) const { return data == other.data; }
            bool operator!=(const Type& other) const { return !operator==(other); }
            size_t Hash() const { return std::hash<TypeUnion>()(data); }

        private:
            using TypeUnion = std::variant<SimpleFloatType,
                                           ConvertedFloatType,
                                           PackedFloatTypes,
                                           PackedConvertedFloatType,
                                           IntegerType,
                                           DoubleType>;
            TypeUnion data;
        };
    }


    //Pulls some chunk of data out of each element in a MeshDataSource.
    //The MeshDataSource is referenced by an index.
    struct BP_API VertexDataField
    {
        size_t MeshDataSourceIndex, InitialByteOffset;
        size_t FieldByteSize, FieldByteOffset;
        VertexData::Type FieldType;

        bool operator==(const VertexDataField& other) const
        {
            return (MeshDataSourceIndex == other.MeshDataSourceIndex) &
                   (InitialByteOffset == other.InitialByteOffset) &
                   (FieldByteSize == other.FieldByteSize) &
                   (FieldByteOffset == other.FieldByteOffset) &
                   (FieldType == other.FieldType);
        }
        bool operator!=(const VertexDataField& other) const { return !operator==(other); }
    };


    //The different kinds of indices that can be used in a mesh.
    BETTER_ENUM(IndexDataTypes, GLenum,
        UInt8 = GL_UNSIGNED_BYTE,
        UInt16 = GL_UNSIGNED_SHORT,
        UInt32 = GL_UNSIGNED_INT
    );


    //A renderable 3D model, or "mesh", made from multiple data sources
    //    spread across some number of Buffers.
    //In OpenGL terms, this is a "Vertex Array Object" or "VAO".
    class BP_API MeshData
    {
    public:

        //Creates an indexed mesh.
        MeshData(const MeshDataSource& indexData, IndexDataTypes indexType,
                 const std::vector<MeshDataSource>& vertexBuffers,
                 const std::vector<VertexDataField>& vertexData)
            : MeshData(indexType, std::make_optional(indexData),
                       vertexBuffers, vertexData) { }
        //Creates a non-indexed mesh.
        MeshData(const std::vector<MeshDataSource>& vertexBuffers,
                 const std::vector<VertexDataField>& vertexData)
            : MeshData(IndexDataTypes::UInt8, std::nullopt,
                       vertexBuffers, vertexData) { }

        ~MeshData();


        //No copying, but moves are allowed.
        MeshData(const MeshData& cpy) = delete;
        MeshData& operator=(const MeshData& cpy) = delete;
        MeshData(MeshData&& src) : glPtr(src.glPtr),
                                   indexDataType(src.indexDataType),
                                   indexData(src.indexData),
                                   vertexDataSources(std::move(src.vertexDataSources)),
                                   vertexData(std::move(src.vertexData))
        {
            src.glPtr = OglPtr::Mesh::Null();
        }
        MeshData& operator=(MeshData&& src)
        {
            //Call deconstructor, then move constructor.
            //Only bother changing things if they represent different handles.
            if (this != &src)
            {
                this->~MeshData();
                new (this) MeshData (std::move(src));
            }

            return *this;
        }


        OglPtr::Mesh GetOglPtr() const { return glPtr; }

        void SetIndexData(const MeshDataSource& indexData, IndexDataTypes type);
        void RemoveIndexData();


    private:

        struct MeshDataSource_Impl
        {
            OglPtr::Buffer Buf;
            uint32_t DataStructSize, InitialByteOffset;
        };


        OglPtr::Mesh glPtr;

        IndexDataTypes indexDataType;
        std::optional<MeshDataSource_Impl> indexData;

        std::vector<MeshDataSource_Impl> vertexDataSources;
        std::vector<VertexDataField> vertexData;


        MeshData(IndexDataTypes indexType,
                 const std::optional<MeshDataSource>& indexData,
                 const std::vector<MeshDataSource>& vertexBuffers,
                 const std::vector<VertexDataField>& vertexData);
    };
}


//Hashes for BETTER_ENUM() enums:
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::Components);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::SimpleFloatTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::PackedFloatTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::ConvertedFloatTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::PackedConvertedFloatTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::VertexData::IntegerTypes);
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::IndexDataTypes);

//Hashes for strong type-defs:
strong_typedef_hashable(Bplus::GL::Buffers::VertexData::DoubleType)

//Hashes for data structures:
BP_HASHABLE_START(Bplus::GL::Buffers::VertexData::SimpleFloatType)
    return Bplus::MultiHash(d.Size, d.ComponentType);
BP_HASHABLE_END
BP_HASHABLE_START(Bplus::GL::Buffers::VertexData::ConvertedFloatType)
    return Bplus::MultiHash(d.Size, d.ComponentType, d.Normalize);
BP_HASHABLE_END
BP_HASHABLE_START(Bplus::GL::Buffers::VertexData::PackedConvertedFloatType)
    return Bplus::MultiHash(d.VectorType, d.Normalize);
BP_HASHABLE_END
BP_HASHABLE_START(Bplus::GL::Buffers::VertexData::IntegerType)
    return Bplus::MultiHash(d.Size, d.ComponentType);
BP_HASHABLE_END
BP_HASHABLE_START(Bplus::GL::Buffers::VertexData::Type)
    return d.Hash();
BP_HASHABLE_END
BP_HASHABLE_START(Bplus::GL::Buffers::VertexDataField)
    return Bplus::MultiHash(d.MeshDataSourceIndex, d.InitialByteOffset,
                            d.FieldByteSize, d.FieldByteOffset, d.Field);
BP_HASHABLE_END