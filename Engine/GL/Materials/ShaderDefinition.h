#pragma once

#include "../Uniforms/DataStructures.h"
#include "../Uniforms/StaticUniforms.h"
#include "ShaderCompileJob.h"


namespace Bplus::GL::Materials
{
    //The data associated with a shader, including uniforms, static parameters,
    //    references to other ShaderDefinition instances, etc.
    class BP_API ShaderDefinition
    {
    public:

        //The below prefixes are used inside shaders,
        //    but are mostly hidden from the public API of classes in this namespace.
        static inline const char* Prefix_Structs() { return "S_"; }
        static inline const char* Prefix_Uniforms() { return "u_"; }


        ShaderDefinition() { }
        ShaderDefinition(Uniforms::StaticUniformDefs&& statics,
                         Uniforms::Definitions&& uniforms,
                         std::vector<std::string>&& includes,
                         std::string&& code)
            : statics(std::move(statics)), uniforms(std::move(uniforms)),
              includedDefs(std::move(includes)), code(std::move(code)) { }
        

        const auto& GetStatics() const { return statics; }
        const auto& GetUniforms() const { return uniforms; }
        const auto& GetIncludes() const { return includedDefs; }


        //A function that tries to load a ShaderDefinition instance from
        //    a given string.
        //Returns the loaded data, or an error message if something went wrong.
        using LoaderFunc = std::function<
            std::variant<ShaderDefinition, std::string> (const std::string& path)
        >;

        //Modifies this instance to load in all the "include"-ed sub-shaders recursively.
        //Returns an error message if something went wrong, or an empty string if successful.
        //The last parameter is an optional input/output
        //    to ignore duplicate "includes" and output which definitions have been "included".
        std::string ProcessIncludes(LoaderFunc tryLoad,
                                    std::unordered_set<std::string>* usedIncludes = nullptr);

        //Generates shader code for this instance's data.
        //Does NOT include the "include" statements;
        //    call 'ProcessIncludes()' first to merge them into this instance.
        void GenerateCode(const Uniforms::StaticUniformValues& staticUniforms,
                          std::string& outCode) const;

    private:

        Uniforms::StaticUniformDefs statics;
        Uniforms::Definitions uniforms;
        std::string code;

        std::vector<std::string> includedDefs;


        //Combines the given shader definitions into this one.
        //Returns an error message if something went wrong (e.x. duplicate uniforms),
        //    or the empty string if everything went fine.
        std::string MergeIn(const ShaderDefinition& input);
    };
}