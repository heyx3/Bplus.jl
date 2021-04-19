#pragma once

#include "../Uniforms/Storage.h"
#include "CompiledShader.h"
#include "ShaderDefinition.h"

namespace Bplus::GL::Materials
{
    using CompiledShaderPtr = std::unique_ptr<CompiledShader>;


    //A group of shaders that can be seamlessly swapped out
    //    based on shader-compile-time settings.
    //Each individual compiled shader is called a "variant".
    class BP_API Factory
    {
    public:

        //A callback for when a shader variant fails to compile.
        //The first argument is the shader, the second argument is this factory,
        //    and the last argument is the error message.
        using ErrorCallback_t = void(const ShaderCompileJob& shader,
                                     const Factory& me,
                                     const std::string& errorMsg);

        //Default behavior: BP_ASSERT_STR(false, ...)
        std::function<ErrorCallback_t> OnError =
            [](const ShaderCompileJob&, const Factory&,
               const std::string& errorMsg)
            {
                BP_ASSERT_STR(false, "Error compiling shader: " + errorMsg);
            };

        //Processes "include" statements in any shaders.
        //Default behavior: returns an error for every include statement.
        std::function<FileContentsLoader> ProcessIncludeStatement =
            [](const std::filesystem::path&, std::stringstream&)
            {
                return false;
            };

        //TODO: A way to get the compiled binary for each shader.



        //Constructs a Material Factory from the given shader definition.
        //Assumes that the definition has been fully processed, and has no 'include's left.
        Factory(const ShaderDefinition& processedDefs,
                const std::string& vertexShader, const std::string&);
        ~Factory() { }


        const ShaderDefinition& GetShaderDefs() const { return shaderDefs; }
        
        const Uniforms::Definitions&       GetUniformDefs() const { return shaderDefs.GetUniforms(); }
        const Uniforms::StaticUniformDefs& GetStaticDefs() const { return shaderDefs.GetStatics(); }

        //Gets the shader variant for the given static uniform values.
        //If this is the first time this variant is being used,
        //    it will be generated and compiled, which can cause a slight hang.
        //Returns null if the shader didn't compile.
        const CompiledShader* GetVariant(const Uniforms::StaticUniformValues& statics) const;

    private:

        ShaderDefinition shaderDefs;
        std::string vertShader, geomShader, fragShader;

        mutable std::unordered_map<Uniforms::StaticUniformValues,
                                   CompiledShaderPtr>
            cachedShaders;


        //Compiles the given shader variant, puts it into the map of cached values,
        //    and returns it (or null if compilation failed).
        CompiledShader* Compile(const Uniforms::StaticUniformValues& statics) const;
    };
}