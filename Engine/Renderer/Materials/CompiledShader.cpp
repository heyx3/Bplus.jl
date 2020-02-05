#include "CompiledShader.h"

using namespace Bplus::GL;

//Hook into the thread rendering context's "RefreshState()" function
//    to refresh the currently-active shader.
namespace
{
    thread_local struct
    {
        CompiledShader* currentShader = nullptr;
        bool initializedYet = false;

        std::unordered_map<OglPtr::ShaderProgram, CompiledShader*> shadersByHandle =
            std::unordered_map<OglPtr::ShaderProgram, CompiledShader*>();

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



OglPtr::ShaderProgram CompiledShader::Compile(const char* vertShader, const char* fragShader,
                                              std::string& outErrMsg)
{
    GLuint vertShaderObj = glCreateShader(GL_VERTEX_SHADER),
           fragShaderObj = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertShaderObj, 1, &vertShader, nullptr);
    glShaderSource(fragShaderObj, 1, &fragShader, nullptr);

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

OglPtr::ShaderProgram CompiledShader::Compile(const char* vertShader,
                                              const char* geomShader,
                                              const char* fragShader,
                                              std::string& outErrMsg)
{
    GLuint vertShaderObj = glCreateShader(GL_VERTEX_SHADER),
           geomShaderObj = glCreateShader(GL_GEOMETRY_SHADER),
           fragShaderObj = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertShaderObj, 1, &vertShader, nullptr);
    glShaderSource(geomShaderObj, 1, &geomShader, nullptr);
    glShaderSource(fragShaderObj, 1, &fragShader, nullptr);

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
        if (loc != OglPtr::ShaderUniform::Null)
            uniformPtrs[uniformName] = loc;
    }
}
CompiledShader::~CompiledShader()
{
    if (programHandle != OglPtr::ShaderProgram::Null)
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

CompiledShader* CompiledShader::GetCurrentActive()
{
    return threadData.currentShader;
}
void CompiledShader::Activate()
{
    Context::GetCurrentContext()->SetState(RenderSettings);

    if (threadData.currentShader == this)
        return;

    glUseProgram(programHandle.Get());
    threadData.currentShader = this;
}

std::tuple<CompiledShader::UniformStates, OglPtr::ShaderUniform>
    CompiledShader::CheckUniform(const std::string& name) const
{
    //Check whether the name exists.
    auto foundPtr = uniformPtrs.find(name);
    if (foundPtr == uniformPtrs.end())
        return std::make_tuple(UniformStates::Missing, OglPtr::ShaderUniform());
    
    //Check whether the name corresponds to a real uniform.
    auto ptr = foundPtr->second;
    if (ptr == OglPtr::ShaderUniform::Null)
        return std::make_tuple(UniformStates::OptimizedOut, ptr);

    //Everything checks out!
    return std::make_tuple(UniformStates::Exists, ptr);
}


#pragma region SetUniform() implementations

//Macro to define SetUniform() and SetUniformArray() that works with almost any type.
#define UNIFORM_SETTER(plainType, refType, glSingleSuffix, glArraySuffix, glSingleValue, glArrayValue) \
    void CompiledShader::SetUniform(OglPtr::ShaderUniform ptr, refType value) const \
    { \
        glProgramUniform##glSingleSuffix(programHandle.Get(), ptr.Get(), glSingleValue); \
    } \
    void CompiledShader::SetUniformArray(OglPtr::ShaderUniform ptr, GLsizei count, \
                                         const plainType* values) const \
    { \
        if (count < 1) return; \
        glProgramUniform##glArraySuffix(programHandle.Get(), ptr.Get(), count, \
                                        glArrayValue); \
    }

//Boolean uniforms need special treatment because they're sent into the OpenGL API as integers.
#pragma region SetUniform() for boolean primitive/vectors

void CompiledShader::SetUniform(OglPtr::ShaderUniform ptr, bool value) const
{
    glProgramUniform1ui(programHandle.Get(), ptr.Get(), (GLuint)value);
}
void CompiledShader::SetUniformArray(OglPtr::ShaderUniform ptr, GLsizei count, const bool* values) const
{
    //Restructure the booleans into an array of 32-bit integers.
    threadData.uniformBoolBuffer.resize(count);
    for (GLsizei i = 0; i < count; ++i)
        threadData.uniformBoolBuffer[i] = values[i];

    //Send the integers into OpenGL.
    glProgramUniform1uiv(programHandle.Get(), ptr.Get(), count,
                         threadData.uniformBoolBuffer.data());
}

void CompiledShader::SetUniformArray(OglPtr::ShaderUniform ptr, GLsizei count, const Bool* values) const
{
    //Originally I just passed this through to the "const bool*" version with a pointer cast,
    //    but that violates "strict aliasing" assumptions that the compiler makes
    //    for optimization purposes.
    //So to be safe, I just copied the code over.

    threadData.uniformBoolBuffer.resize(count);
    for (GLsizei i = 0; i < count; ++i)
        threadData.uniformBoolBuffer[i] = values[i];
    glProgramUniform1uiv(programHandle.Get(), ptr.Get(), count,
                         threadData.uniformBoolBuffer.data());
}

#define UNIFORM_SETTER_BOOLVEC(n, singleValue) \
    void CompiledShader::SetUniform(OglPtr::ShaderUniform ptr, const glm::bvec##n & value) const \
    { \
        glProgramUniform##n##ui(programHandle.Get(), ptr.Get(), (GLuint)singleValue); \
    } \
    void CompiledShader::SetUniformArray(OglPtr::ShaderUniform ptr, GLsizei count, \
                                         const glm::bvec##n * value) const \
    { \
        threadData.uniformBoolBuffer.resize(count * glm::bvec##n::length()); \
        size_t bufferIndex = 0; \
        for (GLsizei i = 0; i < count; ++i) \
            for (glm::length_t axis = 0; axis < glm::bvec##n::length(); ++axis) \
                threadData.uniformBoolBuffer[bufferIndex++] = (GLuint)value[i][axis]; \
        \
        glProgramUniform##n##uiv(programHandle.Get(), ptr.Get(), count, \
                                 threadData.uniformBoolBuffer.data()); \
    }

UNIFORM_SETTER_BOOLVEC(1, value.x);
UNIFORM_SETTER_BOOLVEC(2, value.x BP_COMMA(GLuint)value.y);
UNIFORM_SETTER_BOOLVEC(3, value.x BP_COMMA(GLuint)value.y BP_COMMA(GLuint)value.z);
UNIFORM_SETTER_BOOLVEC(4, value.x BP_COMMA (GLuint)value.y BP_COMMA (GLuint)value.z BP_COMMA (GLuint)value.w);

#undef UNIFORM_SETTER_BOOLVEC

#pragma endregion

#pragma region SetUniform() for all other primitives/vectors

#define UNIFORM_SETTER_VECTORS(primitiveType, glType, glmLetter, glFuncLetters) \
    UNIFORM_SETTER(primitiveType, primitiveType, 1##glFuncLetters, 1##glFuncLetters##v, \
                   (glType)value, values) \
    UNIFORM_SETTER(glm::glmLetter##vec1, const glm::glmLetter##vec1 &, 1##glFuncLetters, 1##glFuncLetters##v, \
                   (glType)value.x, (glType*)values) \
    UNIFORM_SETTER(glm::glmLetter##vec2, const glm::glmLetter##vec2 &, 2##glFuncLetters, 2##glFuncLetters##v, \
                   (glType)value.x BP_COMMA (glType)value.y, \
                   (glType*)glm::value_ptr(values[0])) \
    UNIFORM_SETTER(glm::glmLetter##vec3, const glm::glmLetter##vec3 &, 3##glFuncLetters, 3##glFuncLetters##v, \
                   (glType)value.x BP_COMMA (glType)value.y BP_COMMA (glType)value.z, \
                   (glType*)glm::value_ptr(values[0])) \
    UNIFORM_SETTER(glm::glmLetter##vec4, const glm::glmLetter##vec4 &, 4##glFuncLetters, 4##glFuncLetters##v, \
                   (glType)value.x BP_COMMA (glType)value.y BP_COMMA (glType)value.z BP_COMMA (glType)value.w, \
                   (glType*)glm::value_ptr(values[0]))

UNIFORM_SETTER_VECTORS(int32_t, GLint, i, i)
UNIFORM_SETTER_VECTORS(uint32_t, GLuint, u, ui)
UNIFORM_SETTER_VECTORS(float, GLfloat, f, f)
UNIFORM_SETTER_VECTORS(double, GLdouble, d, d)

#undef UNIFORM_SETTER_VECTORS

#pragma endregion

#pragma region SetUniform() for matrices

#define UNIFORM_SETTER_MATRIX(letter, glmSize, oglSize) \
    UNIFORM_SETTER(glm::letter##mat##glmSize, const glm::letter##mat##glmSize &, \
                   Matrix##oglSize##letter##v, Matrix##oglSize##letter##v, \
                   1 BP_COMMA GL_FALSE BP_COMMA (glm::letter##mat##glmSize::value_type*)glm::value_ptr(value), \
                              GL_FALSE BP_COMMA (glm::letter##mat##glmSize::value_type*)glm::value_ptr(values[0]))

#define UNIFORM_SETTER_MATRICES(glmSize, oglSize) \
    UNIFORM_SETTER_MATRIX(f, glmSize, oglSize) \
    UNIFORM_SETTER_MATRIX(d, glmSize, oglSize)

UNIFORM_SETTER_MATRICES(2x2, 2)
UNIFORM_SETTER_MATRICES(2x3, 2x3)
UNIFORM_SETTER_MATRICES(2x4, 2x4)

UNIFORM_SETTER_MATRICES(3x2, 3x2)
UNIFORM_SETTER_MATRICES(3x3, 3)
UNIFORM_SETTER_MATRICES(3x4, 3x4)

UNIFORM_SETTER_MATRICES(4x2, 4x2)
UNIFORM_SETTER_MATRICES(4x3, 4x3)
UNIFORM_SETTER_MATRICES(4x4, 4)

#undef UNIFORM_SETTER_MATRICES
#undef UNIFORM_SETTER_MATRIX

#pragma endregion

#pragma region SetUniform() for textures

//TODO: Figure out how to set sampler/image uniforms and uniform arrays.
UNIFORM_SETTER(OglPtr::Sampler, OglPtr::Sampler, 1ui, 1uiv,
               value.Get(), &values[0].Get());
UNIFORM_SETTER(OglPtr::Image, OglPtr::Image, 1ui, 1uiv,
               value.Get(), &values[0].Get());


#pragma endregion

#undef UNIFORM_SETTER

#pragma endregion