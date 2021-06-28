#pragma once

#include <span>

#include "Factory.h"

namespace Bplus::GL::Materials
{
    //A set of uniform parameter values, tied to a set of shaders from a Factory.
    //Different Materials can point to the same Factory, but have different parameter values.
    //The factory is assumed to never be moved after this Material is created.
    class Material
    {
        //TODO: Unit tests that focus on the setting/getting of array uniforms, especially out-of-order and with switching variants.
        //TODO: Statically-track the currently-active material.
        //TODO: Support the sending in of gradient data.

    public:

        //Gets the factory that manages this Material's shader variants.
        Factory& GetFactory() const { return *factory; }


        Material(Factory& factory);
        virtual ~Material() { }


        //TODO: Static uniform getters/setters.


        #pragma region Uniform type aliases

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
            //Special:
            Uniforms::GradientValue_t,
            //Resources:
            const Textures::Texture*, //NOTE: textures are actually given to this class as references,
                                      //   not pointers, due to arcane C++ template rules.
            //TODO: Specific TexHandle and ImgHandle instances
            //TODO: Add Buffers
        >;

        //A uniform is either a value, or an array of values.
        //Note that structs (and arrays of structs) are not visible here;
        //    we store their fields/elements individually.
        using UniformValue_t = std::variant<std::vector<UniformElement_t>,
                                            UniformElement_t>;

        #pragma endregion

        #pragma region Uniform setters

        //Sets the given single uniform.
        //If setting an element of a uniform array, pass the index as well.
        //To set multiple elements of an array, use "SetParamArray()".
        inline void SetParam_Dynamic(const std::string& name, const UniformElement_t& value,
                                     std::optional<uint_fast32_t> arrayIndex = std::nullopt)
        {
            //We need to check all the different possible types of the value,
            //    and then dispatch to the templated SetParam<T>().
            //Use macros to cut down on the massive number of different types.

        #define SET_PARAM_BASIC(T) \
            else if (std::holds_alternative<T>(value)) \
                SetParam(name, std::get<T>(value), arrayIndex);

        #define SET_PARAM_VEC(T) \
            SET_PARAM_BASIC(T) \
            SET_PARAM_BASIC(glm::vec<1 BP_COMMA T>) \
            SET_PARAM_BASIC(glm::vec<2 BP_COMMA T>) \
            SET_PARAM_BASIC(glm::vec<3 BP_COMMA T>) \
            SET_PARAM_BASIC(glm::vec<4 BP_COMMA T>)

        #define SET_PARAM_MAT(C, R) \
            SET_PARAM_BASIC(glm::mat<C BP_COMMA R BP_COMMA float>) \
            SET_PARAM_BASIC(glm::mat<C BP_COMMA R BP_COMMA double>)


            if (false) {} //The macros are defined as "else if" statements
            SET_PARAM_VEC(float)
            SET_PARAM_VEC(double)
            SET_PARAM_VEC(int32_t)
            SET_PARAM_VEC(uint32_t)
            SET_PARAM_VEC(bool)
            SET_PARAM_MAT(2, 2) SET_PARAM_MAT(3, 2) SET_PARAM_MAT(4, 2)
            SET_PARAM_MAT(2, 3) SET_PARAM_MAT(3, 3) SET_PARAM_MAT(4, 3)
            SET_PARAM_MAT(2, 4) SET_PARAM_MAT(3, 4) SET_PARAM_MAT(4, 4)
            SET_PARAM_BASIC(const Textures::Texture*)
            SET_PARAM_BASIC(Uniforms::GradientValue_t)
            SET_PARAM_BASIC(OglPtr::View)
            SET_PARAM_BASIC(OglPtr::Buffer)
            else
            {
                BP_ASSERT_STR(false,
                              "Unexpected uniform value type, index " +
                                  std::to_string(value.index()));
            }

        #undef SET_PARAM_VEC
        #undef SET_PARAM_MAT
        #undef SET_PARAM_BASIC
        }

        //To set a single uniform (or element of a uniform array), call the following function:
        /*
            void SetParam(const std::string& name, const T& value,
                          std::optional<uint_fast32_t> arrayIndex = std::nullopt);
        */
        #pragma region SetParam()
        
        //A macro that handles the boilerplate function declaration:
    #define DECL_SET_PARAM(T, tName) \
        void SetParam(const std::string& name, T tName, \
                      std::optional<uint_fast32_t> arrayIndex = std::nullopt)

        //A macro that handles the most common use-case,
        //    passing through to "Impl_SetParam()":
    #define DECL_SET_PARAM_SIMPLE(T) \
        DECL_SET_PARAM(T, val) { \
            Impl_SetParam<T, T>(name, val, val, arrayIndex); \
        }

        //Scalar types:
        DECL_SET_PARAM_SIMPLE(float)
        DECL_SET_PARAM_SIMPLE(double)
        DECL_SET_PARAM_SIMPLE(int32_t)
        DECL_SET_PARAM_SIMPLE(uint32_t)
        DECL_SET_PARAM_SIMPLE(bool)

        //Vector types:
        template<glm::length_t L, typename T>
        DECL_SET_PARAM_SIMPLE(const glm::vec<L BP_COMMA T>&)

        //Matrix types:
        template<glm::length_t C, glm::length_t R, typename T>
        DECL_SET_PARAM_SIMPLE(const glm::mat<C BP_COMMA R BP_COMMA T>&)

        //Textures require special handling;
        //    we need to decide which sampler to use.
        DECL_SET_PARAM(const Textures::Texture&, tex) {

            //Get the desired sampler for this uniform, if one exists;
            //    otherwise use the texture's default sampler.
            //TODO: Cache this information so we don't have to do costly lookups every time the texture changes.

            const auto& allUniformDefs = GetFactory().GetUniformDefs().Uniforms;
            auto paramTypeData = allUniformDefs.find(name);
            BP_ASSERT_STR(paramTypeData != allUniformDefs.end(),
                          "Material parameter '" + name + "' not found in " +
                              "the shader definitions");

            //TODO: Once we support Images in the uniform definitions, we'll need to branch here on TexSampler vs Image.
            const auto& paramData = paramTypeData->second;
            BP_ASSERT_STR(std::holds_alternative<Uniforms::TexSampler>(paramData.ElementType),
                          "Material parameter '" + name + "' isn't a TexSampler type");

            //Pick a sampler and get the corresponding texture handle.
            const auto& tpData = std::get<Uniforms::TexSampler>(paramData.ElementType);
            const auto tpSampler = tpData.FullSampler.value_or(tex.GetSamplerFull());
            auto& texHandle = tex.GetViewHandleFull(tpSampler);

            Textures::TexView view(texHandle);
            UpdateViewUse(name, std::nullopt, view);
            Impl_SetParam(name, view, &tex, arrayIndex);
        }

        //Gradients also require special handling,
        //    because they are stored in textures under the hood:
        DECL_SET_PARAM(const Uniforms::GradientValue_t&, gradient) {
            //Update the Storage for this gradient.
            //This does not re-allocate any textures, so nothing else needs to be done.

            //If this is an element of an array,
            //    the Storage expects the name to include its index.
            auto elementName = name;
            if (arrayIndex.has_value())
                elementName += "[" + std::to_string(arrayIndex.value()) + "]";

            paramStorage.SetGradient(elementName, gradient);

            //We aren't actually updating the shader, but
            //    we need to technically pass in the value that would go to the shader.
            auto rawGradient = paramStorage.GetGradient(elementName).GetView();
            Impl_SetParam(name, rawGradient, gradient, arrayIndex, false);
        }


    #undef DECL_SET_PARAM_SIMPLE
    #undef DECL_SET_PARAM

        #pragma endregion


        //To set a block of elements in a uniform array, call the following function:
        /*
            void SetParamArray(const std::string& name, const span<T>& values,
                               size_t destOffset = 0);
        */
        //Does not work on arrays of structs!
        //For arrays of structs, you should set the elements and fields
        //    as if they're individual parameters, with full names like "u_lights[3].color".

        #pragma region SetParamArray()

        //A macro that handles the boilerplate function declaration:
    #define DECL_SET_PARAM_ARRAY(T, tName) \
        void SetParamArray(const std::string& name, const span<T>& tName, \
                           size_t destOffset = 0)

        //A macro that handles the most common use-case,
        //    passing through to "Impl_SetParamArray()":
    #define DECL_SET_PARAM_ARRAY_SIMPLE(T) \
        DECL_SET_PARAM_ARRAY(T, values) { \
            Impl_SetParamArray(name, values, values, destOffset); \
        }

        //Scalars:
        DECL_SET_PARAM_ARRAY_SIMPLE(float)
        DECL_SET_PARAM_ARRAY_SIMPLE(double)
        DECL_SET_PARAM_ARRAY_SIMPLE(int32_t)
        DECL_SET_PARAM_ARRAY_SIMPLE(uint32_t)
        DECL_SET_PARAM_ARRAY_SIMPLE(bool)

        //Vectors:
        template<glm::length_t L, typename T>
        DECL_SET_PARAM_ARRAY_SIMPLE(glm::vec<L BP_COMMA T>)
            
        //Matrices:
        template<glm::length_t C, glm::length_t R, typename T>
        DECL_SET_PARAM_ARRAY_SIMPLE(glm::mat<C BP_COMMA R BP_COMMA T>)

        //Textures need special handling;
        //    we need to decide what sampler to use.
        DECL_SET_PARAM_ARRAY(const Textures::Texture*, textures) {
            //Get the desired sampler for this uniform, if one exists;
            //    otherwise use the texture's default sampler.

            //TODO: Cache this information so we don't have to do costly lookups every time the texture changes.
            const auto& allUniformDefs = GetFactory().GetUniformDefs().Uniforms;
            auto paramTypeData = allUniformDefs.find(name);
            BP_ASSERT_STR(paramTypeData != allUniformDefs.end(),
                          "Material parameter '" + name + "' not found in " +
                              "the shader definitions");

            //TODO: Once we support Images in the uniform definitions, we'll need to branch here on TexSampler vs Image.
            const auto& paramData = paramTypeData->second;
            BP_ASSERT_STR(paramData.IsArray(),
                          "Material parameter '" + name + "' was supposed to be an array");
            BP_ASSERT_STR(std::holds_alternative<Uniforms::TexSampler>(paramData.ElementType),
                          "Material parameter '" + name + "' isn't a TexSampler type");

            //For each texture in the array,
            //    pick a sampler and get the corresponding view.
            const auto& tpData = std::get<Uniforms::TexSampler>(paramData.ElementType);
            buf_texHandles.resize(textures.size());
            buf_texViewPtrs.resize(textures.size());
            for (size_t i = 0; i < textures.size(); ++i)
            {
                const auto tpSampler = tpData.FullSampler.value_or(textures[i]->GetSamplerFull());
                buf_texHandles[i] = &(textures[i]->GetViewHandleFull(tpSampler));

                Textures::TexView view(*buf_texHandles[i]);
                buf_texViewPtrs[i] = view.GlPtr;

                UpdateViewUse(name, (uint_fast32_t)(destOffset + 1), view);
            }
            
            Impl_SetParamArray(name, span(buf_texViewPtrs), span(textures), destOffset);
        }

        //Gradients also need special handling;
        //    they are backed by a Texture1D, managed by the Storage instance.
        DECL_SET_PARAM_ARRAY(const Uniforms::GradientValue_t&, gradients) {
            //Update the Storage for these gradients.
            //Note that this only changes the values of existing textures,
            //    so we don't need to do much else.
            for (size_t i = 0; i < gradients.size(); ++i)
            {
                //TODO: Re-use a string buffer for generating these names.
                auto elName = name + "[" + std::to_string(i) + "]";
                paramStorage.SetGradient(elName, gradients[i]);
            }

            //We aren't actually updating the shader, but
            //    we need to technically pass in the values that would go to the shader.
            buf_gradientTextures.resize(gradients.size());
            auto dummySpan = span(buf_gradientTextures);

            Impl_SetParamArray(name, dummySpan, gradients, destOffset, false);
        }


    #undef DECL_SET_PARAM_ARRAY_SIMPLE
    #undef DECL_SET_PARAM_ARRAY

        #pragma endregion
        
        #pragma endregion

        #pragma region Uniform getters

        //Gets the uniform with the given name, without knowing before-hand
        //    what type it's supposed to be.
        //If getting an element of a uniform array, pass the index as well.
        //To get multiple elements of an array, use "GetParamArray()".
        UniformElement_t GetParam(const std::string& name,
                                  std::optional<uint_fast32_t> arrayIndex = std::nullopt) const
        {
            auto found = params.find(name);

            BP_ASSERT_STR(found != params.end(),
                          "No Material parameter exists named '" + name + "'");
            BP_ASSERT_STR(!std::holds_alternative<std::vector<UniformElement_t>>(found->second),
                          "The Material parameter '" + name + "' is an array, not a single value");

            return std::get<UniformElement_t>(found->second);
        }

        //Gets the parameter of the given name, and a known type.
        //If getting an element of a uniform array, pass the index as well
        //To get multiple elements of an array, use "GetParamArray()".
        template<typename T>
        const T& GetParam(const std::string& name,
                          std::optional<uint_fast32_t> arrayIndex = std::nullopt) const
        {
            UniformElement_t valueStorage = GetParam(name, arrayIndex);

            BP_ASSERT_STR(std::holds_alternative<T>(valueStorage),
                          "The Material parameter '" + name + "' is not the expected type");

            return std::get<T>(valueStorage);
        }

        //Gets a contiguous block of elements in the given uniform array,
        //    writing them into the given span.
        //The elements must be a simple type, not a struct!
        //For arrays of structs, you should get the elements and fields
        //    as if they're individual uniforms, with full names like "u_lights[3].color".
        template<typename T>
        void GetParamArray(const std::string& name, span<T>& outValues,
                           span<T>::size_type uOffset = 0)
        {
            auto found = params.find(name);

            //Make sure the arguments are valid.
            BP_ASSERT_STR(found != params.end(),
                          "No Material parameter exists named '" + name + "'");
            BP_ASSERT_STR(std::holds_alternative<std::vector<UniformElement_t>>(found->second),
                          "The Material parameter '" + name + "' is not an array, but a single value");

            //Read the values.
            const auto& vec = std::get<std::vector<UniformElement_t>>(found->second);
            std::copy(vec.begin, vec.end(), outValues.begin());
        }

        #pragma endregion


    private:

        CompiledShader* currentVariant;
        Factory* factory;

        //Whether this Material is currently the one that all drawing is done in.
        bool isActive;

        //The current value of all parameters,
        //    including ones that aren't actually used by the current shader variant.
        std::unordered_map<std::string, UniformValue_t> params;

        //Stores GPU resources for special uniforms (e.x. Gradients need to be uploaded to a texture).
        Uniforms::Storage paramStorage;

        //Buffers used when formatting parameters to send to a shader.
        std::vector<Textures::Texture*> buf_gradientTextures;
        std::vector<Textures::TexHandle*> buf_texHandles;
        std::vector<OglPtr::View> buf_texViewPtrs;
        
        //Counts, for each texture/image view, how many different uniforms are referencing it.
        std::unordered_map<std::variant<Textures::ImgView, Textures::TexView>,
                           size_t>
            viewUses;


        //Given a new value for a texture uniform,
        //    upates all the book-keeping related to active texture views.
        void UpdateViewUse(const std::string& paramName,
                           std::optional<uint_fast32_t> paramArrayIndex,
                           const Textures::TexView& newView)
        {
            //If this Material isn't active right now,
            //    there's no need to update our book-keeping.
            if (!isActive)
            {
                BP_ASSERT(viewUses.empty(),
                          "Bplus::GL::Materials::Material.viewUses has elements"
                            " despite the Material not being active");
                return;
            }

            //If the new value matches the old one, nothing changes.
            auto oldView = GetParam<Textures::TexView>(paramName, paramArrayIndex);
            if (oldView == newView)
                return;

            viewUses[newView] += 1;

            //Decrement the counter for the old view,
            //    and destroy its entry if there are no uses of it left.
            auto oldViewRef = viewUses.find(oldView);
            oldViewRef->second -= 1;
            if (oldViewRef->second < 1)
                viewUses.erase(oldViewRef);
        }

        //Switches to use the given shader variant.
        void ChangeVariant(CompiledShader& newVariant);


        //Implementations of the "set param" functions.
        //They take extra parameters which aren't part of the public interface.

        template<typename T, typename TInternal>
        void Impl_SetParam(const std::string& name,
                           const T& shaderValue,
                           const TInternal& materialValue,
                           std::optional<uint_fast32_t> arrayIndex,
                           bool updateShader = true)
        {
            const auto& paramRef = params.find(name);
            BP_ASSERT_STR(paramRef != params.end(),
                          "No parameter named '" + name + "' exists in the Material");

            if (!arrayIndex.has_value())
            {
                //Make sure the parameter is not an array.
                BP_ASSERT_STR(!std::holds_alternative<std::vector<UniformElement_t>>(paramRef->second),
                              "Trying to set an array uniform as if it's just one element: '" + name + "'");

                //Update the uniform in OpenGL/the current variant.
                if (isActive && updateShader)
                    currentVariant->SetUniform(name, shaderValue);

                //Remember the new value, for ALL variants.
                params[name] = materialValue;
            }
            else
            {
                auto valueI = arrayIndex.value();

                //Make sure the parameter is actually an array.
                BP_ASSERT_STR(std::holds_alternative<std::vector<UniformElement_t>>(paramRef->second),
                              "Trying to set a non-array uniform as if it's an array: '" + name + "'");
                auto& vec = std::get<std::vector<UniformElement_t>>(paramRef->second);

                //Update the uniform in OpenGL/the current variant.
                if (isActive && updateShader)
                    currentVariant->SetUniformArrayElement(name, valueI, shaderValue);

                //Remember the new value, for ALL variants.
                vec[valueI] = materialValue;
            }
        }

        template<typename T, typename TInternal>
        void Impl_SetParamArray(const std::string& name,
                                const span<T>& shaderValues,
                                const span<TInternal>& materialValues,
                                span<T>::size_type uOffset,
                                bool updateShader = true)
        {
            auto paramRef = params.find(name);
            BP_ASSERT_STR(paramRef != params.end(),
                          "No parameter named '" + name + "' exists in the Material");

            //Make sure the parameter is actually an array.
            BP_ASSERT_STR(std::holds_alternative<std::vector<UniformElement_t>>(paramRef->second),
                          "Trying to set a non-array uniform as if it's an array: '" + name + "'");
            auto& vec = std::get<std::vector<UniformElement_t>>(paramRef->second);

            //Update the uniform in OpenGL/the current variant.
            if (isActive && updateShader)
                currentVariant->SetUniformArray(name, shaderValues.value(), uOffset);

            //Remember the new value, for ALL variants.
            std::copy(materialValues.begin(), materialValues.end(),
                      vec.begin() + (size_t)uOffset);
        }
    };
}