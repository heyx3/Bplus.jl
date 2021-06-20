#include "Factory.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;

namespace
{
    //Buffers for shader code generation/compilation.
    //Unique for each thread, to make it thread-safe.
    thread_local std::string codeStrBuffer;
    thread_local ShaderCompileJob shaderBuffer;
}

CompiledShader* Factory::Compile(const Uniforms::StaticUniformValues& statics) const
{
    //thread_local variables have an implicit cost to their getter/setter,
        //    so cache a reference to it.
    auto& shaderCompiler = shaderBuffer;
    auto& shaderStr = codeStrBuffer;

    shaderCompiler.Clear();
    shaderStr.clear();

    shaderDefs.GenerateCode(statics, shaderStr);

    //Generate the string for each shader type -- vertex, geometry, fragment...
    auto processShader = [&](const std::string& mainShaderCode, std::string& outField,
                             const char* typeName)
    {
        if (mainShaderCode.empty())
            return;

        outField = shaderStr;

        outField += "\n\n===============================\n"
                        "==       ";
        outField +=               typeName;
        outField +=                      " Shader     ==\n"
                        "===============================\n\n";

        outField += mainShaderCode;
    };
    processShader(vertShader, shaderCompiler.VertexSrc, "Vertex");
    processShader(geomShader, shaderCompiler.GeometrySrc, "Geometry");
    processShader(fragShader, shaderCompiler.FragmentSrc, "Fragment");

    shaderCompiler.IncludeImplementation = ProcessIncludeStatement;
    shaderCompiler.PreProcessIncludes();

    //Compile.
    OglPtr::ShaderProgram shaderOglPtr;
    auto compileResult = shaderCompiler.Compile(shaderOglPtr);
    if (std::get<bool>(compileResult))
    {
        cachedShaders[statics] = std::make_unique<CompiledShader>(std::move(shaderOglPtr),
                                                                  shaderDefs.GetUniforms());
        return cachedShaders[statics].get();
    }
    else
    {
        OnError(shaderCompiler, *this,
                std::get<std::string>(compileResult));
        return nullptr;
    }
}

const CompiledShader* Factory::GetVariant(const Uniforms::StaticUniformValues& statics) const
{
    auto found = cachedShaders.find(statics);

    if (found == cachedShaders.end())
        return Compile(statics);
    else
        return found->second.get();
}


Factory::Factory(const ShaderDefinition& processedDefs,
                 const std::string& vertexShader, const std::string&)
    : shaderDefs(processedDefs)
{
    BP_ASSERT(shaderDefs.GetIncludes().empty(),
              "ShaderDefinition was given to a Factory with un-processed 'include's");
}