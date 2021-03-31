#include "UniformStorage.h"
 
/*

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


namespace
{
    
}


void UniformStorage::AddFromShader(OglPtr::ShaderProgram shader)
{
    GLint nUniforms;
    glGetProgramiv(shader.Get(), GL_ACTIVE_UNIFORMS, &nUniforms);

    const GLsizei maxNameLength = 128;
    GLchar nameBuffer[maxNameLength];
    for (GLint i = 0; i < nUniforms; ++i)
    {
        GLsizei currentNameLength;
        GLint currentSize;
        GLenum currentType;
        glGetActiveUniform(shader.Get(), (GLuint)i,
                           maxNameLength,
                           &currentNameLength, &currentSize, &currentType,
                           nameBuffer);

        LoadUniform(shader, OglPtr::ShaderUniform(i), nameBuffer, currentType);
    }
}

void UniformStorage::LoadUniform(OglPtr::ShaderProgram shader, OglPtr::ShaderUniform location,
                                 const std::string& name, GLenum type)
{
    UniformStorageValue finalValue;
    switch (type)
    {
        #pragma region Vectors

        #pragma region Float

        case GL_FLOAT: {
            float f;
            glGetUniformfv(shader.Get(), location.Get(), &f);
            finalValue = UniformVectorStorage_t<float>(1, { f, glm::fvec3() });
        } break;
        case GL_FLOAT_VEC2: {
            glm::fvec2 v;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<float>(2, { v, glm::fvec2() });
        } break;
        case GL_FLOAT_VEC3: {
            glm::fvec3 v;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<float>(3, { v, 0 });
        } break;
        case GL_FLOAT_VEC4: {
            glm::fvec4 v;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<float>(4, v);
        } break;

        #pragma endregion

        #pragma region Double
            
        case GL_DOUBLE: {
            double f;
            glGetUniformdv(shader.Get(), location.Get(), &f);
            finalValue = UniformVectorStorage_t<double>(1, { f, glm::fvec3() });
        } break;
        case GL_DOUBLE_VEC2: {
            glm::dvec2 v;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<double>(2, { v, glm::dvec2() });
        } break;
        case GL_DOUBLE_VEC3: {
            glm::dvec3 v;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<double>(3, { v, 0 });
        } break;
        case GL_DOUBLE_VEC4: {
            glm::dvec4 v;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<double>(4, v);
        } break;

        #pragma endregion
            
        #pragma region Int

        case GL_INT: {
            glm::i32 i;
            glGetUniformiv(shader.Get(), location.Get(), &i);
            finalValue = UniformVectorStorage_t<glm::i32>(1, { i, glm::ivec3() });
        } break;
        case GL_INT_VEC2: {
            glm::ivec2 v;
            glGetUniformiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<glm::i32>(2, { v, glm::ivec2() });
        } break;
        case GL_INT_VEC3: {
            glm::ivec3 v;
            glGetUniformiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<glm::i32>(3, { v, 0 });
        } break;
        case GL_INT_VEC4: {
            glm::ivec4 v;
            glGetUniformiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<glm::i32>(4, v);
        } break;

        #pragma endregion
            
        #pragma region UInt

        case GL_UNSIGNED_INT: {
            glm::u32 u;
            glGetUniformuiv(shader.Get(), location.Get(), &u);
            finalValue = UniformVectorStorage_t<glm::u32>(1, { u, glm::uvec3() });
        } break;
        case GL_UNSIGNED_INT_VEC2: {
            glm::uvec2 v;
            glGetUniformuiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<glm::u32>(2, { v, glm::uvec2() });
        } break;
        case GL_UNSIGNED_INT_VEC3: {
            glm::uvec3 v;
            glGetUniformuiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<glm::u32>(3, { v, 0 });
        } break;
        case GL_UNSIGNED_INT_VEC4: {
            glm::uvec4 v;
            glGetUniformuiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<glm::i32>(4, v);
        } break;

        #pragma endregion
            
        #pragma region Bool

        case GL_BOOL: {
            glm::u32 u;
            glGetUniformuiv(shader.Get(), location.Get(), &u);
            finalValue = UniformVectorStorage_t<bool>(1, { u != 0, glm::uvec3() });
        } break;
        case GL_BOOL_VEC2: {
            glm::uvec2 v;
            glGetUniformuiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<bool>(2, { glm::notEqual(v, glm::uvec2()), glm::uvec2() });
        } break;
        case GL_BOOL_VEC3: {
            glm::uvec3 v;
            glGetUniformuiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<bool>(3, { glm::notEqual(v, glm::uvec3()), false });
        } break;
        case GL_BOOL_VEC4: {
            glm::uvec4 v;
            glGetUniformuiv(shader.Get(), location.Get(), glm::value_ptr(v));
            finalValue = UniformVectorStorage_t<bool>(4, glm::notEqual(v, glm::uvec4()));
        } break;

        #pragma endregion

        #pragma endregion
            
        #pragma region Matrices

        #pragma region Float

        case GL_FLOAT_MAT2: {
            glm::fmat2 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 2, 2 }, Math::Resize<4, 4>(m));
        } break;
        case GL_FLOAT_MAT2x3: {
            glm::fmat2x3 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 2, 3 }, Math::Resize<4, 4>(m));
        } break;
        case GL_FLOAT_MAT2x4: {
            glm::fmat2x4 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 2, 4 }, Math::Resize<4, 4>(m));
        } break;

        case GL_FLOAT_MAT3x2: {
            glm::fmat3x2 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 3, 2 }, Math::Resize<4, 4>(m));
        } break;
        case GL_FLOAT_MAT3: {
            glm::fmat3 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 3, 3 }, Math::Resize<4, 4>(m));
        } break;
        case GL_FLOAT_MAT3x4: {
            glm::fmat3x4 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 3, 4 }, Math::Resize<4, 4>(m));
        } break;

        case GL_FLOAT_MAT4x2: {
            glm::fmat4x2 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 4, 2 }, Math::Resize<4, 4>(m));
        } break;
        case GL_FLOAT_MAT4x3: {
            glm::fmat4x3 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 4, 3 }, Math::Resize<4, 4>(m));
        } break;
        case GL_FLOAT_MAT4: {
            glm::fmat4 m;
            glGetUniformfv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<float>(glm::uvec2{ 4, 4 }, m);
        } break;

        #pragma endregion
            
        #pragma region Double

        case GL_DOUBLE_MAT2: {
            glm::dmat2 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 2, 2 }, Math::Resize<4, 4>(m));
        } break;
        case GL_DOUBLE_MAT2x3: {
            glm::dmat2x3 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 2, 3 }, Math::Resize<4, 4>(m));
        } break;
        case GL_DOUBLE_MAT2x4: {
            glm::dmat2x4 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 2, 4 }, Math::Resize<4, 4>(m));
        } break;

        case GL_DOUBLE_MAT3x2: {
            glm::dmat3x2 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 3, 2 }, Math::Resize<4, 4>(m));
        } break;
        case GL_DOUBLE_MAT3: {
            glm::dmat3 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 3, 3 }, Math::Resize<4, 4>(m));
        } break;
        case GL_DOUBLE_MAT3x4: {
            glm::dmat3x4 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 3, 4 }, Math::Resize<4, 4>(m));
        } break;

        case GL_DOUBLE_MAT4x2: {
            glm::dmat4x2 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 4, 2 }, Math::Resize<4, 4>(m));
        } break;
        case GL_DOUBLE_MAT4x3: {
            glm::dmat4x3 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 4, 3 }, Math::Resize<4, 4>(m));
        } break;
        case GL_DOUBLE_MAT4: {
            glm::dmat4 m;
            glGetUniformdv(shader.Get(), location.Get(), glm::value_ptr(m));
            finalValue = UniformMatrixStorage_t<double>(glm::uvec2{ 4, 4 }, m);
        } break;

        #pragma endregion

        #pragma endregion

        #pragma region Textures, Images, Buffers

        //TODO: Missing images and atmoic ints.

        case GL_SAMPLER_1D:
        case GL_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_SAMPLER_1D_SHADOW:
        case GL_SAMPLER_2D:
        case GL_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
        case GL_SAMPLER_CUBE:
        case GL_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_SAMPLER_CUBE_SHADOW:
            //Not implemented yet.
            BP_ASSERT(false, "Texture/sampler uniform types not supported yet");
        break;

        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            //Not implemented yet.
            BP_ASSERT(false, "Texture/sampler uniform types not supported yet");
            break;

        //Some types are effectively obsolete.
        case GL_SAMPLER_1D_ARRAY:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
        case GL_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            BP_ASSERT(false,
                      "Texture arrays not supported in B+, because we have bindless textures");
        break;
        case GL_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            BP_ASSERT(false,
                      "Buffer samplers are not supported in B+. Just read from a buffer the normal way");
        break;
        case GL_SAMPLER_2D_RECT:
        case GL_INT_SAMPLER_2D_RECT:
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
        case GL_SAMPLER_2D_RECT_SHADOW:
            BP_ASSERT(false,
                      "Rect samplers are not supported in B+."
                         " My understanding is that they're an old feature"
                         " and not very common anymore.");
        break;

        #pragma endregion
    }
}

*/