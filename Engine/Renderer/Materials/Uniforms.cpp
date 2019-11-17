#include "Uniforms.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::UniformDataStructures;

bool UniformUnion_Vector::IsValidType(UniformTypes type)
{
    switch (type)
    {
    #define T_CASES(prefix) \
            case UniformTypes::prefix##1: \
            case UniformTypes::prefix##2: \
            case UniformTypes::prefix##3: \
            case UniformTypes::prefix##4

        T_CASES(Bool):
        T_CASES(Int):
        T_CASES(Uint):
        T_CASES(Float):
        T_CASES(Double):
            return true;

        default:
            return false;

    #undef T_CASES
    }
}
bool UniformUnion_Matrix::IsValidType(UniformTypes type)
{
    switch (type)
    {
    #define T_CASES(fType, nCols) \
            case UniformTypes::fType##nCols##x##2: \
            case UniformTypes::fType##nCols##x##3: \
            case UniformTypes::fType##nCols##x##4
    #define T_CASES2(fType) \
            T_CASES(fType, 2): \
            T_CASES(fType, 3): \
            T_CASES(fType, 4)
        
        T_CASES2(Float) :
        T_CASES2(Double) :
            return true;

        default:
            return false;
    }
}
bool UniformUnion_Texture::IsValidType(UniformTypes type)
{
    switch (type)
    {
        case UniformTypes::Sampler:
        case UniformTypes::Image:
            return true;

        default:
            return false;
    }
}