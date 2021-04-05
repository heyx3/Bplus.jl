#include "ShaderDefinition.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


std::string ShaderDefinition::ProcessIncludes(LoaderFunc tryLoad,
                                              std::unordered_set<std::string>* _usedIncludes)
{
    //If not given a "usedIncludes", create a new empty one.
    Lazy<std::unordered_set<std::string>> internalUsedIncludes;
    auto& usedIncludes = (_usedIncludes == nullptr) ?
                             internalUsedIncludes.Get() :
                             *_usedIncludes;

    //Try to load each include path.
    for (const std::string& includePath : includedDefs)
    {
        //Don't load an 'include' more than once.
        if (usedIncludes.find(includePath) != usedIncludes.end())
            continue;

        //Try and load the 'include'.
        usedIncludes.insert(includePath);
        auto tryLoaded = tryLoad(includePath);
        if (std::holds_alternative<std::string>(tryLoaded))
        {
            return "Error loading \"" + includePath + "\": " +
                     std::get<std::string>(tryLoaded);
        }
        auto& loaded = std::get<ShaderDefinition>(tryLoaded);

        //Process the sub-shader's own includes.
        std::string subErrorMsg = loaded.ProcessIncludes(tryLoad, &usedIncludes);
        if (!subErrorMsg.empty())
            return "[" + includePath + "]: " + subErrorMsg;
        BP_ASSERT_STR(loaded.GetIncludes().empty(),
                      "'Include's aren't emptied out after processing them: '" +
                          includePath + "'");

        //Apply it to this instance.
        subErrorMsg = MergeIn(std::get<ShaderDefinition>(tryLoaded));
        if (!subErrorMsg.empty())
            return "Error loading \"" + includePath + "\": " + subErrorMsg;
    }
    includedDefs.clear();

    return "";
}

std::string ShaderDefinition::MergeIn(const ShaderDefinition& input)
{
    //Merge in static variables.
    const auto& newStatics = input.GetStatics();
    for (const std::string& sName : newStatics.Ordering)
    {
        //Check for duplicate names.
        if (statics.Definitions.find(sName) != statics.Definitions.end())
            return "Duplicate static uniform: '" + sName + "'";

        //Merge in.
        statics.Definitions[sName] = newStatics.Definitions.at(sName);
    }

    //Merge in struct definitions.
    const auto& newUniforms = input.GetUniforms();
    for (const auto& [sName, sDef] : newUniforms.Structs)
    {
        //Check for duplicate names.
        if (uniforms.Structs.find(sName) != uniforms.Structs.end())
            return "Duplicate struct: '" + sName + "'";

        //Merge in.
        uniforms.Structs[sName] = sDef;
    }
    //Merge in uniforms.
    for (const auto& [uName, uDef] : newUniforms.Uniforms)
    {
        //Check for duplicate names.
        if (uniforms.Uniforms.find(uName) != uniforms.Uniforms.end())
            return "Duplicate uniform: '" + Uniforms::GetDescription(uDef) + " " + uName + "'";

        //Merge in.
        uniforms.Uniforms[uName] = uDef;
    }


    //Merge in shader 'include' statements.
    includedDefs.insert(includedDefs.end(),
                        input.GetIncludes().begin(),
                        input.GetIncludes().end());

    //Merge in code.
    code = code + input.code;

    return "";
}