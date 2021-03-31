#pragma once

#include <tuple>
#include <memory>

#include "UniformDataStructures.h"
#include "StaticUniforms.h"
#include "ShaderCompileJob.h"

//TODO: Integrate this project: https://github.com/dfranx/ShaderDebugger


//The valid "uniform" types for a CompiledShader are as follows:
//    * Primitive types int32_t, uint32_t, float, double, and bool/Bool
//    * A GLM vector of the above primitive types (up to 4D)
//    * A GLM matrix of floats or doubles (up to 4x4)
//    * Textures (i.e. Bplus::OglPtr::View)
//    * Buffers (i.e. a Bplus::OglPtr::Buffer)
//Any functions that are templated on a type of uniform will accept any of these.


namespace Bplus::GL::Materials
{
    //The base class for B-plus shader definitions.
    //Knows how to create instances of CompiledShader.
    class BP_API Factory;


    //A specific compiled shader, plus its "uniforms" (a.k.a. parameters).
    class BP_API CompiledShader
    {
    public:
        
        //A union of the different types of basic uniform data.
        //Matrix and vector data are stored in the highest-dimensional form
        //    just to keep the variant types simple.
        using UniformElement_t = std::variant<
                  glm::fvec4, glm::dvec4,
                  glm::ivec4, glm::uvec4, glm::bvec4,
                  glm::fmat4x4, glm::dmat4x4,
                  OglPtr::View, OglPtr::Buffer
                >;


        //Creates a new instance that manages a given shader program through RAII.
        //Nulls out the input handle after taking ownership of its contents.
        CompiledShader(Factory& owner,
                       OglPtr::ShaderProgram&& compiledProgramHandle,
                       const Uniforms::UniformDefinitions& uniforms);

        ~CompiledShader();

        //No copying -- there should only be one of each compiled shader object.
        CompiledShader(const CompiledShader& cpy) = delete;
        CompiledShader& operator=(const CompiledShader& cpy) = delete;

        //Support move constructor/assignment.
        CompiledShader(CompiledShader&& src);
        CompiledShader& operator=(CompiledShader&& src);


        //Sets this shader as the active one, meaning that
        //    all future rendering operations are done with it.
        void Activate() const;

        //Gets whether the given uniform was optimized out of the shader.
        bool WasOptimizedOut(const std::string& uniformName) const
        {
            auto found = uniformPtrs.find(uniformName);
            return (found != uniformPtrs.end()) &&
                   (found->second == OglPtr::ShaderUniform::null);
        }


        #pragma region Uniform getting

        //Gets a uniform value, of the given templated type.
        //If the shader optimized out the uniform, its current value is undefined and
        //    the given default value will be returned.
        //If the uniform never existed, returns a null value.
        template<typename Value_t>
        std::optional<Value_t> GetUniform(const std::string& name,
                                          std::optional<Value_t> defaultIfOptimizedOut = Value_t{}) const
        {
            auto uniformStatus = CheckUniform(name);
            switch (uniformStatus.Status)
            {
                case UniformStates::Missing:      return std::nullopt;
                case UniformStates::OptimizedOut: return defaultIfOptimizedOut;
                case UniformStates::Exists:       return GetUniform<Value_t>(uniformStatus.Uniform);

                default:
                    BP_ASSERT(false, "Unknown uniform ptr state");
                    return std::nullopt;
            }
        }

        //Gets a uniform of the given templated type.
        //If the shader optimized out the uniform, its current value is undefined and
        //    the given default value will be returned.
        //If the uniform never existed, returns a null value.
        template<typename Value_t>
        std::optional<Value_t> GetUniformArrayElement(const std::string& name,
                                                      OglPtr::ShaderProgram::Data_t index,
                                                      std::optional<Value_t> defaultIfOptimizedOut = Value_t()) const
        {
            auto uniformStatus = CheckUniform(name);
            switch (uniformStatus.Status)
            {
                case UniformStates::Missing: return std::nullopt;
                case UniformStates::OptimizedOut: return defaultIfOptimizedOut;

                case UniformStates::Exists:
                    OglPtr::ShaderUniform elementPtr{ uniformStatus.Uniform.Get() +
                                                        (OglPtr::ShaderUniform::Data_t)index };
                    return GetUniform<Value_t>(elementPtr);

                default:
                    BP_ASSERT(false, "Unknown uniform ptr state");
                    return std::nullopt;
            }
        }

        //Gets a uniform of the given templated type.
        //Returns false if the uniform doesn't exist.
        //If the shader optimized out the uniform,
        //    nothing is done and "true" will be returned as if it exists.
        template<typename Value_t>
        bool GetUniformArray(const std::string& name,
                             OglPtr::ShaderUniform::Data_t count,
                             Value_t* outData) const
        {
            auto uniformStatus = CheckUniform(name);
            switch (uniformStatus.Status)
            {
                case UniformStates::Missing:
                    return false;
                case UniformStates::OptimizedOut:
                    return true;

                case UniformStates::Exists:
                    for (decltype(count) i = 0; i < count; ++i)
                    {
                        OglPtr::ShaderUniform elementPtr{ uniformStatus.Uniform.Get() +
                                                          (OglPtr::ShaderUniform::Data_t)i };
                        outData[i] = GetUniform<Value_t>(elementPtr);
                    }
                    return true;

                default:
                    BP_ASSERT(false, "Unknown uniform ptr state");
                    return false;
            }
        }

        #pragma endregion

        #pragma region Uniform setting
        
        //Sets a uniform of the given templated type.
        //Returns false if the uniform doesn't exist.
        //If the shader optimized out the uniform,
        //    nothing is done and "true" will be returned as if it exists.
        template<typename Value_t>
        bool SetUniform(const std::string& name, const Value_t& value)
        {
            auto uniformStatus = CheckUniform(name);
            switch (uniformStatus.Status)
            {
                case UniformStates::Missing: return false;
                case UniformStates::OptimizedOut: return true;

                case UniformStates::Exists:
                    _SetUniform(uniformStatus.Uniform, value);
                    return true;

                default:
                    BP_ASSERT(false, "Unknown uniform ptr state");
                    return false;
            }
        }

        //Sets a uniform of the given templated type.
        //Returns false if the uniform doesn't exist.
        //If the shader optimized out the uniform,
        //    nothing is done and "true" will be returned as if it exists.
        template<typename Value_t>
        bool SetUniformArrayElement(const std::string& name, int index,
                                    const Value_t& value)
        {
            auto uniformStatus = CheckUniform(name);
            switch (uniformStatus.Status)
            {
                case UniformStates::Missing: return false;
                case UniformStates::OptimizedOut: return true;

                case UniformStates::Exists:
                    OglPtr::ShaderUniform elementPtr{ uniformStatus.Uniform.Get() +
                                                        (OglPtr::ShaderUniform::Data_t)index };
                    _SetUniform(elementPtr, value);
                    return true;

                default:
                    BP_ASSERT(false, "Unknown uniform ptr state");
                    return false;
            }
        }

        //Sets a uniform of the given templated type.
        //Returns false if the uniform doesn't exist.
        //If the shader optimized out the uniform,
        //    nothing is done and "true" will be returned as if it exists.
        template<typename Value_t>
        bool SetUniformArray(const std::string& name, int count, const Value_t* data)
        {
            auto uniformStatus = CheckUniform(name);
            switch (uniformStatus.Status)
            {
                case UniformStates::Missing:
                    return false;
                case UniformStates::OptimizedOut:
                    return true;

                case UniformStates::Exists:
                    for (int i = 0; i < count; ++i)
                    {
                        OglPtr::ShaderUniform elementPtr{ uniformStatus.Uniform.Get() +
                                                            (OglPtr::ShaderUniform::Data_t)i };
                        _SetUniform(elementPtr, data[i]);
                    }
                    return true;

                default:
                    BP_ASSERT(false, "Unknown uniform ptr state");
                    return false;
            }
        }

        #pragma endregion

    private:

        OglPtr::ShaderProgram programHandle;
        Factory* owner;

        //Stores "null" for uniforms that have been optimized out.
        //This allows the class to distinguish between incorrect uniform names
        //    and uniforms that the shader just doesn't use.
        std::unordered_map<std::string, OglPtr::ShaderUniform> uniformPtrs;

        //Stores the current value for each uniform.
        std::unordered_map<OglPtr::ShaderUniform, UniformElement_t> uniformValues;
        using UniformValueEntry       = decltype(uniformValues)::      iterator;
        using UniformValueEntry_const = decltype(uniformValues)::const_iterator;


        enum class UniformStates { Missing, OptimizedOut, Exists };
        struct UniformAndStatus {   OglPtr::ShaderUniform Uniform;    UniformStates Status;    };
        UniformAndStatus CheckUniform(const std::string& name) const;
        
        template<typename Value_t>
        auto GetUniform(OglPtr::ShaderUniform ptr) const
        {
            BP_ASSERT(!ptr.IsNull(), "Given an null uniform location!");

            auto found = uniformValues.find(ptr);
            BP_ASSERT_STR(found != uniformValues.end(),
                          "Nonexistent uniform pointer: " + std::to_string(ptr.Get()));
            return _GetUniform<Value_t>()(found);
        }
        #pragma region _GetUniform()

        //Gets a uniform's value, templated on the type of uniform data.
        //The non-specialized version gives you a compile error
        //    about using an incorrect template type.
        //Implemented as a functor class instead of a function,
        //    to get partial template specialization.
        template<typename Value_t>
        struct _GetUniform
        {
            Value_t operator()(const UniformValueEntry_const& ptr)
            {
                static_assert(false, "Invalid 'T' for GetUniform<T>()");
            }
        };


    #define GET_UNIFORM_FOR(Type) \
        template<> struct _GetUniform<Type> { Type operator()(const UniformValueEntry_const& ptr) const {
    #define GET_UNIFORM_FOR_END } };

        #pragma region Primitives and vectors
        
        template<glm::length_t L, typename T, glm::qualifier Q>
        struct _GetUniform<glm::vec<L, T, Q>>
        {
            glm::vec<L, T, Q> operator()(const UniformValueEntry_const& ptr) const
            {
                using Output_t = glm::vec<L, T, Q>;
                using Storage_t = glm::vec<4, T>;

                BP_ASSERT(std::holds_alternative<Storage_t>(ptr->second),
                          "Uniform isn't the expected primitive/vector type");

                return (Output_t)std::get<Storage_t>(ptr->second);
            }
        };
        
        //The primitive versions just call into the 1D vector versions.
        GET_UNIFORM_FOR(float)    return _GetUniform<glm::fvec1>()(ptr).x; GET_UNIFORM_FOR_END
        GET_UNIFORM_FOR(double)   return _GetUniform<glm::dvec1>()(ptr).x; GET_UNIFORM_FOR_END
        GET_UNIFORM_FOR(glm::i32) return _GetUniform<glm::ivec1>()(ptr).x; GET_UNIFORM_FOR_END
        GET_UNIFORM_FOR(glm::u32) return _GetUniform<glm::uvec1>()(ptr).x; GET_UNIFORM_FOR_END
        GET_UNIFORM_FOR(bool)     return _GetUniform<glm::bvec1>()(ptr).x; GET_UNIFORM_FOR_END
        GET_UNIFORM_FOR(Bool)     return _GetUniform<glm::bvec1>()(ptr).x; GET_UNIFORM_FOR_END

        #pragma endregion

        #pragma region Matrices
        
        template<glm::length_t C, glm::length_t R, typename T>
        struct _GetUniform<glm::mat<C, R, T>>
        {
            glm::mat<C, R, T> operator()(const UniformValueEntry_const& ptr) const
            {
                using Storage_t = glm::mat<4, 4, T>;

                BP_ASSERT(std::holds_alternative<Storage_t>(ptr->second),
                          "Uniform isn't the expected matrix type");

                return Math::Resize<C, R>(std::get<Storage_t>(ptr->second));
            }
        };

        #pragma endregion

        #pragma region Textures and Buffers
        
        GET_UNIFORM_FOR(OglPtr::View)
            BP_ASSERT(std::holds_alternative<OglPtr::View>(ptr->second),
                      "Uniform isn't a texture (or image) as expected");

            return std::get<OglPtr::View>(ptr->second);
        GET_UNIFORM_FOR_END

        GET_UNIFORM_FOR(OglPtr::Buffer)
            BP_ASSERT(std::holds_alternative<OglPtr::Buffer>(ptr->second),
                      "Uniform isn't a buffer as expected");

            return std::get<OglPtr::Buffer>(ptr->second);
        GET_UNIFORM_FOR_END

        #pragma endregion

    #undef GET_UNIFORM_FOR
    #undef GET_UNIFORM_FOR_END

        #pragma endregion

        template<typename Value_t>
        void SetUniform(OglPtr::ShaderUniform ptr, const Value_t& value)
        {
            BP_ASSERT(!ptr.IsNull(), "Given a null uniform location!");

            auto found = uniformValues.find(ptr);
            BP_ASSERT_STR(found != uniformValues.end(),
                          "Nonexistent uniform pointer: " + std::to_string(ptr.Get()));
            _SetUniform<Value_t>(found, programHandle, value);
        }
        #pragma region _SetUniform()

        //The non-specialized version gives you a compile error
        //    about using an incorrect template type.
        //Implemented as a functor class instead of a function,
        //    to get partial template specialization.
        template<typename Value_t>
        struct _SetUniform
        {
            void operator()(const UniformValueEntry& ptr,
                            OglPtr::ShaderProgram programHandle,
                            const Value_t& value)
            {
                static_assert(false, "Invalid type used for SetUniform()");
            }
        };

    #define SET_UNIFORM_FOR(Type) \
        template<> struct _SetUniform<Type> { void operator()(const UniformValueEntry& ptr, \
                                                              OglPtr::ShaderProgram programHandle, \
                                                              const Type& value) const {
    #define SET_UNIFORM_FOR_END } };

        #pragma region Primitives and Vectors

    #define SET_UNIFORM_VECTORS(PrimitiveType, glFuncLetters) \
        SET_UNIFORM_FOR(PrimitiveType                     ) glProgramUniform1##glFuncLetters(programHandle.Get(), ptr->first.Get(), value); SET_UNIFORM_FOR_END \
        SET_UNIFORM_FOR(glm::vec<1 BP_COMMA PrimitiveType>) glProgramUniform1##glFuncLetters(programHandle.Get(), ptr->first.Get(), value.x); SET_UNIFORM_FOR_END \
        SET_UNIFORM_FOR(glm::vec<2 BP_COMMA PrimitiveType>) glProgramUniform2##glFuncLetters(programHandle.Get(), ptr->first.Get(), value.x, value.y); SET_UNIFORM_FOR_END \
        SET_UNIFORM_FOR(glm::vec<3 BP_COMMA PrimitiveType>) glProgramUniform3##glFuncLetters(programHandle.Get(), ptr->first.Get(), value.x, value.y, value.z); SET_UNIFORM_FOR_END \
        SET_UNIFORM_FOR(glm::vec<4 BP_COMMA PrimitiveType>) glProgramUniform4##glFuncLetters(programHandle.Get(), ptr->first.Get(), value.x, value.y, value.z, value.w); SET_UNIFORM_FOR_END

        SET_UNIFORM_VECTORS(glm::u32, ui);
        SET_UNIFORM_VECTORS(glm::i32, i);
        SET_UNIFORM_VECTORS(float, f);
        SET_UNIFORM_VECTORS(double, d);
        //Booleans need special handling, done below.
    #undef SET_UNIFORM_VECTORS

        //Bool, bool, and vectors of those:
        SET_UNIFORM_FOR(bool) _SetUniform<glm::u32>()(ptr, programHandle, value ? 1 : 0); SET_UNIFORM_FOR_END
        SET_UNIFORM_FOR(Bool) _SetUniform<glm::u32>()(ptr, programHandle, value ? 1 : 0); SET_UNIFORM_FOR_END
        SET_UNIFORM_FOR(glm::bvec2) _SetUniform<glm::uvec2>()(ptr, programHandle, glm::uvec2{ value[0] ? 1 : 0,
                                                                                              value[1] ? 1 : 0 }); SET_UNIFORM_FOR_END
        SET_UNIFORM_FOR(glm::bvec3) _SetUniform<glm::uvec3>()(ptr, programHandle, glm::uvec3{ value[0] ? 1 : 0,
                                                                                              value[1] ? 1 : 0,
                                                                                              value[2] ? 1 : 0 }); SET_UNIFORM_FOR_END
        SET_UNIFORM_FOR(glm::bvec4) _SetUniform<glm::uvec4>()(ptr, programHandle, glm::uvec4{ value[0] ? 1 : 0,
                                                                                              value[1] ? 1 : 0,
                                                                                              value[2] ? 1 : 0,
                                                                                              value[3] ? 1 : 0 }); SET_UNIFORM_FOR_END
        
        #pragma endregion

        #pragma region Matrices

    #define SET_UNIFORM_MATRICES(NCols, NRows, glRowCol) \
        SET_UNIFORM_FOR(glm::mat<NCols BP_COMMA NRows BP_COMMA float>) \
            glProgramUniformMatrix##glRowCol##fv(programHandle.Get(), ptr->first.Get(), \
                                                 1, GL_FALSE, glm::value_ptr(value)); \
        SET_UNIFORM_FOR_END \
        SET_UNIFORM_FOR(glm::mat<NCols BP_COMMA NRows BP_COMMA double>) \
            glProgramUniformMatrix##glRowCol##dv(programHandle.Get(), ptr->first.Get(), \
                                                 1, GL_FALSE, glm::value_ptr(value)); \
        SET_UNIFORM_FOR_END
        
        SET_UNIFORM_MATRICES(2, 2, 2)
        SET_UNIFORM_MATRICES(2, 3, 2x3)
        SET_UNIFORM_MATRICES(2, 4, 2x4)
        SET_UNIFORM_MATRICES(3, 2, 3x2)
        SET_UNIFORM_MATRICES(3, 3, 3)
        SET_UNIFORM_MATRICES(3, 4, 3x4)
        SET_UNIFORM_MATRICES(4, 2, 4x2)
        SET_UNIFORM_MATRICES(4, 3, 4x3)
        SET_UNIFORM_MATRICES(4, 4, 4)
    #undef SET_UNIFORM_MATRICES

        #pragma endregion

        #pragma region Textures and Buffers

        SET_UNIFORM_FOR(OglPtr::View)
            glProgramUniform1ui64ARB(programHandle.Get(), ptr->first.Get(),
                                     value.Get());
        SET_UNIFORM_FOR_END

        SET_UNIFORM_FOR(OglPtr::Buffer)
            glProgramUniform1ui64ARB(programHandle.Get(), ptr->first.Get(), value.Get());
        SET_UNIFORM_FOR_END

        #pragma endregion

    #undef SET_UNIFORM_FOR
    #undef SET_UNIFORM_FOR_END

        #pragma endregion
    };
}