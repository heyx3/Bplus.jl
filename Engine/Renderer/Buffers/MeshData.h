#pragma once

#include "Buffer.h"


namespace Bplus::GL::Buffers
{
    //A reference to a Buffer which contains an array of some data.
    struct BP_API MeshDataSource
    {
        const Buffer* Buf;
        size_t DataStructSize, InitialByteOffset;

        MeshDataSource() { }
        MeshDataSource(const Buffer* buf, size_t structSize,
                       size_t initialByteOffset = 0)
            : Buf(buf), DataStructSize(structSize),
              InitialByteOffset(initialByteOffset) { }
    };


    //Pulls some chunk of data out of each element in a MeshDataSource.
    //The MeshDataSource is referenced by an index.
    struct BP_API VertexDataField
    {
        size_t MeshDataSourceIndex;
        size_t FieldByteSize, FieldByteOffset;


        VertexDataField() { }

        VertexDataField(size_t meshDataSrcIndex,
                        size_t fieldByteSize, size_t fieldByteOffset)
            : MeshDataSourceIndex(meshDataSrcIndex),
              FieldByteSize(fieldByteSize),
              FieldByteOffset(fieldByteOffset) { }
    };


    //The different kinds of indices that can be used in a mesh.
    BETTER_ENUM(IndexDataTypes, GLenum,
        UInt8 = GL_UNSIGNED_BYTE,
        UInt16 = GL_UNSIGNED_SHORT,
        UInt32 = GL_UNSIGNED_INT
    );


    //Multiple buffers combined together to represent a renderable 3D model, or "mesh".
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
                                   vertexDataSrc(std::move(src.vertexDataSrc)),
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