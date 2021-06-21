#pragma once

#include <span>

#include "Factory.h"

namespace Bplus::GL::Materials
{
    //A specific instance of a shader, with custom parameter values.
    //The shader is loaded from a Factory.
    class Material
    {
        //TODO: Unit tests that focus on the setting/getting of array uniforms, especially out-of-order and with switching variants.

    public:

        //Gets the factory that manages this Material's shader variants.
        Factory& GetFactory() const { return *factory; }


        Material(Factory& factory) : factory(&factory), currentVariant(nullptr), isActive(false) { }
        virtual ~Material() { }


        //The different types of single values a uniform can be.
        using UniformElement_t = std::variant<
            //Float scalar/vector:
            float, glm::fvec1, glm::fvec2, glm::fvec3, glm::fvec4,
            //Double scalar/vector:
            double, glm::dvec1, glm::dvec2, glm::dvec3, glm::dvec4,
            //Int scalar/vector:
            int32_t, glm::ivec1, glm::ivec2, glm::ivec3, glm::ivec4,
            //UInt scalar/vector:
            uint32_t, glm::uvec1, glm::uvec2, glm::uvec3, glm::uvec4,
            //Bool scalar/vector:
            bool, glm::bvec1, glm::bvec2, glm::bvec3, glm::bvec4,
            //Float matrix:
            glm::fmat2, glm::fmat2x3, glm::fmat2x4,
            glm::fmat3x2, glm::fmat3, glm::fmat3x4,
            glm::fmat4x2, glm::fmat4x3, glm::fmat4,
            //Double matrix:
            glm::dmat2, glm::dmat2x3, glm::dmat2x4,
            glm::dmat3x2, glm::dmat3, glm::dmat3x4,
            glm::dmat4x2, glm::dmat4x3, glm::dmat4,
            //Resources:
            OglPtr::View, OglPtr::Buffer
        >;

        //A uniform is either a value, or an array of values.
        //Uniform structs are not visible here; we focus on their fields individually.
        using UniformValue_t = std::variant<std::vector<UniformElement_t>,
                                            UniformElement_t>;


        //Sets the given single uniform.
        //If setting an element of a uniform array, pass the index as well.
        //To set an entire array, use "SetParamArray()".
        inline void SetParam(const std::string& name, const UniformElement_t& value,
                             std::optional<uint_fast32_t> arrayIndex = std::nullopt)
        {
            //We need to check all the different possible types of the value,
            //    and then dispatch to the templated SetParam<T>().
            //Use macros to cut down on the massive number of different types.

            //Add a meaningless "if" statement first to make the macros simpler.
            if (false) {}

        #define SET_PARAM_BASIC(T) \
            else if (std::holds_alternative<T>(value)) \
                SetParam(name, std::get<T>(value), arrayIndex)

        #define SET_PARAM_VEC(T) \
            SET_PARAM_BASIC(T); \
            SET_PARAM_BASIC(glm::vec<1 BP_COMMA T>); \
            SET_PARAM_BASIC(glm::vec<2 BP_COMMA T>); \
            SET_PARAM_BASIC(glm::vec<3 BP_COMMA T>); \
            SET_PARAM_BASIC(glm::vec<4 BP_COMMA T>)

            SET_PARAM_VEC(float);
            SET_PARAM_VEC(double);
            SET_PARAM_VEC(int32_t);
            SET_PARAM_VEC(uint32_t);
            SET_PARAM_VEC(bool);
        #undef SET_PARAM_VEC

        #define SET_PARAM_MAT(C, R) \
            SET_PARAM_BASIC(glm::mat<C BP_COMMA R BP_COMMA float>); \
            SET_PARAM_BASIC(glm::mat<C BP_COMMA R BP_COMMA double>)

            SET_PARAM_MAT(2, 2); SET_PARAM_MAT(3, 2); SET_PARAM_MAT(4, 2);
            SET_PARAM_MAT(2, 3); SET_PARAM_MAT(3, 3); SET_PARAM_MAT(4, 3);
            SET_PARAM_MAT(2, 4); SET_PARAM_MAT(3, 4); SET_PARAM_MAT(4, 4);
        #undef SET_PARAM_MAT

            SET_PARAM_BASIC(OglPtr::View);
            SET_PARAM_BASIC(OglPtr::Buffer);

        #undef SET_PARAM_BASIC

            //Error-handling for unknown types:
            else
            {
                BP_ASSERT_STR(false,
                              "Unexpected uniform value type, index " +
                                  std::to_string(value.index()));
            }
        }

        //Sets the given single uniform.
        //If setting an element of a uniform array, pass the index as well.
        //To set an entire array, use "SetParamArray()".
        template<typename T>
        void SetParam(const std::string& name, const T& value,
                      std::optional<uint_fast32_t> arrayIndex = std::nullopt)
        {
            bool success;
            if (arrayIndex.has_value())
            {
                auto valueI = arrayIndex.value();

                //Update the uniform in the current variant.
                success = currentVariant->SetUniformArrayElement(name, valueI, value);

                //Remember the new value for all variants.
                auto& vec = std::get<std::vector<UniformElement_t>>(params[name]);
                //If the user is setting this index out-of-order,
                //    we may not be tracking all the elements leading up to this one yet.
                if (valueI > vec.size() - 1)
                {
                    size_t newElementsStartI = vec.size();
                    vec.resize(value + 1);

                    span<UniformElement_t> newData(&vec[newElementsStartI],
                                                   &vec[value]);
                    currentVariant->GetUniformArray(name, newData, newElementsStartI);
                }
                vec[valueI] = value;
            }
            else
            {
                //Update the uniform in the current variant.
                success = currentVariant->SetUniform(name, value);

                //Remember the new value for all variants.
                params[name] = value;
            }

            BP_ASSERT_STR(success,
                          "No uniform named '" + name + "' exists in the Material's shader definitions");
        }

        //Sets the given uniform array.
        //The array elements must be simple types, not a struct!
        //For arrays of structs, you should set the elements and fields
        //    as if they're individual uniforms, with full names like "u_lights[3].color".
        template<typename T>
        void SetParamArray(const std::string& name, const span<T>& values,
                           span<T>::size_type uOffset = 0)
        {
            //Update the uniform in the current variant.
            bool success = currentVariant->SetUniformArray(name, values, uOffset);

            BP_ASSERT_STR(success,
                          "Material uniform '" + name + "' doesn't exist in the shader definitions");

            //Remember the new value for all variants.

            //Get the vector storage for this data, or create it if it doesn't exist yet.
            auto found = params.find(name);
            std::vector<UniformElement_t>* _storedData;
            if (found == params.end())
            {
                params[name] = std::vector<UniformElement_t>{ };
                _storedData = &std::get<decltype(*_storedData)>(params[name]);
            }
            else
            {
                _storedData = &std::get<decltype(*_storedData)>(found->second);
            }
            auto& storedData = *_storedData;

            //Make sure the vector is big enough to hold these elements.
            auto lastElementI = uOffset + values.size() - 1;
            if (lastElementI > storedData.size() - 1)
            {
                size_t oldSize = storedData.size();
                storedData.resize(lastElementI + 1);

                //Read the new elements leading up to the region being set.
                span<UniformElement_t> preSlice(&storedData[oldSize],
                                                &storedData.data()[uOffset]);
                currentVariant->GetUniformArray(name, preSlice, oldSize);
            }

            //Set the vector storage for this data.
            for (decltype(uOffset) i = 0; i < values.size(); ++i)
                storedData[(size_t)(i + uOffset)] = values[i];
        }
        

        //TODO: Finish updating from here down. Watch out for the dynamic filling of vectors in storedData.


        //Fails if the parameter doesn't exist in either
        //    this Material's storage, or the current shader variant.
        template<typename T>
        const T& GetParam(const std::string& name) const
        {
            const T* tryGet = TryGetParam<T>(name);
            BP_ASSERT_STR(tryGet != nullptr,
                          "Material parameter not found: ", name);
            return *tryGet;
        }

        //Returns 'nullptr' if the parameter doesn't exist in either
        //    this Material's storage, or the current shader variant.
        template<typename T>
        const T* TryGetParam(const std::string& name) const
        {
            static_assert(std::is_constructible_v<UniformValue_t, T>,
                          "Calling Material::GetParam() or Material::TryGetParam()"
                            " with an invalid uniform data type. T must be one of the"
                            " types in Material::UniformValue_t");

            auto found = params.find(name);
            if (found != params.end())
                return &std::get<T>(found->second);
            else
                currentVariant->GetUniform;
        }

        #pragma region TryGetParam() implementation

            

        #pragma endregion



    protected:

        void ChangeVariant(CompiledShader& newVariant);

    private:

        CompiledShader* currentVariant;
        Factory* factory;

        //Whether this Material is currently the one that all drawing is done in.
        bool isActive;

        //The current value of all parameters, including ones
        //     that aren't actually used by the current shader variant.
        std::unordered_map<std::string, UniformValue_t> params;
    };
}