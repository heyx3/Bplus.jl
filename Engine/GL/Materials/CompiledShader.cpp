#include "CompiledShader.h"

#include "../Buffers/Buffer.h"


using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


CompiledShader::CompiledShader(RenderState renderSettings,
                               OglPtr::ShaderProgram compiledProgramHandle,
                               const std::vector<std::string>& uniformNames)
    : programHandle(compiledProgramHandle),
      RenderSettings(renderSettings), DefaultRenderSettings(renderSettings)
{
    if (!threadData.initializedYet)
    {
        threadData.initializedYet = true;

        std::function<void()> refreshContext = []()
        {
            //Get the handle of the currently-bound shader.
            GLint _currentProgram;
            glGetIntegerv(GL_CURRENT_PROGRAM, &_currentProgram);
            OglPtr::ShaderProgram currentProgram((GLuint)_currentProgram);

            //Look that shader up in the thread-local dictionary
            //    of all compiled shaders.
            auto found = threadData.shadersByHandle.find(currentProgram);
            threadData.currentShader = (found == threadData.shadersByHandle.end()) ?
                                           nullptr :
                                           found->second;
        };
        refreshContext();
        Context::RegisterCallback_RefreshState(refreshContext);

        Context::RegisterCallback_Destroyed([]()
        {
            //If any CompiledShaders haven't been cleaned up yet,
            //    it's a memory leak.
            BP_ASSERT(threadData.shadersByHandle.size() == 0,
                     "Some CompiledShader instances haven't been cleaned up");
            threadData.shadersByHandle.clear();
        });
    }


    BP_ASSERT(threadData.shadersByHandle.find(programHandle) == threadData.shadersByHandle.end(),
             "A CompiledShader already exists with this program");
    threadData.shadersByHandle[programHandle] = this;

    //Store all uniform pointers, ignoring ones that don't exist/
    //    have been optimized out.
    for (const auto& uniformName : uniformNames)
    {
        OglPtr::ShaderUniform loc{ glGetUniformLocation(programHandle.Get(),
                                                        uniformName.c_str()) };
        if (!loc.IsNull())
            uniformPtrs[uniformName] = loc;
    }
}
CompiledShader::~CompiledShader()
{
    if (!programHandle.IsNull())
    {
        glDeleteProgram(programHandle.Get());
        threadData.shadersByHandle.erase(programHandle);
    }
}

CompiledShader::CompiledShader(CompiledShader&& src)
    : uniformPtrs(std::move(src.uniformPtrs)), programHandle(src.programHandle)
{
    src.programHandle = OglPtr::ShaderProgram();

    BP_ASSERT(threadData.shadersByHandle.find(programHandle) != threadData.shadersByHandle.end(),
             "CompiledShader is missing from 'shadersByHandle'");
    BP_ASSERT(threadData.shadersByHandle[programHandle] == &src,
             "Some other CompiledShader owns this program handle");
    threadData.shadersByHandle[programHandle] = this;
}
CompiledShader& CompiledShader::operator=(CompiledShader&& src)
{
    uniformPtrs = std::move(src.uniformPtrs);
    programHandle = src.programHandle;

    src.programHandle = OglPtr::ShaderProgram();
    
    BP_ASSERT(threadData.shadersByHandle.find(programHandle) != threadData.shadersByHandle.end(),
             "CompiledShader is missing from 'shadersByHandle'");
    BP_ASSERT(threadData.shadersByHandle[programHandle] == &src,
             "Some other CompiledShader owns this program handle");
    threadData.shadersByHandle[programHandle] = this;

    return *this;
}

const CompiledShader* CompiledShader::GetCurrentActive()
{
    return threadData.currentShader;
}
void CompiledShader::Activate() const
{
    glUseProgram(programHandle.Get());
}

const CompiledShader* CompiledShader::Find(OglPtr::ShaderProgram ptr)
{
    auto found = threadData.shadersByHandle.find(ptr);
    if (found == threadData.shadersByHandle.end())
        return nullptr;
    else
        return found->second;
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