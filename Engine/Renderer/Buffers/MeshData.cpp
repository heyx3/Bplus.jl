#include "MeshData.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Buffers;

//TODO: Add Debug-mode code to ensure buffers aren't destroyed before the MeshData instance.

MeshData::MeshData(IndexDataTypes _indexType,
                   const std::optional<MeshDataSource>& _indexData,
                   const std::vector<MeshDataSource>& _vertexBuffers,
                   const std::vector<VertexDataField>& _vertexData)
    : glPtr(GlCreate(glCreateVertexArrays)),
      indexDataType(_indexType), vertexData(_vertexData)
{
    //Set the vertex and index data sources.
    if (_indexData.has_value())
    {
        indexData = MeshDataSource_Impl{ _indexData.value().Buf->GetOglPtr(),
                                         _indexData.value().DataStructSize,
                                         _indexData.value().InitialByteOffset };
    }
    for (const auto& vertexBufferSrc : _vertexBuffers)
    {
        vertexDataSources.push_back({ vertexBufferSrc.Buf->GetOglPtr(),
                                      vertexBufferSrc.DataStructSize,
                                      vertexBufferSrc.InitialByteOffset });
    }

    //Configure the vertex and index buffers in OpenGL.
    if (indexData.has_value())
    {
        glVertexArrayElementBuffer(glPtr.Get(), indexData.value().Buf.Get());
    }
    //Configure the vertex buffer in OpenGL.
    for (size_t i = 0; i < vertexDataSources.size(); ++i)
    {
        glVertexArrayVertexBuffer(glPtr.Get(), i,
                                  vertexDataSources[i].Buf.Get(),
                                  (ptrdiff_t)vertexDataSources[i].InitialByteOffset,
                                  (GLsizei)vertexDataSources[i].DataStructSize);
    }
    for (size_t i = 0; i < vertexData.size(); ++i)
        glEnableVertexArrayAttrib(glPtr.Get(), i);
    for (const auto& vInput : vertexData)
        glVertexArrayAttribFormat(); //TODO: Finish.
    for (size_t i = 0; i < vertexData.size(); ++i)
        glVertexArrayAttribBinding(glPtr.Get(), i, (GLuint)vertexData[i].MeshDataSourceIndex);
}

MeshData::~MeshData()
{
    if (!glPtr.IsNull())
        glDeleteVertexArrays(1, &glPtr.Get());
}

//TODO: Implement other methods.