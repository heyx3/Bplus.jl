#include "MeshData.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Buffers;

//TODO: Add Debug-mode code to ensure buffers aren't destroyed before the MeshData instance.

VertexData::VectorSizes VertexData::Type::GetNComponents() const
{
    if (IsFMatrix())
        return AsFMatrix().RowSize;
    else if (IsDMatrix())
        return AsDMatrix().RowSize;
    else if (IsSimpleFVector())
        return AsSimpleFVector().Size;
    else if (IsConvertedFVector())
        return AsConvertedFVector().Size;
    else if (IsPackedFVector())
        switch (AsPackedFVector()) {
            case PackedFloatTypes::UFloat_B10_GR11:
                return VectorSizes::XYZ;
            default: {
                std::string errMsg = "Unexpected Bplus::GL::Buffers::VertexData::PackedFloatTypes ";
                errMsg += AsPackedFVector()._to_string();
                BPAssert(false, errMsg.c_str());
                return VectorSizes::X;
            }
        }
    else if (IsPackedConvertedFVector())
        switch (AsPackedConvertedFVector().VectorType) {
            case PackedConvertedFVectorTypes::UInt_A2_BGR10:
            case PackedConvertedFVectorTypes::Int_A2_BGR10:
                return VectorSizes::XYZW;
            default: {
                std::string errMsg = "Unexpected Bplus::GL::Buffers::VertexData::"
                                        "PackedConvertedFVectorTypes ";
                errMsg += AsPackedConvertedFVector().VectorType._to_string();
                BPAssert(false, errMsg.c_str());
                return VectorSizes::X;
            }
        }
    else if (IsIVector())
        return AsIVector().Size;
    else if (IsDVector())
        return AsDVector().Size;
    else
    {
        BPAssert(false, "Unknown Bplus::GL::Buffers::VertexData::Type case");
        return VectorSizes::X;
    }
}
uint8_t VertexData::Type::GetNAttributes() const
{
    if (IsFMatrix())
        return AsFMatrix().ColSize;
    else if (IsDMatrix())
        return AsDMatrix().ColSize;
    else if (IsFloatVector() | IsIVector() | IsDVector())
        return 1;
    else
    {
        BPAssert(false, "Unknown Bplus::GL::Buffers::VertexData::Type case");
        return 255;
    }
}
GLenum VertexData::Type::GetOglEnum() const
{
    if (IsFMatrix())
        return GL_FLOAT;
    else if (IsDMatrix())
        return GL_DOUBLE;
    else if (IsSimpleFVector())
        return AsSimpleFVector().ComponentType._to_integral();
    else if (IsConvertedFVector())
        return AsConvertedFVector().ComponentType._to_integral();
    else if (IsPackedFVector())
        return AsPackedFVector()._to_integral();
    else if (IsPackedConvertedFVector())
        return AsPackedConvertedFVector().VectorType._to_integral();
    else if (IsIVector())
        return AsIVector().ComponentType._to_integral();
    else if (IsDVector())
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
    //TODO: Can the below be done in one loop instead of three?
    for (size_t i = 0; i < vertexData.size(); ++i)
        glEnableVertexArrayAttrib(glPtr.Get(), i);
    size_t vertAttribI = 0;
    for (size_t i = 0; i < vertexData.size(); ++i)
    {
        GLuint relativeOffset = vertexData[i].; //TODO: How?

        //TODO: Do double vectors/matrices take up twice as many attrib slots as floats? Currently we assume they don't.
        auto fieldType = vertexData[i].FieldType;
        if (fieldType.IsIVector())
        {
            glVertexArrayAttribIFormat(glPtr.Get(), vertAttribI,
                                       fieldType.AsIVector().Size._to_integral(),
                                       fieldType.GetOglEnum(),
                                       relativeOffset);
        }
        else if (fieldType.IsDVector())
        {
            glVertexArrayAttribLFormat(glPtr.Get(), vertAttribI,
                                       fieldType.AsDVector().Size._to_integral(),
                                       fieldType.GetOglEnum(),
                                       relativeOffset);
        }
        else if (fieldType.IsFMatrix())
        {
            for (size_t attr = 0; attr < fieldType.GetNAttributes(); ++attr)
            {
                glVertexArrayAttribFormat(glPtr.Get(), vertAttribI,
                                          fieldType.GetNComponents(), fieldType.GetOglEnum(),
                                          GL_FALSE, relativeOffset);
                vertAttribI += 1;
            }
        }
        else if (fieldType.IsDMatrix())
        {
            for (size_t attr = 0; attr < fieldType.GetNAttributes(); ++attr)
            {
                glVertexArrayAttribLFormat(glPtr.Get(), vertAttribI,
                                           fieldType.GetNComponents(), fieldType.GetOglEnum(),
                                           relativeOffset);
                vertAttribI += 1;
            }
        }
        else //Must be an FVector.
        {
            BPAssert(fieldType.IsFloatVector(), "FieldType isn't known");
            glVertexArrayAttribFormat(glPtr.Get(), vertAttribI,
                                      fieldType.GetNComponents(), fieldType.GetOglEnum(),
                                      (fieldType.IsConvertedFVector() &&
                                           fieldType.AsConvertedFVector().Normalize) |
                                        (fieldType.IsPackedConvertedFVector() &&
                                         fieldType.AsPackedConvertedFVector().Normalize),
                                      relativeOffset);
        }

        vertAttribI += 1;
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