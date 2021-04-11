#include "ShaderDefinition.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


namespace
{
    void GenerateStaticDef(std::string outCode, const ShaderDefinition& defs,
                           const std::string& defName,
                           const Uniforms::StaticUniformValues::mapped_type& value)
    {
        if (std::holds_alternative<int64_t>(value))
        {
            outCode += "#define ";
            outCode += defName;
            outCode += std::to_string(std::get<int64_t>(value));
        }
        else if (std::holds_alternative<std::string>(value))
        {
            outCode += "#define ";
            outCode += defName;
            outCode += "_";
            outCode += std::get<std::string>(value);
        }
        else
        {
            BP_ASSERT_STR(false,
                          "Unknown static uniform value type, index " +
                              std::to_string(value.index()));
            outCode += "//  [failed to #define '";
            outCode += defName;
            outCode += "' here]";
        }

        outCode += "\n";
    }
    void GenerateStructDef(std::string& outCode, const ShaderDefinition& defs,
                           const std::string& structName, const Uniforms::StructDef& structDef)
    {
        outCode += "struct ";
        outCode += structName;
    }
    void GenerateUniformDef(std::string& outCode, const ShaderDefinition& defs,
                            const std::string& uName, const Uniforms::Type& uType)
    {
        outCode += "uniform ";

        //Generate the uniform's type name.
        if (std::holds_alternative<Uniforms::Vector>(uType.ElementType))
        {
            const auto& vData = std::get<Uniforms::Vector>(uType.ElementType);
            if (vData.D == +Uniforms::VectorSizes::One)
                switch (vData.Type)
                {
                    case Uniforms::ScalarTypes::Float: outCode += "float"; break;

                    case Uniforms::ScalarTypes::Double: outCode += "double"; break;
                    case Uniforms::ScalarTypes::Int: outCode += "int"; break;
                    case Uniforms::ScalarTypes::UInt: outCode += "uint"; break;
                    case Uniforms::ScalarTypes::Bool: outCode += "bool"; break;

                    default:
                        BP_ASSERT_STR(false,
                                      "Unexpected Bplus::GL::Uniforms::ScalarTypes::" +
                                          vData.Type._to_string());
                    break;
                }
            else
            {
                //Append the vector's type letter.
                //E.x. "uvec3" is a 3D vector of uints, "vec2" is a 2D vector of floats.
                switch (vData.Type)
                {
                    case Uniforms::ScalarTypes::Float: break;

                    case Uniforms::ScalarTypes::Double: outCode += "d"; break;
                    case Uniforms::ScalarTypes::Int: outCode += "i"; break;
                    case Uniforms::ScalarTypes::UInt: outCode += "u"; break;
                    case Uniforms::ScalarTypes::Bool: outCode += "b"; break;

                    default:
                        BP_ASSERT_STR(false,
                                      "Unexpected Bplus::GL::Uniforms::ScalarTypes::" +
                                          vData.Type._to_string());
                    break;
                }

                outCode += "vec";
                outCode += vData.D._to_integral();
            }
        }
        else if (std::holds_alternative<Uniforms::Matrix>(uType.ElementType))
        {
            const auto& mData = std::get<Uniforms::Matrix>(uType.ElementType);

            if (mData.IsDouble)
                outCode += "dmat";
            else
                outCode += "fmat";

            outCode += mData.Columns._to_integral();
            //If it's a square matrix, the GLSL type is just 'n' instead of 'nxn'.
            if (mData.Rows != mData.Columns)
            {
                outCode += "x";
                outCode += mData.Rows._to_integral();
            }
        }
        else if (std::holds_alternative<Uniforms::Color>(uType.ElementType))
        {
            const auto& cData = std::get<Uniforms::Color>(uType.ElementType);
            switch (cData.Channels)
            {
                case Textures::SimpleFormatComponents::R: outCode += "float"; break;
                case Textures::SimpleFormatComponents::RG: outCode += "vec2"; break;
                case Textures::SimpleFormatComponents::RGB: outCode += "vec3"; break;
                case Textures::SimpleFormatComponents::RGBA: outCode += "vec4"; break;

                default:
                    BP_ASSERT_STR(false,
                                  "Unexpected Bplus::GL::Textures::SimpleFormatComponents::" +
                                      cData.Channels._to_string());
                break;
            }
        }
        else if (std::holds_alternative<Uniforms::Gradient>(uType.ElementType))
        {
            const auto& gData = std::get<Uniforms::Gradient>(uType.ElementType);
            outCode += "sampler1D";
        }
        else if (std::holds_alternative<Uniforms::TexSampler>(uType.ElementType))
        {
            const auto& tData = std::get<Uniforms::TexSampler>(uType.ElementType);
            
            //Assert that shadow samplers aren't using an integer or float format.

            //Int and UInt samplers have a special prefix.
            switch (tData.SamplingType)
            {
                case Uniforms::SamplerTypes::Float: break;
                case Uniforms::SamplerTypes::Shadow: break;

                case Uniforms::SamplerTypes::Int: outCode += "i"; break;
                case Uniforms::SamplerTypes::UInt: outCode += "u"; break;

                default:
                    BP_ASSERT_STR(false,
                                  "Unknown Bplus::GL::Uniforms::SamplingType::" + tData.SamplingType._to_string());
                break;
            }

            outCode += "sampler";

            switch (tData.Type)
            {
                case Textures::Types::OneD: outCode += "1D"; break;
                case Textures::Types::TwoD: outCode += "2D"; break;
                case Textures::Types::ThreeD: outCode += "3D"; break;
                case Textures::Types::Cubemap: outCode += "Cube"; break;
                    
                default:
                    BP_ASSERT_STR(false,
                                  "Unknown Bplus::GL::Textures::Types::" + tData.Type._to_string());
                break;
            }

            //Shadow samplers have a special suffix.
            if (tData.SamplingType == +Uniforms::SamplerTypes::Shadow)
                outCode += "Shadow";
        }
        else if (std::holds_alternative<Uniforms::StructInstance>(uType.ElementType))
        {
            const auto& sData = std::get<Uniforms::StructInstance>(uType.ElementType);
            outCode += ShaderDefinition::Prefix_Structs();
            outCode += sData.Get();
        }
        else
        {
            BP_ASSERT_STR(false,
                          "Unknown uniform type, index " +
                              std::to_string(uType.ElementType.index()));
        }

        outCode += " ";

        //Add the name and array size.
        outCode += uName;
        if (uType.IsArray())
        {
            outCode += "[";
            outCode += std::to_string(uType.ArrayCount);
            outCode += "]";
        }

        //Finish up.
        outCode += ";\n";
    }
}


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

void ShaderDefinition::GenerateCode(const Uniforms::StaticUniformValues& staticValues,
                                    std::string& outCode) const
{
    //Generate #define statements based on static uniforms.
    if (!GetStatics().Definitions.empty())
    {
        outCode += "// ==================================================\n"
                   "//                         Statics                   \n"
                   "// ==================================================\n\n";
        
        for (const auto& staticName : GetStatics().Ordering)
            GenerateStaticDef(outCode, *this, staticName, staticValues.at(staticName));
        
        outCode += "\n// ==================================================\n"
                     "//                     End of Statics                \n"
                     "// ==================================================\n\n\n\n\n";
    }

    //Generate structs and uniforms.
    if (!GetUniforms().Structs.empty())
    {
        outCode += "\n// ==================================================\n"
                     "//                         Structs                   \n"
                     "// ==================================================\n\n";

        for (const auto& [structName, structDef] : GetUniforms().Structs)
            GenerateStructDef(outCode, *this, Prefix_Structs() + structName, structDef);
        
        outCode += "\n// ==================================================\n"
                     "//                   End of Structs                  \n"
                     "// ==================================================\n\n\n\n";
    }
    if (!GetUniforms().Uniforms.empty())
    {
        outCode += "\n// ==================================================\n"
                     "//                         Uniforms                  \n"
                     "// ==================================================\n\n";

        for (const auto& [uniformName, uniformDef] : GetUniforms().Uniforms)
            GenerateUniformDef(outCode, *this, Prefix_Uniforms() + uniformName, uniformDef);
        
        outCode += "\n// ==================================================\n"
                     "//                   End of Uniforms                 \n"
                     "// ==================================================\n\n\n\n";
    }

    //Emit custom user code.
    if (!code.empty())
    {
        outCode += "\n// ==================================================\n"
                     "//                         User Code                 \n"
                     "// ==================================================\n\n";

        outCode += code;

        outCode += "\n// ==================================================\n"
                     "//                   End of User Code                \n"
                     "// ==================================================\n\n\n\n";
    }
}