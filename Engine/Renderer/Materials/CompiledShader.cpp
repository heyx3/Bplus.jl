#include "CompiledShader.h"

#include "../Buffers/Buffer.h"


using namespace Bplus::GL;


//Hook into the thread rendering context's "RefreshState()" function
//    to refresh the currently-active shader.
namespace
{
    thread_local struct
    {
        const CompiledShader* currentShader = nullptr;
        bool initializedYet = false;

        std::unordered_map<OglPtr::ShaderProgram, const CompiledShader*> shadersByHandle =
            std::unordered_map<OglPtr::ShaderProgram, const CompiledShader*>();

        //Annoyingly, OpenGL booleans have to be sent in as 32-bit integers.
        //This buffer stores the booleans, converted to integers to be sent to OpenGL.
        std::vector<GLuint> uniformBoolBuffer;
    } threadData;


    //Returns the error message, or an empty string if everything is fine.
    std::string TryCompile(GLuint shaderObject)
    {
        glCompileShader(shaderObject);

        GLint isCompiled = 0;
        glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint msgLength = 0;
            glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &msgLength);

            std::vector<char> msgBuffer(msgLength);
            glGetShaderInfoLog(shaderObject, msgLength, &msgLength, msgBuffer.data());

            return std::string(msgBuffer.data());
        }

        return "";
    }
}



OglPtr::ShaderProgram CompiledShader::Compile(std::string vertShader, std::string fragShader,
                                              std::string& outErrMsg)
{
    //Generate the OpenGL/extensions declarations for the top of the shader files.
    std::string shaderPrefix = Context::GLSLVersion();
    shaderPrefix += "\n";
    for (int i = 0; i < Context::GLSLExtensions().size(); ++i)
    {
        shaderPrefix += Context::GLSLExtensions()[i];
        shaderPrefix += "\n";
    }
    vertShader.insert(0, shaderPrefix);
    fragShader.insert(0, shaderPrefix);

    GLuint vertShaderObj = glCreateShader(GL_VERTEX_SHADER),
           fragShaderObj = glCreateShader(GL_FRAGMENT_SHADER);

    auto* shaderPtr = vertShader.c_str();
    glShaderSource(vertShaderObj, 1, &shaderPtr, nullptr);
    shaderPtr = fragShader.c_str();
    glShaderSource(fragShaderObj, 1, &shaderPtr, nullptr);

    auto errorMsg = TryCompile(vertShaderObj);
    if (errorMsg.size() > 0)
    {
        outErrMsg = "Error compiling vertex shader: " + errorMsg;
        glDeleteShader(vertShaderObj);
        glDeleteShader(fragShaderObj);
        return OglPtr::ShaderProgram();
    }

    errorMsg = TryCompile(fragShaderObj);
    if (errorMsg.size() > 0)
    {
        outErrMsg = "Error compiling fragment shader: " + errorMsg;
        glDeleteShader(vertShaderObj);
        glDeleteShader(fragShaderObj);
        return OglPtr::ShaderProgram();
    }

    //Now that everything is compiled, try linking it all together.
    GLuint programObj = glCreateProgram();
    glAttachShader(programObj, vertShaderObj);
    glAttachShader(programObj, fragShaderObj);
    glLinkProgram(programObj);
    //We can immediately mark the individual shader objects for cleanup.
    glDeleteShader(vertShaderObj);
    glDeleteShader(fragShaderObj);
    //Check the link result.
    GLint isLinked = 0;
    glGetProgramiv(programObj, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint msgLength = 0;
        glGetProgramiv(programObj, GL_INFO_LOG_LENGTH, &msgLength);

        std::vector<char> msgBuffer(msgLength);
        glGetProgramInfoLog(programObj, msgLength, &msgLength, msgBuffer.data());

        glDeleteProgram(programObj);
        errorMsg = "Error linking shaders: " + std::string(msgBuffer.data());
        return OglPtr::ShaderProgram();
    }

    //If the link is successful, we need to "detach" the shader objects
    //    from the program object so that they can be cleaned up.
    glDetachShader(programObj, vertShaderObj);
    glDetachShader(programObj, fragShaderObj);

    return OglPtr::ShaderProgram(programObj);
}

OglPtr::ShaderProgram CompiledShader::Compile(std::string vertShader,
                                              std::string geomShader,
                                              std::string fragShader,
                                              std::string& outErrMsg)
{
    //Generate the OpenGL/extensions declarations for the top of the shader files.
    std::string shaderPrefix = Context::GLSLVersion();
    shaderPrefix += "\n";
    for (int i = 0; i < Context::GLSLExtensions().size(); ++i)
    {
        shaderPrefix += Context::GLSLExtensions()[i];
        shaderPrefix += "\n";
    }
    vertShader.insert(0, shaderPrefix);
    fragShader.insert(0, shaderPrefix);
    geomShader.insert(0, shaderPrefix);

    GLuint vertShaderObj = glCreateShader(GL_VERTEX_SHADER),
           geomShaderObj = glCreateShader(GL_GEOMETRY_SHADER),
           fragShaderObj = glCreateShader(GL_FRAGMENT_SHADER);

    auto* shaderPtr = vertShader.c_str();
    glShaderSource(vertShaderObj, 1, &shaderPtr, nullptr);
    shaderPtr = geomShader.c_str();
    glShaderSource(geomShaderObj, 1, &shaderPtr, nullptr);
    shaderPtr = fragShader.c_str();
    glShaderSource(fragShaderObj, 1, &shaderPtr, nullptr);

    auto errorMsg = TryCompile(vertShaderObj);
    if (errorMsg.size() > 0)
    {
        outErrMsg = "Error compiling vertex shader: " + errorMsg;
        glDeleteShader(vertShaderObj);
        glDeleteShader(geomShaderObj);
        glDeleteShader(fragShaderObj);
        return OglPtr::ShaderProgram();
    }

    errorMsg = TryCompile(geomShaderObj);
    if (errorMsg.size() > 0)
    {
        outErrMsg = "Error compiling geometry shader: " + errorMsg;
        glDeleteShader(vertShaderObj);
        glDeleteShader(geomShaderObj);
        glDeleteShader(fragShaderObj);
        return OglPtr::ShaderProgram();
    }

    errorMsg = TryCompile(fragShaderObj);
    if (errorMsg.size() > 0)
    {
        outErrMsg = "Error compiling fragment shader: " + errorMsg;
        glDeleteShader(vertShaderObj);
        glDeleteShader(geomShaderObj);
        glDeleteShader(fragShaderObj);
        return OglPtr::ShaderProgram();
    }

    //Now that everything is compiled, try linking it all together.
    GLuint programObj = glCreateProgram();
    glAttachShader(programObj, vertShaderObj);
    glAttachShader(programObj, geomShaderObj);
    glAttachShader(programObj, fragShaderObj);
    glLinkProgram(programObj);
    //We can immediately mark the individual shader objects for cleanup.
    glDeleteShader(vertShaderObj);
    glDeleteShader(geomShaderObj);
    glDeleteShader(fragShaderObj);
    //Check the link result.
    GLint isLinked = 0;
    glGetProgramiv(programObj, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint msgLength = 0;
        glGetProgramiv(programObj, GL_INFO_LOG_LENGTH, &msgLength);

        std::vector<char> msgBuffer(msgLength);
        glGetProgramInfoLog(programObj, msgLength, &msgLength, msgBuffer.data());

        glDeleteProgram(programObj);
        errorMsg = "Error linking shaders: " + std::string(msgBuffer.data());
        return OglPtr::ShaderProgram();
    }

    //If the link is successful, we need to "detach" the shader objects
    //    from the program object so that they can be cleaned up.
    glDetachShader(programObj, vertShaderObj);
    glDetachShader(programObj, geomShaderObj);
    glDetachShader(programObj, fragShaderObj);

    return OglPtr::ShaderProgram(programObj);
}

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
            BPAssert(threadData.shadersByHandle.size() == 0,
                     "Some CompiledShader instances haven't been cleaned up");
            threadData.shadersByHandle.clear();
        });
    }


    BPAssert(threadData.shadersByHandle.find(programHandle) == threadData.shadersByHandle.end(),
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

    BPAssert(threadData.shadersByHandle.find(programHandle) != threadData.shadersByHandle.end(),
             "CompiledShader is missing from 'shadersByHandle'");
    BPAssert(threadData.shadersByHandle[programHandle] == &src,
             "Some other CompiledShader owns this program handle");
    threadData.shadersByHandle[programHandle] = this;
}
CompiledShader& CompiledShader::operator=(CompiledShader&& src)
{
    uniformPtrs = std::move(src.uniformPtrs);
    programHandle = src.programHandle;

    src.programHandle = OglPtr::ShaderProgram();
    
    BPAssert(threadData.shadersByHandle.find(programHandle) != threadData.shadersByHandle.end(),
             "CompiledShader is missing from 'shadersByHandle'");
    BPAssert(threadData.shadersByHandle[programHandle] == &src,
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
    Context::GetCurrentContext()->SetState(RenderSettings);

    if (threadData.currentShader == this)
        return;

    glUseProgram(programHandle.Get());
    threadData.currentShader = this;
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