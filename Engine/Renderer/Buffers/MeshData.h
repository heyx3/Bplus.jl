#pragma once

#include "Buffer.h"
#include "MeshVertexData.h"


namespace Bplus::GL::Buffers
{
    //A reference to a Buffer which contains an array of some data
    //    (presumably vertices or indices).
    struct BP_API MeshDataSource
    {
        const Buffer* Buf;
        uint32_t DataStructSize, InitialByteOffset;
    };

    //Pulls some chunk of data out of each element in a MeshDataSource.
    //The MeshDataSource is referenced by an index.
    struct BP_API VertexDataField
    {
        //The MeshDataSource this field pulls from, by its index in the list.
        size_t MeshDataSourceIndex;
        //The size of this field, in bytes.
        size_t FieldByteSize;
        //The offset of this field from the beginning of its struct, in bytes.
        size_t FieldByteOffset;
        //Describes the actual type of this field, as well as
        //    the type it appears to be in the shader.
        VertexData::Type FieldType;

        //If 0, this data is regular old per-vertex data.
        //If 1, this data is per-instance, for instanced rendering.
        //If 2, this data is per-instance and each element is reused for 2 consecutive instances.
        //If 3, this data is per-instance and each element is reused for 3 consecutive instances.
        // etc.
        uint32_t PerInstance = 0;


        bool operator==(const VertexDataField& other) const
        {
            return (MeshDataSourceIndex == other.MeshDataSourceIndex) &
                   (FieldByteSize == other.FieldByteSize) &
                   (FieldByteOffset == other.FieldByteOffset) &
                   (FieldType == other.FieldType) &
                   (PerInstance == other.PerInstance);
        }
        bool operator!=(const VertexDataField& other) const { return !operator==(other); }
    };

    //TODO: Rename VertexDataField to VertexDataFromSource, and put it in the new VertexDataField as a union with with plain vector/matrix types, allowing the MeshData to set up a constant in place of a real array of vertex data.


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

        //TODO: More methods to change mesh data (if OpenGL allows), or get/set mesh primitive type.


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
BETTER_ENUMS_DECLARE_STD_HASH(Bplus::GL::Buffers::IndexDataTypes);

//Hashes for data structures:
BP_HASHABLE_SIMPLE(Bplus::GL::Buffers::VertexDataField,
                   d.MeshDataSourceIndex, d.FieldType,
                   d.FieldByteSize, d.FieldByteOffset,
                   d.PerInstance)