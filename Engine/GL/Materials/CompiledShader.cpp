#include "CompiledShader.h"

#include "Factory.h"


using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


CompiledShader::CompiledShader(Factory& owner,
                               OglPtr::ShaderProgram&& compiledProgramHandle,
                               const Uniforms::UniformDefinitions& uniforms)
    : programHandle(compiledProgramHandle), owner(&owner)
{
    compiledProgramHandle = OglPtr::ShaderProgram::Null();
    //Build the map of uniforms and their current values.
    //uniforms
}
CompiledShader::~CompiledShader()
{
    if (!programHandle.IsNull())
    {
        glDeleteProgram(programHandle.Get());
    }
}

CompiledShader::CompiledShader(CompiledShader&& src)
    : owner(src.owner), programHandle(src.programHandle),
      uniformPtrs(std::move(src.uniformPtrs)),
      uniformValues(std::move(src.uniformValues))
{
    src.programHandle = OglPtr::ShaderProgram();
}
CompiledShader& Bplus::GL::Materials::CompiledShader::operator=(CompiledShader&& src)
{
    if (&src == this)
        return *this;

    this->~CompiledShader();
    new (this) CompiledShader(std::move(src));

    return *this;
}

void CompiledShader::Activate() const
{
    glUseProgram(programHandle.Get());
}

CompiledShader::UniformAndStatus CompiledShader::CheckUniform(const std::string& name) const
{
    //Check whether the name exists.
    auto foundPtr = uniformPtrs.find(name);
    if (foundPtr == uniformPtrs.end())
        return { OglPtr::ShaderUniform::Null(), UniformStates::Missing };
    
    //Check whether the uniform actually exists in the shader program.
    auto ptr = foundPtr->second;
    if (ptr.IsNull())
        return { ptr, UniformStates::OptimizedOut };

    //Everything checks out!
    return { ptr, UniformStates::Exists };
}