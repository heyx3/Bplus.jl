#pragma once

//Provides a way to access a matrix/vector as an array of floats.
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "../Context.h"


namespace Bplus::GL::Materials
{
    /*
    //Manages the storage of shader parameters, a.k.a. "uniforms".
    //For high-level representation and management of uniform data,
    //    see "UniformManager".
    class BP_API UniformStorage
    {
        //The valid "uniform" types for a shader are as follows:
        //    * Primitive types int32_t, uint32_t, float, double, and bool/Bool
        //    * A GLM vector of the above primitive types (up to 4D)
        //    * A GLM matrix of floats or doubles (up to 4x4)
        //    * Textures (i.e. GL::Textures::TexView and GL::Textures::ImgView)
        //    * Buffers (i.e. a pointer or reference to GL::Buffers::Buffer)
        //Any functions that are templated on a type of uniform should accept any of these.

        //TODO: Track uniform pointers per-program, maybe in an unordered_map<OglPtr::ShaderProgram, unordered_map<...>>
        //TODO: Load all uniforms in constructor, from a shader program: https://stackoverflow.com/questions/440144/in-opengl-is-there-a-way-to-get-a-list-of-all-uniforms-attribs-used-by-a-shade

        //Loads uniforms from the given program.
        UniformStorage(OglPtr::ShaderProgram shader) { AddFromShader(shader); }
        

        //Loads uniforms from the given program.
        void AddFromShader(OglPtr::ShaderProgram shader); //TODO: Make sure to check 'futureValues' afterwards!


        //Gets the pointer for a uniform. Returns Null if it doesn't exist/was optimized out.
        //Using this pointer is a bit more efficient than passing the uniform's name each time.
        //Note that this only works if one of the shader variants using this uniform
        //    has already been compiled.
        OglPtr::ShaderUniform GetPtr(const std::string& name)
        {
            auto found = valuePtrsByName.find(name);
            return (found == valuePtrsByName.end()) ?
                       OglPtr::ShaderUniform::Null() :
                       found->second;
        }


        //Gets the value of a uniform, if it exists/wasn't optimized out.
        template<typename Value_t>
        std::optional<Value_t> GetValue(const std::string& name)
        {
            return GetValue<Value_t>(GetPtr(name));
        }

        //Gets the value of a uniform, if it exists/wasn't optimized out.
        template<typename Value_t>
        std::optional<Value_t> GetValue(OglPtr::ShaderUniform ptr) const { static_assert(false, "Invalid uniform data type for Bplus::GL::Materials::GetValue<T>()"); }
        #pragma region GetValue() Specializations

        //Basic data types:
        #define DEFINE_BASIC_VALUE(T) \
            template<> \
            std::optional<T> GetValue<T>(OglPtr::ShaderUniform ptr) const { \
                auto found = values.find(ptr); \
                if (found == values.end()) \
                   return std::nullopt; \
                const auto& [valueSize, valueStorage] = std::get<UniformVectorStorage_t<T>>(found->second); \
                BP_ASSERT_STR(valueSize == 1, "Expected a single value but it was a " + std::to_string(valueSize) + "D vector"); \
                return valueStorage.x; \
            }
            
            DEFINE_BASIC_VALUE(float);
            DEFINE_BASIC_VALUE(double);
            DEFINE_BASIC_VALUE(glm::u32);
            DEFINE_BASIC_VALUE(glm::i32);
            DEFINE_BASIC_VALUE(bool);
        #undef DEFINE_BASIC_VALUE

        //Vectors:
        template<glm::length_t L, typename T>
        const auto GetValue<glm::vec<L, T>>(OglPtr::ShaderUniform ptr) const
        {
            auto found = values.find(ptr);
            if (found == values.end())
                return std::optional<glm::vec<L, T>>();

            const auto& [valueSize, valueStorage] = std::get<UniformVectorStorage_t<T>>(found->second);
            BP_ASSERT_STR(valueSize == L,
                          "Expected a vector uniform of dimension " +
                              std::to_string(L) + " but it was " + std::to_string(valueSize));

            return std::make_optional<glm::vec<L, T>>(valueStorage);
        }
        //Matrices:
        template<glm::length_t C, glm::length_t R, typename T>
        const auto GetValue<glm::mat<C, R, T>>(OglPtr::ShaderUniform ptr) const
        {
            auto found = values.find(ptr);
            if (found == values.end())
                return std::optional<glm::mat<C, R, T>>();

            const auto& [valueSize, valueStorage] = std::get<UniformMatrixStorage_t<T>>(found->second);
            BP_ASSERT_STR(valueSize.x == C && valueSize.y == R,
                          "Expected a matrix uniform of dimension " +
                              "{" + std::to_string(C) + ", " + std::to_string(R) + "} but got " +
                              "{" + std::to_string(valueSize.x) + ", " + std::to_string(valueSize.y) + "}");

            return std::make_optional(Math::Resize<C, R>(valueStorage));
        }

        #pragma endregion


        //Sets the value of a uniform, if it exists/wasn't optimized out.
        template<typename Value_t>
        void SetValue(const std::string& name, const Value_t& val)
        {
            auto ptr = GetPtr(name);
            if (ptr.IsNull())
                futureValues[name] = ToStorage(val);
            else
                SetValue(ptr, val);
        }

        //Sets the value of the uniforms, if it exists/wasn't optimized out.
        //Passing the 'null' pointer results in a no-op.
        //Passing an unrecognized/invalid pointer is in an error.
        template<typename Value_t>
        void SetValue(OglPtr::ShaderUniform ptr, const Value_t& val) { static_assert(false, "Invalid uniform data type for Bplus::GL::Materials::UniformStorage::SetUniform<T>()") }
        #pragma region SetValue() Specializations

        //Basic data types:
        #define DEFINE_BASIC_VALUE(T) \
            template<> \
            void SetValue<T>(OglPtr::ShaderUniform ptr, const T& val) { \
                auto found = values.find(ptr); \
                if (found == values.end()) { \
                    BP_ASSERT(ptr.IsNull(), "Invalid uniform pointer given to UniformStorage"); \
                } \
                else { \
                    const auto& [valueSize, valueStorage] = std::get<UniformVectorStorage_t<T>>(found->second); \
                    BP_ASSERT_STR(valueSize == 1, "Expected a single value but it was a " + std::to_string(valueSize) + "D vector"); \
                    found->second = ToStorage(val); \
                } \
            }
            
            DEFINE_BASIC_VALUE(float);
            DEFINE_BASIC_VALUE(double);
            DEFINE_BASIC_VALUE(glm::u32);
            DEFINE_BASIC_VALUE(glm::i32);
            DEFINE_BASIC_VALUE(bool);
        #undef DEFINE_BASIC_VALUE
            
            //Vector types:
            template<glm::length_t L, typename T>
            void SetValue<glm::vec<L, T>>(OglPtr::ShaderUniform ptr, const glm::vec<L, T>& val)
            {
                auto found = values.find(ptr);
                if (found == values.end())
                {
                    BP_ASSERT(ptr.IsNull(), "Invalid uniform pointer given to UniformStorage");
                    return;
                }

                const auto& [valueSize, valueStorage] = std::get<UniformVectorStorage_t<T>>(found->second);
                BP_ASSERT_STR(valueSize == L,
                              "Expected a vector uniform of dimension " +
                                  std::to_string(L) + " but it was " + std::to_string(valueSize));
                found->second = ToStorage(val);
            }
            //Matrix types:
            template<glm::length_t C, glm::length_t R, typename T>
            void SetValue<glm::mat<C, R, T>>(OglPtr::ShaderUniform ptr, const glm::mat<C, R, T>& val)
            {
                auto found = values.find(ptr);
                if (found == values.end())
                {
                    BP_ASSERT(ptr.IsNull(), "Invalid uniform pointer given to UniformStorage");
                    return;
                }

                const auto& [valueSize, valueStorage] = std::get<UniformMatrixStorage_t<T>>(found->second);
                BP_ASSERT_STR(valueSize == glm::uvec2{ C, R },
                              "Expected a matrix uniform of dimension " +
                                  "{" + std::to_string(C) + ", " + std::to_string(R) + "}" +
                                  " but it was {" + std::to_string(valueSize.x) + ", " + std::to_string(valueSize.y) + "}");
                found->second = ToStorage(val);
            }

        #pragma endregion


        //TODO: An "Apply to OpenGL program" function. Consider marking each uniform with a bool for whether it needs to be updated.


    private:

        //For simplicity, vectors and matrices are always stored as
        //    their largest-possible version, paired with a value storing their true size.

        template<typename T>
        using UniformVectorStorage_t = std::tuple<glm::length_t, glm::vec<4, T>>;
        template<typename T>
        using UniformMatrixStorage_t = std::tuple<glm::uvec2, glm::mat<4, 4, T>>;

        //TODO: Figure out Buffers and Textures
        //TODO: Figure out int64 and uint64 since we expect that OpenGL extension for 64-bit shader data

        using UniformStorageValue = std::variant<UniformVectorStorage_t<float>,
                                                 UniformVectorStorage_t<double>,
                                                 UniformVectorStorage_t<glm::u32>,
                                                 UniformVectorStorage_t<glm::i32>,
                                                 UniformVectorStorage_t<bool>,
                                                 UniformMatrixStorage_t<float>,
                                                 UniformMatrixStorage_t<double>>;
        //Converts a uniform value into a UniformStorageValue.
        template<typename Value_t>
        static UniformStorageValue ToStorage(const Value_t&) { static_assert(false, "Bplus::GL::Materials::UniformStorage::ToStorage() does not support this type of uniform"); }
        #pragma region ToStorage() specializations
        
        //Basic data types:
        #define SETUP_BASIC_TYPE(T) \
            template<> static UniformStorageValue ToStorage<T>(const T& f) { \
                return UniformVectorStorage_t<T>(1, glm::vec<1, T>{ f }); \
            }

            SETUP_BASIC_TYPE(float);
            SETUP_BASIC_TYPE(double);
            SETUP_BASIC_TYPE(glm::u32);
            SETUP_BASIC_TYPE(glm::i32);
            SETUP_BASIC_TYPE(bool);
        #undef SETUP_BASIC_TYPE

        //Vectors:
        template<glm::length_t L, typename T>
        static UniformStorageValue ToStorage<glm::vec<L, T>>(const glm::vec<L, T>& v)
        {
            return UniformVectorStorage_t<T>(L, v);
        }
        //Matrices:
        template<glm::length_t C, glm::length_t R, typename T>
        static UniformStorageValue ToStorage<glm::mat<C, R, T>>(const glm::mat<C, R, T>& m)
        {

            return UniformMatrixStorage_t<T>(glm::uvec2{ C, R },
                                             Math::Resize<4, 4>(m));
        }

        #pragma endregion

        //Loads the given uniform from the given program,
        //    and adds it to this storage.
        void LoadUniform(OglPtr::ShaderProgram shader, OglPtr::ShaderUniform location,
                         const std::string& name, GLenum type);


        std::unordered_map<std::string, OglPtr::ShaderUniform> valuePtrsByName;
        std::unordered_map<OglPtr::ShaderUniform, UniformStorageValue> values;

        //If the user tried to set a uniform that doesn't exist yet,
        //    we need to remember that in case it turns out to be
        //    from a different shader variant that will eventually be compiled.
        std::unordered_map<std::string, UniformStorageValue> futureValues;
    };
    */
}