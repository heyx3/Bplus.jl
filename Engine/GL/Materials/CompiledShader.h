#pragma once

#include <tuple>

#include "../Textures/Texture.h"
#include "ShaderCompileJob.h"

//Provides a way to access a matrix/vector as an array of floats.
#include <glm/gtc/type_ptr.hpp>

//TODO: Integrate this project: https://github.com/dfranx/ShaderDebugger


//The valid "uniform" types for a shader are as follows:
//    * Primitive types int32_t, uint32_t, float, double, and bool/Bool
//    * A GLM vector of the above primitive types (up to 4D)
//    * A GLM matrix of floats or doubles (up to 4x4)
//    * Textures (i.e. Bplus::GL::Texture::TexView and ::ImgView)
//    * Buffers (i.e. a pointer or reference to Bplus::GL::Buffers::Buffer)
//Any functions that are templated on a type of uniform will accept any of these.


namespace Bplus::GL
{
    namespace Buffers { class Buffer; }

    //A specific compiled shader, plus its "uniforms" (a.k.a. parameters).
    class BP_API CompiledShader
    {
    public:
        
        //Gets the currently-active shader program.
        //Returns nullptr if none is active.
        static const CompiledShader* GetCurrentActive();

        //Gets the shader with the given OpenGL pointer.
        //Returns nullptr if it couldn't be found.
        static const CompiledShader* Find(OglPtr::ShaderProgram ptr);


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
        void Activate() const;

        //Gets whether the given uniform was optimized out of the shader.
        bool WasOptimizedOut(const std::string& uniformName) const
        {
            auto found = uniformPtrs.find(uniformName);
            return (found != uniformPtrs.end()) &&
                   (found->second == OglPtr::ShaderUniform::null);
        }


        #pragma region Uniform getting

        //Gets a uniform of the given templated type.
        //Returns std::nullopt if a uniform with the given name doesn't exist.
        //If the shader optimized out the uniform, its current value is undefined and
        //    the given default value will be returned.
        template<typename Value_t>
        std::optional<Value_t> GetUniform(const std::string& name,
                                          std::optional<Value_t> defaultIfOptimizedOut = Value_t()) const
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
        //Returns std::nullopt if a uniform with the given name doesn't exist.
        //If the shader optimized out the uniform, its "set" value is unknown and
        //    the given default value will be returned.
        template<typename Value_t>
        std::optional<Value_t> GetUniformArrayElement(const std::string& name,
                                                      OglPtr::ShaderProgram::Data_t index,
                                                      std::optional<Value_t> defaultIfOptimizedOut = Value_t()) const
        {
            UniformStates state;
            OglPtr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing: return std::nullopt;
                case UniformStates::OptimizedOut: return defaultIfOptimizedOut;

                case UniformStates::Exists:
                    return GetUniform({ ptr.Get() + index });

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
            UniformStates state;
            OglPtr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing:
                    return false;
                case UniformStates::OptimizedOut:
                    return true;

                case UniformStates::Exists:
                    for (decltype(count) i = 0; i < count; ++i)
                    {
                        OglPtr::ShaderUniform elementPtr{ ptr.Get() +
                                                          (OglPtr::ShaderUniform::Data_t)i };
                        outData[i] = GetUniform(elementPtr);
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
            UniformAndStatus status = CheckUniform(name);
            switch (status.Status)
            {
                case UniformStates::Missing: return false;
                case UniformStates::OptimizedOut: return true;

                case UniformStates::Exists:
                    _SetUniform(status.Uniform, value);
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
            UniformStates state;
            OglPtr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing: return false;
                case UniformStates::OptimizedOut: return true;

                case UniformStates::Exists:
                    _SetUniform({ ptr.Get() + (OglPtr::ShaderUniform::Data_t)index },
                                value);
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
            UniformStates state;
            OglPtr::ShaderUniform ptr;
            std::tie(state, ptr) = CheckUniform(name);
            switch (state)
            {
                case UniformStates::Missing:
                    return false;
                case UniformStates::OptimizedOut:
                    return true;

                case UniformStates::Exists:
                    for (int i = 0; i < count; ++i)
                    {
                        OglPtr::ShaderUniform elementPtr{ ptr.Get() +
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

        OglPtr::ShaderProgram programHandle = OglPtr::ShaderProgram::Null();

        std::unordered_map<std::string, OglPtr::ShaderUniform> uniformPtrs;

        std::unordered_map<OglPtr::View, Textures::TexView> texViews;
        std::unordered_map<OglPtr::View, Textures::ImgView> imgViews;

        //Tracks how many uniforms use each texture/image view.
        std::unordered_map<OglPtr::View, std::vector<OglPtr::ShaderUniform>> usedViews;


        enum class UniformStates { Missing, OptimizedOut, Exists };
        struct UniformAndStatus {   OglPtr::ShaderUniform Uniform;    UniformStates Status;    };
        UniformAndStatus CheckUniform(const std::string& name) const;
        
        template<typename Value_t>
        auto GetUniform(OglPtr::ShaderUniform ptr) const
        {
            BP_ASSERT(!ptr.IsNull(), "Given an null uniform location!");
            return _GetUniform<Value_t>(ptr);
        }
        #pragma region _GetUniform()

        //Texture views may or may not exist,
        //    so they are returned as a pointer which may be null.
        template<>
        auto GetUniform<Textures::TexView>(OglPtr::ShaderUniform ptr) const
        {
            return GetUniform<const Textures::TexView*>(ptr);
        }
        //Image views may or may not exist,
        //    so they are returned as a pointer which may be null.
        template<>
        auto GetUniform<Textures::ImgView>(OglPtr::ShaderUniform ptr) const
        {
            return GetUniform<const Textures::ImgView*>(ptr);
        }


        //The non-specialized version gives you a compile error
        //    about using an incorrect template type.
        template<typename Value_t>
        Value_t _GetUniform(OglPtr::ShaderUniform ptr) const
        {
            if constexpr (std::is_same_v<Value_t, Buffers::Buffer*>) {
                static_assert(false, "Can't get a non-const pointer to a Buffer through GetUniform()");
            } else if constexpr (std::is_same_v<Value_t, Buffers::Buffer&>) {
                static_assert(false, "Can't get a non-const reference to a Buffer through GetUniform()");
            } else {
                static_assert(false, "Invalid type used for GetUniform()");
            }
        }


        //Use macros to more easily define the specializations.
        
    #define GET_UNIFORM_FOR(Type) \
        template<> Type _GetUniform<Type>(OglPtr::ShaderUniform ptr) const

        #pragma region Primitives and vectors
        
        //Handle all the non-bool primitives/vectors.
        //Bools are special because the OpenGL API requires you to treat them as numbers.
    #define GET_UNIFORM_FOR_PRIM_VECTOR(Type, Count, glLetters) \
        GET_UNIFORM_FOR(glm::vec<Count BP_COMMA Type>) { \
            glm::vec<Count, Type> val; \
            glGetnUniform##glLetters##v(programHandle.Get(), ptr.Get(), \
                                        sizeof(glm::vec<Count, Type>), \
                                        &val[0]); \
            return val; }
    #define GET_UNIFORM_FOR_PRIMITIVE(Type, glLetters) \
        GET_UNIFORM_FOR(Type){ Type val; \
                               glGetnUniform##glLetters##v(programHandle.Get(), ptr.Get(), \
                                                           sizeof(decltype(val)), &val); \
                               return val; } \
        GET_UNIFORM_FOR_PRIM_VECTOR(Type, 1, glLetters) \
        GET_UNIFORM_FOR_PRIM_VECTOR(Type, 2, glLetters) \
        GET_UNIFORM_FOR_PRIM_VECTOR(Type, 3, glLetters) \
        GET_UNIFORM_FOR_PRIM_VECTOR(Type, 4, glLetters)

        GET_UNIFORM_FOR_PRIMITIVE(uint32_t, ui)
        GET_UNIFORM_FOR_PRIMITIVE(int32_t, i)
        GET_UNIFORM_FOR_PRIMITIVE(float, f)
        GET_UNIFORM_FOR_PRIMITIVE(double, d)
    #undef GET_UNIFORM_FOR_PRIMITIVE
    #undef GET_UNIFORM_FOR_PRIM_VECTOR

        //Handle bool, bool vectors, and Bool for good measure.
        GET_UNIFORM_FOR(Bool) { return _GetUniform<bool>(ptr); }
        GET_UNIFORM_FOR(bool) { return _GetUniform<uint32_t>(ptr) != 0; }
        GET_UNIFORM_FOR(glm::bvec2) { return glm::notEqual(_GetUniform<glm::uvec2>(ptr),
                                                           glm::uvec2{}); }
        GET_UNIFORM_FOR(glm::bvec3) { return glm::notEqual(_GetUniform<glm::uvec3>(ptr),
                                                           glm::uvec3{}); }
        GET_UNIFORM_FOR(glm::bvec4) { return glm::notEqual(_GetUniform<glm::uvec4>(ptr),
                                                           glm::uvec4{}); }

        #pragma endregion

        #pragma region Matrices

    #define GET_UNIFORM_FOR_MAT(NCols, NRows, Type, GLType) \
        GET_UNIFORM_FOR(glm::mat<NCols BP_COMMA NRows BP_COMMA Type>) { \
            using mat_t = glm::GLType##mat##NCols##x##NRows; \
            mat_t result; \
            Type* dataPtr = &result[0][0]; \
            glGetnUniform##GLType##v( \
                programHandle.Get(), ptr.Get(), \
                sizeof(mat_t), dataPtr); \
            return result; }
    #define GET_UNIFORM_FOR_MATS(NCols, NRows) \
        GET_UNIFORM_FOR_MAT(NCols, NRows, float, f) \
        GET_UNIFORM_FOR_MAT(NCols, NRows, double, d)
    #define GET_UNIFORM_FOR_MATS_ROWS(NCols) \
        GET_UNIFORM_FOR_MATS(NCols, 2) \
        GET_UNIFORM_FOR_MATS(NCols, 3) \
        GET_UNIFORM_FOR_MATS(NCols, 4)

        GET_UNIFORM_FOR_MATS_ROWS(2)
        GET_UNIFORM_FOR_MATS_ROWS(3)
        GET_UNIFORM_FOR_MATS_ROWS(4)
    #undef GET_UNIFORM_FOR_MATS_ROWS
    #undef GET_UNIFORM_FOR_MATS

        #pragma endregion

        #pragma region Textures and Buffers

        GET_UNIFORM_FOR(const Textures::TexView*)
        {
            OglPtr::View viewPtr;
            glGetnUniformui64vARB(programHandle.Get(), ptr.Get(),
                                  sizeof(viewPtr), &viewPtr.Get());

            if (viewPtr.IsNull())
                return nullptr;

            auto found = texViews.find(viewPtr);
            BP_ASSERT(found != texViews.end(),
                     "Can't find TexView uniform value");
            return &found->second;
        }
        GET_UNIFORM_FOR(const Textures::ImgView*)
        {
            OglPtr::View viewPtr;
            glGetnUniformui64vARB(programHandle.Get(), ptr.Get(),
                                  sizeof(viewPtr), &viewPtr.Get());

            if (viewPtr.IsNull())
                return nullptr;

            auto found = imgViews.find(viewPtr);
            BP_ASSERT(found != imgViews.end(),
                     "Can't find ImgView uniform value");
            return &found->second;
        }

        //TODO: Figure out how to support UBO and SSBO
        //GET_UNIFORM_FOR(const Buffers::Buffer*);
        //GET_UNIFORM_FOR(const Buffers::Buffer&);

        #pragma endregion

    #undef GET_UNIFORM_FOR

        #pragma endregion

        template<typename Value_t>
        void SetUniform(OglPtr::ShaderUniform ptr, const Value_t& value)
        {
            BP_ASSERT(ptr != OglPtr::ShaderUniform::null, "Given a null uniform location!");
            _SetUniform<Value_t>(ptr, value);
        }
        #pragma region _SetUniform()

        //The non-specialized version gives you a compile error
        //    about using an incorrect template type.
        template<typename Value_t>
        void _SetUniform(OglPtr::ShaderUniform ptr, const Value_t& value)
        {
            static_assert(false, "Invalid type used for SetUniform()");
        }


        //Use macros to more easily define the specializations.

    #define SET_UNIFORM_FOR(Type) \
        template<> void _SetUniform<Type>(OglPtr::ShaderUniform ptr, const Type& value)

        #pragma region Primitives and Vectors

    #define SET_UNIFORM_VECTORS(PrimitiveType, glFuncLetters) \
        SET_UNIFORM_FOR(PrimitiveType) { glProgramUniform1##glFuncLetters(programHandle.Get(), ptr.Get(), value); } \
        SET_UNIFORM_FOR(glm::vec<1 BP_COMMA PrimitiveType>) { glProgramUniform1##glFuncLetters(programHandle.Get(), ptr.Get(), value.x); } \
        SET_UNIFORM_FOR(glm::vec<2 BP_COMMA PrimitiveType>) { \
            glProgramUniform2##glFuncLetters(programHandle.Get(), ptr.Get(), value.x, value.y); \
        } \
        SET_UNIFORM_FOR(glm::vec<3 BP_COMMA PrimitiveType>) { \
            glProgramUniform3##glFuncLetters(programHandle.Get(), ptr.Get(), value.x, value.y, value.z); \
        } \
        SET_UNIFORM_FOR(glm::vec<4 BP_COMMA PrimitiveType>) { \
            glProgramUniform4##glFuncLetters(programHandle.Get(), ptr.Get(), value.x, value.y, value.z, value.w); \
        }

        SET_UNIFORM_VECTORS(uint32_t, ui);
        SET_UNIFORM_VECTORS(int32_t, i);
        SET_UNIFORM_VECTORS(float, f);
        SET_UNIFORM_VECTORS(double, d);
        //Booleans need special handling, done below.
    #undef SET_UNIFORM_VECTORS

        //Bool, bool, and vectors of those:
        SET_UNIFORM_FOR(bool) { _SetUniform<uint32_t>(ptr, value ? 1 : 0); }
        SET_UNIFORM_FOR(Bool) { _SetUniform<uint32_t>(ptr, value ? 1 : 0); }
        SET_UNIFORM_FOR(glm::bvec2) { _SetUniform<glm::uvec2>(ptr, glm::uvec2{ value[0] ? 1 : 0,
                                                                               value[1] ? 1 : 0 }); }
        SET_UNIFORM_FOR(glm::bvec3) { _SetUniform<glm::uvec3>(ptr, glm::uvec3{ value[0] ? 1 : 0,
                                                                               value[1] ? 1 : 0,
                                                                               value[2] ? 1 : 0 }); }
        SET_UNIFORM_FOR(glm::bvec4) { _SetUniform<glm::uvec4>(ptr, glm::uvec4{ value[0] ? 1 : 0,
                                                                               value[1] ? 1 : 0,
                                                                               value[2] ? 1 : 0,
                                                                               value[3] ? 1 : 0 }); }
        #pragma endregion

        #pragma region Matrices

    #define SET_UNIFORM_MATRICES(NCols, NRows, glRowCol) \
        SET_UNIFORM_FOR(glm::mat<NCols BP_COMMA NRows BP_COMMA float>) { \
            glProgramUniformMatrix##glRowCol##fv(programHandle.Get(), ptr.Get(), \
                                                 1, GL_FALSE, glm::value_ptr(value)); \
        } \
        SET_UNIFORM_FOR(glm::mat<NCols BP_COMMA NRows BP_COMMA double>) { \
            glProgramUniformMatrix##glRowCol##dv(programHandle.Get(), ptr.Get(), \
                                                 1, GL_FALSE, glm::value_ptr(value)); \
        }
        
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

        #pragma region Textures/Images and Buffers

        SET_UNIFORM_FOR(Textures::TexView)
        {
            const auto* currentView = GetUniform<const Textures::TexView*>(ptr);
            if (currentView != nullptr && *currentView == value)
                return;

            //The old value is no longer associated with this uniform.
            if (currentView != nullptr)
            {
                auto& v = usedViews[currentView->GlPtr];
                v.erase(std::find(v.begin(), v.end(), ptr));

                //If the old value doesn't associate with ANY uniform anymore, stop tracking it.
                if (v.empty())
                    usedViews.erase(currentView->GlPtr);
            }

            texViews.emplace(value.GlPtr, value);
            usedViews[value.GlPtr].push_back(ptr);

            glProgramUniform1ui64ARB(programHandle.Get(), ptr.Get(),
                                     value.GlPtr.Get());
        }
        SET_UNIFORM_FOR(Textures::ImgView)
        {
            const auto* currentView = GetUniform<const Textures::ImgView*>(ptr);
            if (currentView != nullptr && *currentView == value)
                return;

            //The old value is no longer associated with this uniform.
            if (currentView != nullptr)
            {
                auto& v = usedViews[currentView->GlPtr];
                v.erase(std::find(v.begin(), v.end(), ptr));

                //If the old value doesn't associate with ANY uniform anymore, stop tracking it.
                if (v.empty())
                    usedViews.erase(currentView->GlPtr);
            }

            imgViews.emplace(value.GlPtr, value);
            usedViews[value.GlPtr].push_back(ptr);

            glProgramUniform1ui64ARB(programHandle.Get(), ptr.Get(),
                                     value.GlPtr.Get());
        }

        //TODO: Buffers as UBO and SSBO.

        #pragma endregion

    #undef SET_UNIFORM_FOR

        #pragma endregion
    };
}