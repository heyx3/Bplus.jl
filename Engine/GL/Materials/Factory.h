#pragma once

#include "../Uniforms/Storage.h"
#include "CompiledShader.h"
#include "ShaderDefinition.h"

namespace Bplus::GL::Materials
{
    //CompiledShaders are expected to never move after creation,
    //    to keep the rest of the Material classes simpler,
    //    so the norm will be to store them in unique_ptr.
    using CompiledShaderPtr = std::unique_ptr<CompiledShader>;


    //A group of shaders that are generated from the same source,
    //    but with different preprocessor #defines.
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

        //What to do if a shader fails to compile.
        //Defaults to a BP_ASSERT failure.
        std::function<ErrorCallback_t> OnError =
            [](const ShaderCompileJob&, const Factory&,
               const std::string& errorMsg)
            {
                BP_ASSERT_STR(false, "Error compiling shader: " + errorMsg);
            };

        //Processes "include" statements in the shaders.
        //Default behavior: fails to process it.
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

        //Gets the shader variant for the given set of static uniform values.
        //If this is the first time a particular variant is being used,
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

    //The standard way to store a Factory is to use unique_ptr
    //    so that we don't have to worry about them being moved around.
    using FactoryPtr = std::unique_ptr<Factory>;
}