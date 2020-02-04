#pragma once

#include <optional>
#include <tuple>

#include "../Context.h"

//Provides a way to access a matrix/vector as an array of floats.
#include <glm/gtc/type_ptr.hpp>

//TODO: Integrate this project: https://github.com/dfranx/ShaderDebugger


namespace Bplus::GL
{
    //A specific compiled shader, plus its "uniforms" (a.k.a. parameters).
    class BP_API CompiledShader
    {
    public:
        
        //Gets the currently-active shader program.
        //Returns nullptr if none is active.
        static CompiledShader* GetCurrentActive();

        
        //Compiles and returns an OpenGL shader program with a vertex and fragment shader.
        //If compilation failed, 0 is returned and an error message is written.
        //Otherwise, the result should eventually be cleaned up with glDeleteProgram().
        static OglPtr::ShaderProgram Compile(const char* vertexShader, const char* fragmentShader,
                                             std::string& outErrMsg);
        //Compiles and returns an OpenGL shader program with a vertex, geometry,
        //    and fragment shader.
        //If compilation failed, 0 is returned and an error message is written.
        //Otherwise, the result should be cleaned up with glDeleteProgram().
        static OglPtr::ShaderProgram Compile(const char* vertexShader, const char* geometryShader,
                                             const char* fragmentShader,
                                             std::string& outErrMsg);


        //The render state this shader will use.
        //Note that you can modify these settings at will,
        //    but they only take effect by calling Activate().
        RenderState RenderSettings;
        //The original render settings this shader was created with.
        const RenderState DefaultRenderSettings;


        //Creates a new instance that manages the given shader program with RAII.
        //Also pre-calculates all shader uniform pointers if they're not set already.
        CompiledShader(RenderState defaultRenderSettings,
                       OglPtr::ShaderProgram compiledProgramHandle,
                       const std::vector<std::string>& uniformNames);

        ~CompiledShader();

        //No copying -- there should only be one of each compiled shader object.
        CompiledShader(const CompiledShader& cpy) = delete;
        CompiledShader& operator=(const CompiledShader& cpy) = delete;

        //Support move constructor/assignment.
        CompiledShader(CompiledShader&& src);
        CompiledShader& operator=(CompiledShader&& src);


        //Sets this shader as the active one, meaning that
        //    all future rendering operations are done with it.
        void Activate();

        //Gets whether the given uniform was optimized out of the shader.
        bool WasOptimizedOut(const std::string& uniformName) const
        {
            auto found = uniformPtrs.find(uniformName);
            return (found != uniformPtrs.end()) &&
                   (found->second == OglPtr::ShaderUniform::Null);
        }


        #pragma region Uniform getting

        //Gets a uniform of the given templated type.
        //Valid types are the primitives (32-bit int, 32-bit uint, float, double, and bool),
        //    GLM vectors of the primitives,
        //    GLM matrices of float and double,
        //    OglPtr::Image, and OglPtr::Sampler.
        //Returns std::nullopt if a uniform with the given name doesn't exist.
        //If the shader optimized out the uniform, its "set" value is unknown and
        //    the given default value will be returned.
        template<typename Value_t>
        std::optional<Value_t> GetUniform(const std::string& name,
                                          std::optional<Value_t> defaultIfOptimizedOut = Value_t())
        {
            UniformStates state;
            OglPtr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing: return std::nullopt;
                case UniformStates::OptimizedOut: return defaultIfOptimizedOut;
                case UniformStates::Exists: return GetUniform(ptr);
                default: BPAssert(false, "Unknown uniform ptr state");
            }
        }

        //Gets a uniform of the given templated type.
        //Valid types are the primitives (32-bit int, 32-bit uint, float, double, and bool),
        //    GLM vectors of the primitives,
        //    GLM matrices of float and double,
        //    OglPtr::Image, and OglPtr::Sampler.
        //Returns std::nullopt if a uniform with the given name doesn't exist.
        //If the shader optimized out the uniform, its "set" value is unknown and
        //    the given default value will be returned.
        template<typename Value_t>
        std::optional<Value_t> GetUniformArrayElement(const std::string& name,
                                                      OglPtr::ShaderProgram::Data_t index,
                                                      std::optional<Value_t> defaultIfOptimizedOut = Value_t())
        {
            UniformStates state;
            OglPtr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing: return std::nullopt;
                case UniformStates::OptimizedOut: return defaultIfOptimizedOut;

                case UniformStates::Exists:
                    return GetUniform(OglPtr::ShaderUniform(ptr.Get() + index));

                default: BPAssert(false, "Unknown uniform ptr state");
            }
        }

        //Gets a uniform of the given templated type.
        //Valid types are the primitives (32-bit int, 32-bit uint, float, double, and bool),
        //    GLM vectors of the primitives,
        //    GLM matrices of float and double,
        //    OglPtr::Image, and OglPtr::Sampler.
        //Returns true if everything went fine and the data was copied into "outData".
        //Returns false if the uniform didn't exist or was optimized out by the shader.
        template<typename Value_t>
        bool GetUniformArray(const std::string& name,
                             OglPtr::ShaderUniform::Data_t count,
                             Value_t* outData)
        {
            UniformStates state;
            OglPtr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing:
                    return false;
                case UniformStates::OptimizedOut:
                    return false;

                case UniformStates::Exists:
                    for (decltype(count) i = 0; i < count; ++i)
                    {
                        OglPtr::ShaderUniform elementPtr{ ptr.Get() +
                                                          (OglPtr::ShaderUniform::Data_t)i };
                        outData[i] = GetUniform(elementPtr);
                    }
                    return true;

                default:
                    BPAssert(false, "Unknown uniform ptr state");
                    return false;
            }
        }

        #pragma endregion

        //All SetUniform() calls return true if the uniform was successfully set,
        //    or false if the uniform wasn't found.
        //If the uniform exists but was optimized out of the shader,
        //    nothing is done and true is returned.
        #pragma region Uniform setting via SetUniform()

        //TODO: Change to a single templated function like GetUniform()

    #define UNIFORM_SETTER_SINGLE(inType) \
        public: \
        bool SetUniform(const std::string& name, inType value) const \
        { \
            UniformStates state; \
            OglPtr::ShaderUniform ptr; \
            std::tie(state, ptr) = CheckUniform(name); \
            switch (state) { \
                case UniformStates::Missing: return false; \
                case UniformStates::OptimizedOut: return true; \
                case UniformStates::Exists: \
                    SetUniform(ptr, value); \
                    return true; \
                default: BPAssert(false, "Unknown UniformStates value"); return false; \
            } \
        } \
        private: \
        void SetUniform(OglPtr::ShaderUniform ptr, inType value) const; \
        public:

    #define UNIFORM_SETTER_ARRAY(inType) \
        bool SetUniformArray(const std::string& name, GLsizei count, const inType *values) const \
        { \
            UniformStates state; \
            OglPtr::ShaderUniform ptr; \
            std::tie(state, ptr) = CheckUniform(name); \
            switch (state) { \
                case UniformStates::Missing: return false; \
                case UniformStates::OptimizedOut: return true; \
                case UniformStates::Exists: \
                    SetUniformArray(ptr, count, values); \
                    return true; \
                default: BPAssert(false, "Unknown UniformStates value"); return false; \
            } \
        } \
        private: \
        void SetUniformArray(OglPtr::ShaderUniform ptr, GLsizei count, const inType *values) const; \
        public:

    #define UNIFORM_SETTER(inType, inRefType) \
        UNIFORM_SETTER_SINGLE(inRefType) \
        UNIFORM_SETTER_ARRAY(inType)


        #pragma region SetUniform() for primitive/vector types

        //Quick note about bool: because vector<bool> doesn't provide a data() function,
        //    it can be a pain to work with arrays of bool values.
        //To help ameliorate that, this class provies an extra overload
        //    that uses the "Bool" struct, which acts exactly like a bool.
        UNIFORM_SETTER_ARRAY(Bool)

    #define UNIFORM_SETTER_VECTOR(glmLetter, n) \
        UNIFORM_SETTER(glm::glmLetter##vec##n, const glm::glmLetter##vec##n &)

    #define UNIFORM_SETTER_VECTORS(glmLetter, primitive) \
        UNIFORM_SETTER(primitive, primitive) \
        UNIFORM_SETTER_VECTOR(glmLetter, 1) \
        UNIFORM_SETTER_VECTOR(glmLetter, 2) \
        UNIFORM_SETTER_VECTOR(glmLetter, 3) \
        UNIFORM_SETTER_VECTOR(glmLetter, 4)

        UNIFORM_SETTER_VECTORS(b, bool);
        UNIFORM_SETTER_VECTORS(i, GLint);
        UNIFORM_SETTER_VECTORS(u, GLuint);
        UNIFORM_SETTER_VECTORS(f, GLfloat);
        UNIFORM_SETTER_VECTORS(d, double);

    #undef UNIFORM_SETTER_VECTORS
    #undef UNIFORM_SETTER_VECTOR

        #pragma endregion

        #pragma region SetUniform() for matrix types

    #define UNIFORM_SETTER_MATRIX(size) \
        UNIFORM_SETTER(glm::fmat##size, const glm::fmat##size &); \
        UNIFORM_SETTER(glm::dmat##size, const glm::dmat##size &);

    #define UNIFORM_SETTER_MATRICES(nRows) \
        UNIFORM_SETTER_MATRIX(2##x##nRows); \
        UNIFORM_SETTER_MATRIX(3##x##nRows); \
        UNIFORM_SETTER_MATRIX(4##x##nRows)

        UNIFORM_SETTER_MATRICES(2);
        UNIFORM_SETTER_MATRICES(3);
        UNIFORM_SETTER_MATRICES(4);

    #undef UNIFORM_SETTER_MATRICES
    #undef UNIFORM_SETTER_MATRIX

        #pragma endregion

        #pragma region SetUniform() for texture types

        UNIFORM_SETTER(OglPtr::Sampler, OglPtr::Sampler);
        UNIFORM_SETTER(OglPtr::Image, OglPtr::Image);

        #pragma endregion

    #undef UNIFORM_SETTER
        #pragma endregion

    private:

        OglPtr::ShaderProgram programHandle{ OglPtr::ShaderProgram::Null };

        std::unordered_map<std::string, OglPtr::ShaderUniform> uniformPtrs;


        enum class UniformStates { Missing, OptimizedOut, Exists };
        std::tuple<UniformStates, OglPtr::ShaderUniform> CheckUniform(const std::string& name) const;
        
        template<typename Value_t>
        Value_t GetUniform(OglPtr::ShaderUniform ptr)
        {
            BPAssert(ptr != OglPtr::ShaderUniform::Null, "Given a null ptr!");

            //This "if" statement only exists so that all subsequent lines
            //    can start with "} else", to simplify the macros.
            if constexpr (false) { return (void*)nullptr;

            //Each CASE is a new scope, for getting the given type.
            //The last closing brace comes after all this macro stuff.
        #define CASE(T) \
            } else if constexpr (std::is_same_v<T, Value_t>) {


            #pragma region Vector/primitive types

            //Bools are a special case becuause they go through the OpenGL API
            //    as integers or floats.
            #pragma region Bool types
            
            CASE(bool)
                GLuint result;
                glGetUniformuiv(programHandle.Get(), ptr.Get(), &result);
                return (bool)result;

        #define CASE_BOOLVEC(n) \
            CASE(glm::bvec##n) \
                glm::uvec##n result; \
                auto* inputPtr = glm::value_ptr(result); \
                glGetUniformuiv(programHandle.Get(), ptr.Get(), inputPtr); \
                glm::bvec##n returnVal; \
                for (glm::length_t i = 0; i < n; ++i) \
                    returnVal[i] = (bool)result[i]; \
                return returnVal

            CASE_BOOLVEC(1);
            CASE_BOOLVEC(2);
            CASE_BOOLVEC(3);
            CASE_BOOLVEC(4);

        #undef CASE_BOOLVEC

            #pragma endregion


        #define CASE_PRIMITIVE(type, oglLetters, inputExpr) \
            CASE(type) \
                type result; \
                auto* dataPtr = inputExpr; \
                glGetUniform##oglLetters##v(programHandle.Get(), ptr.Get(), dataPtr); \
                return result

        #define CASE_PRIMITIVES(primitive, glmPrefix, oglLetters) \
            CASE_PRIMITIVE(primitive,         oglLetters, &result               ); \
            CASE_PRIMITIVE(glm::glmPrefix##1, oglLetters, glm::value_ptr(result)); \
            CASE_PRIMITIVE(glm::glmPrefix##2, oglLetters, glm::value_ptr(result)); \
            CASE_PRIMITIVE(glm::glmPrefix##3, oglLetters, glm::value_ptr(result)); \
            CASE_PRIMITIVE(glm::glmPrefix##4, oglLetters, glm::value_ptr(result))

            CASE_PRIMITIVES(uint32_t, uvec, ui);
            CASE_PRIMITIVES(int32_t, ivec, i);
            CASE_PRIMITIVES(float, fvec, f);
            CASE_PRIMITIVES(double, dvec, d);

        #undef CASE_PRIMITIVES
        #undef CASE_PRIMITIVE

            #pragma endregion

            #pragma region Matrices

        #define CASE_MATRIX(glmType, oglLetter) \
            CASE(glm::glmType) \
                glm::glmType result; \
                glGetUniform##oglLetter##v(programHandle.Get(), ptr.Get(), \
                                           glm::value_ptr(result)); \
                return result
        #define CASE_MATRIX_DUAL(size) \
            CASE_MATRIX(fmat##size, f); \
            CASE_MATRIX(dmat##size, d)
        #define CASE_MATRICES(nRows) \
            CASE_MATRIX_DUAL(2x##nRows); \
            CASE_MATRIX_DUAL(3x##nRows); \
            CASE_MATRIX_DUAL(4x##nRows)

            CASE_MATRICES(2);
            CASE_MATRICES(3);
            CASE_MATRICES(4);

        #undef CASE_MATRICES
        #undef CASE_MATRIX_DUAL
        #undef CASE_MATRIX

            #pragma endregion

            #pragma region Textures

            //TODO: Figure out how to use bindless textures.
            //CASE(OglPtr::Sampler)

            //TODO: Figure out how to use images.
            //CASE(OglPtr::Image)

            #pragma endregion

        #undef CASE

            //If none of the previous cases apply, then an invalid type was passed.
            } else { static_assert(false); return (void*)nullptr; }
        }
    };
}