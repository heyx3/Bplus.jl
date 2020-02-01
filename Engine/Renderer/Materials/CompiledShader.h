#pragma once

#include <optional>
#include <tuple>

#include "../Context.h"

//Provides a way to access a matrix/vector as an array of floats.
#include <glm/gtc/type_ptr.hpp>

//TODO: Integrate this project: https://github.com/dfranx/ShaderDebugger


namespace Bplus::GL
{
    //Various OpenGL handles, wrapped in type-safety.
    namespace Ptr
    {
        //An OpenGL handle to a compiled shader program.
        #pragma region ShaderProgram

        strong_typedef_start(ShaderProgram, GLuint, BP_API)

            static const ShaderProgram Null;

            strong_typedef_equatable(ShaderProgram, GLuint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        const ShaderProgram ShaderProgram::Null = ShaderProgram(0);

        #pragma endregion
        
        //An OpenGL handle to a specific uniform within a shader.
        //If it points to an array of uniforms,
        //    you can get element 'i' in the array by adding 'i' to this value.
        //The byte-size of the uniform has no bearing on this numbering scheme.
        #pragma region ShaderUniform

        strong_typedef_start(ShaderUniform, GLint, BP_API)

            static const ShaderUniform Null;

            strong_typedef_equatable(ShaderUniform, GLint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        const ShaderUniform ShaderUniform::Null = ShaderUniform(-1);

        #pragma endregion

        //An OpenGL handle to a specific sampleable texture.
        //The difference between this and "Image" is that Samplers
        //    have sampling settings (linear, wrap, etc)
        //    while Images are basically plain 2D arrays.
        #pragma region Sampler

        strong_typedef_start(Sampler, GLuint, BP_API)

            static const Sampler Null;

            strong_typedef_equatable(Sampler, GLuint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        const Sampler Sampler::Null = Sampler(0);

        #pragma endregion

        //An OpenGL handle to a specific image buffer.
        //The difference between this and "Sampler" is that Samplers
        //    have sampling settings (linear, wrap, etc)
        //    while Images are basically just 2D arrays.
        #pragma region Image

        strong_typedef_start(Image, GLuint, BP_API)

            //TODO: Check whether 0 is actually the "invalid" handle for an Image.
            //static const Image Null;

            strong_typedef_equatable(Image, GLuint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        //const Image Image::Null = Image(0);

        #pragma endregion
    }


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
        static Ptr::ShaderProgram Compile(const char* vertexShader, const char* fragmentShader,
                                          std::string& outErrMsg);
        //Compiles and returns an OpenGL shader program with a vertex, geometry,
        //    and fragment shader.
        //If compilation failed, 0 is returned and an error message is written.
        //Otherwise, the result should be cleaned up with glDeleteProgram().
        static Ptr::ShaderProgram Compile(const char* vertexShader, const char* geometryShader,
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
                       Ptr::ShaderProgram compiledProgramHandle,
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
                   (found->second == Ptr::ShaderUniform::Null);
        }


        #pragma region Uniform getting

        //Gets a uniform of the given templated type.
        //Valid types are the primitives (32-bit int, 32-bit uint, float, double, and bool),
        //    GLM vectors of the primitives,
        //    GLM matrices of float and double,
        //    Ptr::Image, and Ptr::Sampler.
        //Returns std::nullopt if a uniform with the given name doesn't exist.
        //If the shader optimized out the uniform, its "set" value is unknown and
        //    the given default value will be returned.
        template<typename Value_t>
        std::optional<Value_t> GetUniform(const std::string& name,
                                          std::optional<Value_t> defaultIfOptimizedOut = Value_t())
        {
            UniformStates state;
            Ptr::ShaderUniform ptr;
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
        //    Ptr::Image, and Ptr::Sampler.
        //Returns std::nullopt if a uniform with the given name doesn't exist.
        //If the shader optimized out the uniform, its "set" value is unknown and
        //    the given default value will be returned.
        template<typename Value_t>
        std::optional<Value_t> GetUniformArrayElement(const std::string& name,
                                                      Ptr::ShaderProgram::Data_t index,
                                                      std::optional<Value_t> defaultIfOptimizedOut = Value_t())
        {
            UniformStates state;
            Ptr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing: return std::nullopt;
                case UniformStates::OptimizedOut: return defaultIfOptimizedOut;

                case UniformStates::Exists:
                    return GetUniform(Ptr::ShaderUniform(ptr.Get() + index));

                default: BPAssert(false, "Unknown uniform ptr state");
            }
        }

        //Gets a uniform of the given templated type.
        //Valid types are the primitives (32-bit int, 32-bit uint, float, double, and bool),
        //    GLM vectors of the primitives,
        //    GLM matrices of float and double,
        //    Ptr::Image, and Ptr::Sampler.
        //Returns true if everything went fine and the data was copied into "outData".
        //Returns false if the uniform didn't exist or was optimized out by the shader.
        template<typename Value_t>
        bool GetUniformArray(const std::string& name,
                             Ptr::ShaderUniform::Data_t count,
                             Value_t* outData)
        {
            UniformStates state;
            Ptr::ShaderUniform ptr;
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
                        Ptr::ShaderUniform elementPtr{ ptr.Get() +
                                                       (Ptr::ShaderUniform::Data_t)i };
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

    #define UNIFORM_SETTER(inType, inRefType) \
        public: \
        bool SetUniform(const std::string& name, inRefType value) const \
        { \
            UniformStates state; \
            Ptr::ShaderUniform ptr; \
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
        bool SetUniformArray(const std::string& name, GLsizei count, const inType *values) const \
        { \
            UniformStates state; \
            Ptr::ShaderUniform ptr; \
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
        void SetUniform(Ptr::ShaderUniform ptr, inRefType value) const; \
        void SetUniformArray(Ptr::ShaderUniform ptr, GLsizei count, const inType *values) const; \
        public:

        #pragma region SetUniform() for primitive/vector types

        //Quick note about bool: because vector<bool> doesn't provide a data() function,
        //    it can be a pain to work with arrays of bool values.
        //To help ameliorate that, this class provies an extra overload
        //    that uses the "Bool" struct, which acts exactly like a bool.
        bool SetUniformArray(const std::string& name, GLsizei count, const Bool* elements) const
            { return SetUniformArray(name, count, (bool*)elements); }

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

        UNIFORM_SETTER(Ptr::Sampler, Ptr::Sampler);
        UNIFORM_SETTER(Ptr::Image, Ptr::Image);

        #pragma endregion

        #pragma endregion

    private:

        Ptr::ShaderProgram programHandle = Ptr::ShaderProgram::Null;

        std::unordered_map<std::string, Ptr::ShaderUniform> uniformPtrs;


        enum class UniformStates { Missing, OptimizedOut, Exists };
        std::tuple<UniformStates, Ptr::ShaderUniform> CheckUniform(const std::string& name) const;
        
        template<typename Value_t>
        Value_t GetUniform(Ptr::ShaderUniform ptr)
        {
            BPAssert(ptr != Ptr::ShaderUniform::Null, "Given a null ptr!");

            //This "if" statement only exists so that all subsequent lines
            //    can start with "} else", to simplify the macros.
            if constexpr (false) { return (void*)nullptr;

            //Each CASE is a new scope, for getting the given type.
            //The last closing brace comes after all this macro stuff.
        #define CASE(T) \
            } else if constexpr (std::is_same_v<T, Value_t>) {


            #pragma region Vector/primitive types

        #define CASE_PRIMITIVE(T, glmType, oglLetters, outputExpr) \
            CASE(T) \
                glm::glmType result; \
                glGetUniform##oglLetters##v(programHandle.Get(), uniformPtr.Get(), \
                                            glm::value_ptr(result)); \
                return (T)(outputExpr)

        #define CASE_PRIMITIVES(T, glmPrefix, oglLetters) \
            CASE_PRIMITIVE(T, glmPrefix##1, oglLetters, result.x); \
            CASE_PRIMITIVE(glmPrefix##1, glmPrefix##1, oglLetters, { result.x }); \
            CASE_PRIMITIVE(glmPrefix##2, glmPrefix##2, { result.x BP_COMMA result.y }); \
            CASE_PRIMITIVE(glmPrefix##3, glmPrefix##3, { result.x BP_COMMA result.y BP_COMMA result.z }); \
            CASE_PRIMITIVE(glmPrefix##4, glmPrefix##4, { result.x BP_COMMA result.y BP_COMMA result.z BP_COMMA result.w })

            CASE_PRIMITIVES(bool, uvec, ui);
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
                glGetUniform##oglLetter##v(programHandle.Get(), uniformPtr.Get(), \
                                           glm::value_ptr(result)); \
                return result;
        #define CASE_MATRIX_DUAL(size) \
            CASE_MATRIX(fmat##size, f); \
            CASE_MATRIX(dmat##size, d)
        #define CASE_MATRICES(nRows) \
            CASE_MATRIX_DUAL(2##nRows); \
            CASE_MATRIX_DUAL(3##nRows); \
            CASE_MATRIX_DUAL(4##nRows)

            CASE_MATRICES(2);
            CASE_MATRICES(3);
            CASE_MATRICES(4);

        #undef CASE_MATRICES
        #undef CASE_MATRIX_DUAL
        #undef CASE_MATRIX

            #pragma endregion

            #pragma region Textures

            //TODO: Figure out how to use bindless textures.
            //CASE(Ptr::Sampler)

            //TODO: Figure out how to use images.
            //CASE(Ptr::Image)

            #pragma endregion

        #undef CASE

            //If none of the previous cases apply, then an invalid type was passed.
            } else { static_assert(false); return (void*)nullptr; }
        }
    };
}


//Add hashing support for the various OpenGL handles.
strong_typedef_hashable(Bplus::GL::Ptr::ShaderProgram, GLuint);
strong_typedef_hashable(Bplus::GL::Ptr::ShaderUniform, GLint, BP_API);
strong_typedef_hashable(Bplus::GL::Ptr::Sampler, GLuint, BP_API);
strong_typedef_hashable(Bplus::GL::Ptr::Image, GLuint, BP_API);