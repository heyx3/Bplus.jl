#include "UniformDataStructures.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Uniforms;


std::string UniformDefinitions::Import(const UniformDefinitions& newDefs)
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