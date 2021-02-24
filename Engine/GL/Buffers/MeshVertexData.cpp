#include "MeshVertexData.h"


using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Buffers;


VertexData::LogicalTypes VertexData::Type::GetLogicalType() const
{
    if (IsFMatrix() | IsSimpleFVector() | IsPackedFVector() |
        IsConvertedFVector() | IsPackedConvertedFVector())
    {
        return LogicalTypes::Float32;
    }
    else if (IsDMatrix() | IsDVector())
    {
        return LogicalTypes::Float64;
    }
    else if (IsIVector())
    {
        return LogicalTypes::S_or_U_Int32;
    }
    else
    {
        BP_ASSERT(false, "Unknown Bplus::GL::Buffers::VertexData::LogicalTypes");
        return LogicalTypes::Float32;
    }
}
VertexData::LogicalFormats VertexData::Type::GetLogicalFormat() const
{
    if (IsFMatrix() | IsDMatrix())
    {
        return LogicalFormats::Matrix;
    }
    else if (IsSimpleFVector() | IsConvertedFVector() |
             IsPackedFVector() | IsPackedConvertedFVector() |
             IsDVector() | IsIVector())
    {
        return LogicalFormats::Vector;
    }
    else
    {
        BP_ASSERT(false, "Unknown Bplus::GL::Buffers::VertexData::LogicalFormats");
        return LogicalFormats::Vector;
    }
}

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
            case PackedFVectorTypes::UFloat_B10_GR11:
                return VectorSizes::XYZ;
            default: {
                std::string errMsg = "Unexpected Bplus::GL::Buffers::VertexData::PackedFVectorTypes ";
                errMsg += AsPackedFVector()._to_string();
                BP_ASSERT(false, errMsg.c_str());
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
                BP_ASSERT(false, errMsg.c_str());
                return VectorSizes::X;
            }
        }
    else if (IsIVector())
        return AsIVector().Size;
    else if (IsDVector())
        return AsDVector().Size;
    else
    {
        BP_ASSERT(false, "Unknown Bplus::GL::Buffers::VertexData::Type case");
        return VectorSizes::X;
    }
}
uint8_t VertexData::Type::GetNAttributes() const
{
    if (IsFMatrix())
        return AsFMatrix().ColSize;
    else if (IsDMatrix())
        return AsDMatrix().ColSize;
    else if (GetLogicalFormat() == +LogicalFormats::Vector)
        return 1;
    else
    {
        BP_ASSERT(false, "Unknown Bplus::GL::Buffers::VertexData::Type case");
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
        BP_ASSERT(false, "Unknown Bplus::GL::Buffers::VertexData::Type case");
        return GL_NONE;
    }
}