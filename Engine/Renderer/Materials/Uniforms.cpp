#include "Uniforms.h"

//Provides a way to access a matrix/vector as an array of floats.
#include <glm/gtc/type_ptr.hpp>


using namespace Bplus;
using namespace Bplus::GL;

#pragma region 
#if 0

void UniformArray::Remove(size_t index)
{
    BPAssert(index < count, //Both are unsigned, so this implies count > 0 as well.
             (std::string("Trying to remove element i:") + std::to_string(index) +
                  " from an array with only " + std::to_string(count) + " elements").c_str());

    count -= 1;

    auto stride = GetUniformByteSize(type);
    auto byteIndex = index * stride,
            nextByteIndex = byteIndex + stride;

    arrayBytes.erase(arrayBytes.begin() + byteIndex,
                        arrayBytes.begin() + nextByteIndex);
}
void UniformArray::Clear(UniformTypes newType)
{
    count = 0;
    arrayBytes.clear();

    type = newType;
}

void UniformArray::AssertIndex(size_t i) const
{
    BPAssert(count < i,
             (std::string("Array has ") + std::to_string(count) +
                 " elements but tried to access element " + std::to_string(i)).c_str());
}
void UniformArray::AssertType(UniformTypes expected) const
{
    BPAssert(type == expected,
             (std::string("Expected uniform array to be ") +
                 expected._to_string() + ", but it's " + type._to_string()).c_str());
}

void Bplus::GL::SetUniform(Ptr::ShaderUniform ptr, const VectorUniform& value)
{
    switch (value.GetType())
    {
        //Does the case for a specific uniform data type.
    #define DO_CASE(enumVal, glDataType, glmDataType, n, getDataElements) \
        case UniformTypes::enumVal: \
            for (size_t i = 0; i < value.GetCount(); ++i) \
            { \
                auto data = value.Get<glm::glmDataType##n>(i); \
                glUniform##n##glDataType((GLint)ptr + i, getDataElements); \
            } \
        break

        //Does the cases for 1D - 4D vectors of a specific primitive data type.
    #define DO_CASES4(enumPrefix, glDataType, glmDataType) \
        DO_CASE(enumPrefix##1, glDataType, glmDataType, 1, data.x); \
        DO_CASE(enumPrefix##2, glDataType, glmDataType, 2, data.x BP_COMMA data.y); \
        DO_CASE(enumPrefix##3, glDataType, glmDataType, 3, \
                data.x BP_COMMA data.y BP_COMMA data.z); \
        DO_CASE(enumPrefix##4, glDataType, glmDataType, 4, \
                data.x BP_COMMA data.y BP_COMMA data.z BP_COMMA data.w)

        //Use the macros to define all cases.
        DO_CASES4(Bool, ui, bvec);
        DO_CASES4(Int, i, ivec);
        DO_CASES4(Uint, ui, uvec);
        DO_CASES4(Float, f, fvec);
        DO_CASES4(Double, d, dvec);

        #undef DO_CASES4
        #undef DO_CASE

        default:
            std::string msg = "Unknown vector uniform type '";
            msg += value.GetType()._to_string();
            msg += "'";
            BPAssert(false, msg.c_str());
        break;
    }
}
void Bplus::GL::SetUniform(Ptr::ShaderUniform ptr, const MatrixUniform& value)
{
    switch (value.GetType())
    {
        //Defines the switch case for a specific matrix.
    #define DO_CASE(enumType, glmType, glType) \
        case UniformTypes::enumType: \
            for (size_t i = 0; i < value.GetCount(); ++i) \
            { \
                auto data = value.Get<glm::glmType>(i); \
                glUniformMatrix##glType##v((GLint)ptr + i, 1, GL_FALSE, glm::value_ptr(data)); \
            } \
        break

        //Defines two switch cases, for the Float and Double versions of a matrix.
    #define DO_CASES2(sizeFull, sizeMinimized) \
        DO_CASE(Float##sizeFull, fmat##sizeFull, sizeMinimized##f); \
        DO_CASE(Double##sizeFull, dmat##sizeFull, sizeMinimized##d)

        //Use the macros to define all cases.
        DO_CASES2(2x2, 2); DO_CASES2(2x3, 2x3); DO_CASES2(2x4, 2x4);
        DO_CASES2(3x2, 3x2); DO_CASES2(3x3, 3); DO_CASES2(3x4, 3x4);
        DO_CASES2(4x2, 4x2); DO_CASES2(4x3, 4x3); DO_CASES2(4x4, 4);

    #undef DO_CASES2
    #undef DO_CASE

        default:
            std::string msg = "Unknown matrix uniform type '";
            msg += value.GetType()._to_string();
            msg += "'";
            BPAssert(false, msg.c_str());
        break;
    }
}
void Bplus::GL::SetUniform(Ptr::ShaderUniform ptr, const TextureUniform& value)
{
    switch (value.GetType())
    {
        case UniformTypes::Sampler:
            TODO: How to handle active texture slots
    }
}

#endif