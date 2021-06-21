#include "DataStructures.h"

#include <unordered_set>

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Uniforms;


namespace
{
    void VisitUniform(std::function<void(const std::string&, const Type&)> func,
                      const std::string& uName,
                      const Type& uType,
                      const Definitions& defs,
                      std::unordered_set<std::string>& usedStructs,
                      bool iterateSimpleArrayElements,
                      bool iterateArrays = false)
    {
        BP_ASSERT(!iterateArrays || !uType.IsArray(),
                  "Passed the 'iterate array' flag on a uniform that isn't an array");

        //If this is a new array uniform, iterate its elements.
        //Unless it's an array of simple (non-struct) data, and 'iterateSimpleArrayElements' is false.
        bool encounteredArray = uType.IsArray() && !iterateArrays,
             shouldIterateArray = iterateSimpleArrayElements ||
                                  std::holds_alternative<StructInstance>(uType.ElementType);
        if (encounteredArray && shouldIterateArray)
        {
            for (uint32_t i = 0; i < uType.ArrayCount; ++i)
            {
                VisitUniform(func,
                             uName + "[" + std::to_string(i) + "]", uType,
                             defs, usedStructs, iterateSimpleArrayElements, true);
            }
        }
        //Otherwise, if this is a struct uniform, iterate its fields.
        else if (std::holds_alternative<StructInstance>(uType.ElementType))
        {
            const auto& structName = std::get<StructInstance>(uType.ElementType).Get();
            
            //Avoid infinite loops of struct A containing struct B containing struct A...
            BP_ASSERT_STR(
                usedStructs.find(structName) == usedStructs.end(),
                "Nested reference to a struct in a struct in a struct, etc. "
                  "Involving struct '" + structName + "'"
            );
            usedStructs.insert(structName);

            const auto& structInfo = defs.Structs.find(structName);
            BP_ASSERT_STR(structInfo != defs.Structs.end(),
                          "Uniform '" + uName + "' is of type 'struct " +
                              structName + "', but such a struct doesn't exist");

            for (const auto& [fieldName, fieldType] : structInfo->second)
            {
                VisitUniform(func, uName + "." + fieldName,
                             fieldType, defs, usedStructs, iterateSimpleArrayElements);
            }
        }
        //Otherwise, we've found an "atomic" uniform field.
        else
        {
            func(uName, uType);
        }
    }
}


std::string Bplus::GL::Uniforms::GetDescription(const Type& type)
{
    std::string output;

    if (std::holds_alternative<Vector>(type.ElementType))
    {
        const auto& vData = std::get<Vector>(type.ElementType);
        switch (vData.Type)
        {
            case ScalarTypes::Float:
                output += (vData.D == +VectorSizes::One) ? "float" : "f";
            break;
            case ScalarTypes::Double:
                output += (vData.D == +VectorSizes::One) ? "double" : "d";
            break;
            case ScalarTypes::Int:
                output += (vData.D == +VectorSizes::One) ? "int" : "i";
            break;
            case ScalarTypes::UInt:
                output += (vData.D == +VectorSizes::One) ? "uint" : "u";
            break;
            case ScalarTypes::Bool:
                output += (vData.D == +VectorSizes::One) ? "bool" : "b";
            break;
        }
        if (vData.D != +VectorSizes::One)
        {
            output += "vec";
            output += std::to_string(vData.D._to_integral());
        }
    }
    else if (std::holds_alternative<Matrix>(type.ElementType))
    {
        const auto& mData = std::get<Matrix>(type.ElementType);

        if (mData.IsDouble)
            output += "dmat";
        else
            output += "fmat";

        output += std::to_string(mData.Columns._to_integral());
        if (mData.Rows != mData.Columns)
        {
            output += "x";
            output += std::to_string(mData.Rows._to_integral());
        }
    }
    else if (std::holds_alternative<Color>(type.ElementType))
    {
        const auto& cData = std::get<Color>(type.ElementType);

        output += cData.Channels._to_string();

        if (cData.IsHDR)
            output += "_hdr";
    }
    else if (std::holds_alternative<Gradient>(type.ElementType))
    {
        const auto& gData = std::get<Gradient>(type.ElementType);

        output += "gradient";

        if (gData.IsHDR())
            output += "_hdr";
    }
    else if (std::holds_alternative<TexSampler>(type.ElementType))
    {
        const auto& sData = std::get<TexSampler>(type.ElementType);

        output += "sampler";
        output += sData.Type._to_string();
        if (sData.FullSampler.has_value())
            output += "*";
    }
    else if (std::holds_alternative<StructInstance>(type.ElementType))
    {
        const auto& sData = std::get<StructInstance>(type.ElementType);

        output += "struct:";
        output += sData.Get();
    }
    else
    {
        BP_ASSERT_STR(false,
                      "Unknown element type, index " + std::to_string(type.ElementType.index()));
        return "UNKNOWN";
    }

    if (type.IsArray())
        output += "[" + std::to_string(type.ArrayCount) + "]";

    return output;
}


std::string Definitions::Import(const Definitions& newDefs)
{
    for (const auto& [structName, structDef] : newDefs.Structs)
    {
        if (Structs.find(structName) != Structs.end())
            return std::string("Duplicate struct name: ") + structName;
        Structs[structName] = structDef;
    }

    for (const auto& [uniformName, uniformDef] : newDefs.Uniforms)
    {
        if (Uniforms.find(uniformName) != Uniforms.end())
            return std::string("Duplicate uniform name: ") + uniformName;
        Uniforms[uniformName] = uniformDef;
    }

    return "";
}
void Definitions::VisitAllUniforms(bool iterateSimpleArrayElements,
                                   std::function<void(const std::string&, const Type&)> visitor) const
{
    std::unordered_set<std::string> usedStructs;
    for (const auto& [uName, uType] : Uniforms)
    {
        VisitUniform(visitor, uName, uType, *this, usedStructs,
                     iterateSimpleArrayElements);
        usedStructs.clear();
    }
}