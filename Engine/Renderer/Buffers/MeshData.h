#pragma once

#include "Buffer.h"
#include "MeshVertexData.h"

//TODO: Add a "RunPass" struct that provides all the data and behavior for actually executing a render pass.


namespace Bplus::GL::Buffers
{
    //A reference to a Buffer which contains an array of vertices or indices.
    struct BP_API MeshDataSource
    {
        const Buffer* Buf;
        //The byte size of a single element in the array.
        uint32_t DataStructSize;
        //The byte offset into the beginning of the buffer
        //    for where the vertex/index data starts.
        size_t InitialByteOffset;

        //Gets the maximum number of elements available for the mesh to pull from.
        size_t GetMaxNElements() const {
            size_t nBytes = Buf->GetByteSize() - InitialByteOffset;
            return nBytes / DataStructSize;
        }
    };

    //Pulls some chunk of data (usually a vector of floats) out of each element
    //    in a MeshDataSource.
    struct BP_API VertexDataField
    {
        //The buffer this field pulls from, as its index in a list of MeshDataSources.
        size_t MeshDataSourceIndex;
        //The size of this field, in bytes.
        //For example, a vec3 field would be "3*sizeof(float)".
        size_t FieldByteSize;
        //The offset of this field from the beginning of its struct, in bytes.
        //For example, the offset of "Pos" in an array of "struct Vertex{ vec4 Color, vec3 Pos }"
        //    is "offsetof(Vertex, Pos)" (i.e. "4*sizeof(float)").
        size_t FieldByteOffset;
        //Describes the actual type of this field in the buffer, as well as
        //    the type it appears as in the shader.
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
    uint_fast8_t GetByteSize(IndexDataTypes d) { switch (d) {
        case IndexDataTypes::UInt8: return 1;
        case IndexDataTypes::UInt16: return 2;
        case IndexDataTypes::UInt32: return 4;
        default: BPAssert(false, d._to_string()); return 0;
    } }

    //The different kinds of shapes that a mesh can be built from.
    BETTER_ENUM(PrimitiveTypes, GLenum,
        //Each vertex is a screen-space square.
        Point = GL_POINTS,
        //Each pair of vertices is a line.
        //If an extra vertex is at the end of the mesh, it's ignored.
        Line = GL_LINES,
        //Each triplet of vertices is a triangle.
        //If one or two extra vertices are at the end of the mesh, they're ignored.
        Triangle = GL_TRIANGLES,

        //Each vertex creates a line reaching forward to the next vertex.
        //If there's only one vertex, no lines are created.
        LineStripOpen = GL_LINE_STRIP,
        //Each vertex creates a line reaching forward to the next vertex.
        //The last vertex reaches back to the first vertex, creating a closed loop.
        //If there's only one vertex, no lines are created.
        LineStripClosed = GL_LINE_LOOP,

        //Each new vertex creates a triangle with its two previous vertices.
        //If there's only one or two vertices, no triangles are created.
        TriangleStrip = GL_TRIANGLE_STRIP,
        //Each new vertex creates a triangle with its previous vertex plus the first vertex.
        //If there's only one or two vertices, no triangles are created.
        TriangleFan = GL_TRIANGLE_FAN
    );


    //A renderable 3D model, or "mesh", made from multiple data sources
    //    spread across some number of Buffers.
    //In OpenGL terms, this is a "Vertex Array Object" or "VAO".
    class BP_API MeshData
    {
    public:

        PrimitiveTypes PrimitiveType;


        //Creates an indexed mesh.
        MeshData(PrimitiveTypes type,
                 const MeshDataSource& indexData, IndexDataTypes indexType,
                 const std::vector<MeshDataSource>& vertexBuffers,
                 const std::vector<VertexDataField>& vertexData)
            : MeshData(type, indexType, std::make_optional(indexData),
                       vertexBuffers, vertexData) { }
        //Creates a non-indexed mesh.
        MeshData(PrimitiveTypes type,
                 const std::vector<MeshDataSource>& vertexBuffers,
                 const std::vector<VertexDataField>& vertexData)
            : MeshData(type, IndexDataTypes::UInt8, std::nullopt,
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

        bool HasIndexData() const { return indexData.has_value(); }
        std::optional<MeshDataSource> GetIndexData() const;
        std::optional<IndexDataTypes> GetIndexDataType() const;

        void GetVertexData(std::vector<MeshDataSource>& outSources,
                           std::vector<VertexDataField>& outData) const;


        void SetIndexData(const MeshDataSource& indexData, IndexDataTypes type);
        void RemoveIndexData();

        //TODO: More methods to change mesh data


    private:

        //Internally, Buffers are stored by their OpenGL pointer,
        //    so that they aren't tied to a specific location in memory
        //    (otherwise we could get undefined behavior when e.x. an STL container moves the Buffer).
        //The Buffer class provides a static function to get a buffer by its ID, so this is fine.

        struct MeshDataSource_Impl
        {
            OglPtr::Buffer Buf;
            uint32_t DataStructSize;
            size_t InitialByteOffset;
        };


        OglPtr::Mesh glPtr;

        IndexDataTypes indexDataType;
        std::optional<MeshDataSource_Impl> indexData;

        std::vector<MeshDataSource_Impl> vertexDataSources;
        std::vector<VertexDataField> vertexData;


        MeshData(PrimitiveTypes primType, IndexDataTypes indexType,
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