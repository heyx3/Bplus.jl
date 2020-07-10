#include "MeshData.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Buffers;

//TODO: Add Debug-mode code to ensure buffers aren't destroyed before the MeshData instance.

uint8_t VertexData::Type::GetNComponents() const
{
    if (IsSimpleFloat())
        return AsSimpleFloat().Size._to_integral();
    else if (IsConvertedFloat())
        return AsConvertedFloat().Size._to_integral();
    else if (IsPackedFloat())
        switch (AsPackedFloat()) {
            case PackedFloatTypes::UFloat_B10_GR11:
                return 3;
            default: {
                std::string errMsg = "Unexpected Bplus::GL::Buffers::VertexData::PackedFloatTypes ";
                errMsg += AsPackedFloat()._to_string();
                BPAssert(false, errMsg.c_str());
                return 254;
            }
        }
    else if (IsPackedConvertedFloat())
        switch (AsPackedConvertedFloat().VectorType) {
            case PackedConvertedFloatTypes::UInt_A2_BGR10:
            case PackedConvertedFloatTypes::Int_A2_BGR10:
                return 4;
            default: {
                std::string errMsg = "Unexpected Bplus::GL::Buffers::VertexData::"
                                        "PackedConvertedFloatTypes ";
                errMsg += AsPackedConvertedFloat().VectorType._to_string();
                BPAssert(false, errMsg.c_str());
                return 253;
            }
        }
    else if (IsInteger())
        return AsInteger().Size._to_integral();
    else if (IsDouble())
        return AsDouble()._to_integral();
    else
    {
        BPAssert(false, "Unknown Bplus::GL::Buffers::VertexData::Type case");
        return 255;
    }
}

GLenum VertexData::Type::GetOglEnum() const
{
    if (IsSimpleFloat())
        return AsSimpleFloat().ComponentType._to_integral();
    else if (IsConvertedFloat())
        return AsConvertedFloat().ComponentType._to_integral();
    else if (IsPackedFloat())
        return AsPackedFloat()._to_integral();
    else if (IsPackedConvertedFloat())
        return AsPackedConvertedFloat().VectorType._to_integral();
    else if (IsInteger())
        return AsInteger().ComponentType._to_integral();
    else if (IsDouble())
        return GL_DOUBLE;
    else
    {
        BPAssert(false, "Unknown Bplus::GL::Buffers::VertexData::Type case");
        return GL_NONE;
    }
}


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
    //TODO: Can the below be done in one loop?
    for (size_t i = 0; i < vertexData.size(); ++i)
        glEnableVertexArrayAttrib(glPtr.Get(), i);
    for (size_t i = 0; i < vertexData.size(); ++i)
    {
        if (vertexData[i].FieldType.IsInteger)
        {
            glVertexArrayAttribIFormat(glPtr.Get(), i,
                                       vertexData[i].FieldType.GetNComponents(),
                                       vertexData[i].FieldType.GetOglEnum(),
                                       vertexData[i],);
        }
        glVertexArrayAttribFormat(glPtr.Get(), i, vertexData[i].FieldType);
    }
    for (size_t i = 0; i < vertexData.size(); ++i)
        glVertexArrayAttribBinding(glPtr.Get(), i, (GLuint)vertexData[i].MeshDataSourceIndex);
}

MeshData::~MeshData()
{
    if (!glPtr.IsNull())
        glDeleteVertexArrays(1, &glPtr.Get());
}

//TODO: Implement other methods.