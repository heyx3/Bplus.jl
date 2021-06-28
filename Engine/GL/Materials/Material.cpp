#include "Material.h"

#include "../Textures/TextureCube.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


namespace Defaults
{
    //Default texture data for new Materials.
    thread_local std::optional<Textures::Texture1D> t1D;
    thread_local std::optional<Textures::Texture2D> t2D;
    thread_local std::optional<Textures::Texture3D> t3D;
    thread_local std::optional<Textures::TextureCube> tCube;

    Textures::Texture* GetDefaultTexParam(Textures::Types type)
    {
        Textures::Format format = Textures::SimpleFormat(
            Textures::FormatTypes::NormalizedUInt,
            Textures::SimpleFormatComponents::RGBA,
            Textures::SimpleFormatBitDepths::B8
        );

        switch (type)
        {
            case Textures::Types::OneD: {
                auto& t = t1D;
                if (!t.has_value())
                    t.emplace(glm::uvec1{ 1 }, format);
                return &t.value();
            } break;

            case Textures::Types::TwoD: {
                auto& t = t2D;
                if (!t.has_value())
                    t.emplace(glm::uvec2{ 1, 1 }, format);
                return &t.value();
            } break;

            case Textures::Types::ThreeD: {
                auto& t = t3D;
                if (!t.has_value())
                    t.emplace(glm::uvec3{ 1, 1, 1 }, format);
                return &t.value();
            } break;

            case Textures::Types::Cubemap: {
                auto& t = tCube;
                if (!t.has_value())
                    t.emplace(4, format);
                return &t.value();
            } break;

            default:
                BP_ASSERT_STR(false, "Unexpected Bplus::Textures::Types::" + type._to_string());
                return nullptr;
        }
    }
}


Material::Material(Factory& factory)
    : factory(&factory),
      paramStorage(factory.GetUniformDefs()),
      currentVariant(nullptr), isActive(false)
{
    //Learn the type of each uniform parameter,
    //    and remember it by writing an initial value into the "params" field.

    //Helper that converts a uniform type declaration into its default value.
    //If the type is an array, this returns the default value for an element.
    //Assumes the type is not a struct.
    auto getTypeDefault = [&](const Uniforms::Type& uType) -> UniformElement_t
    {
        if (std::holds_alternative<Uniforms::Vector>(uType.ElementType))
        {
            const auto& vData = std::get<Uniforms::Vector>(uType.ElementType);
            switch (vData.Type)
            {
            #define V_DATA_TYPE(EnumT, T, StoreT) \
                case Uniforms::ScalarTypes::EnumT: \
                    switch (vData.Type) { \
                        case Uniforms::VectorSizes::One:   return              T(std::get<StoreT>(vData.DefaultValue).x); \
                        case Uniforms::VectorSizes::Two:   return glm::vec<2, T>(Math::Resize<2>(std::get<StoreT>(vData.DefaultValue))); \
                        case Uniforms::VectorSizes::Three: return glm::vec<3, T>(Math::Resize<3>(std::get<StoreT>(vData.DefaultValue))); \
                        case Uniforms::VectorSizes::Four:  return glm::vec<4, T>(std::get<StoreT>(vData.DefaultValue)); \
                        default: \
                            BP_ASSERT_STR(false, "Unexpected Bplus::GL::Uniforms::VectorSizes::" + vData.D._to_string()); \
                            return false; \
                    } break

                V_DATA_TYPE(Float, float, glm::dvec4);
                V_DATA_TYPE(Double, double, glm::dvec4);
                V_DATA_TYPE(Int, int32_t, glm::i64vec4);
                V_DATA_TYPE(UInt, uint32_t, glm::i64vec4);
                V_DATA_TYPE(Bool, bool, glm::i64vec4);
            #undef V_DATA_TYPE
                
                default:
                    BP_ASSERT_STR(false,
                                    "Unexpected Bplus::GL::Uniforms::ScalarTypes::" +
                                        vData.Type._to_string());
                    return false;
            }
        }
        else if (std::holds_alternative<Uniforms::Matrix>(uType.ElementType))
        {
            const auto& mData = std::get<Uniforms::Matrix>(uType.ElementType);

            switch (mData.Rows)
            {
                //Use macros to simplify the below syntax.

            #define M_DATA_TYPE(EnumT, T) \
                case Uniforms::MatrixSizes::EnumT: return T;

            #define M_CASE_DEFAULT(val) \
                default: \
                    BP_ASSERT_STR(false, \
                                    "Unexpected Bplus::GL::Uniforms::MatrixSizes::" +  \
                                        (val)._to_string()); \
                    return false;

                case Uniforms::MatrixSizes::Two:
                    switch (mData.Columns) {
                        M_DATA_TYPE(Two, glm::fmat2x2(1));
                        M_DATA_TYPE(Three, glm::fmat3x2(1));
                        M_DATA_TYPE(Four, glm::fmat4x2(1));
                        M_CASE_DEFAULT(mData.Columns);
                    } break;

                case Uniforms::MatrixSizes::Three:
                    switch (mData.Columns) {
                        M_DATA_TYPE(Two, glm::fmat2x3(1));
                        M_DATA_TYPE(Three, glm::fmat3x3(1));
                        M_DATA_TYPE(Four, glm::fmat4x3(1));
                        M_CASE_DEFAULT(mData.Columns);
                    } break;

                case Uniforms::MatrixSizes::Four:
                    switch (mData.Columns) {
                        M_DATA_TYPE(Two, glm::fmat2x4(1));
                        M_DATA_TYPE(Three, glm::fmat3x4(1));
                        M_DATA_TYPE(Four, glm::fmat4x4(1));
                        M_CASE_DEFAULT(mData.Columns);
                    } break;

            #undef M_DATA_TYPE

                M_CASE_DEFAULT(mData.Rows);
            #undef M_CASE_DEFAULT
            }
        }
        else if (std::holds_alternative<Uniforms::Color>(uType.ElementType))
        {
            const auto& cData = std::get<Uniforms::Color>(uType.ElementType);
            switch (cData.Channels)
            {
                case Textures::SimpleFormatComponents::R:    return cData.Default.r;
                case Textures::SimpleFormatComponents::RG:   return Math::Resize<2>(cData.Default);
                case Textures::SimpleFormatComponents::RGB:  return Math::Resize<3>(cData.Default);
                case Textures::SimpleFormatComponents::RGBA: return cData.Default;
                default:
                    BP_ASSERT_STR(false,
                                    "Unexpected Bplus::GL::Textures::SimpleFormatComponents::" +
                                        cData.Channels._to_string());
                    return false;
            }
        }
        else if (std::holds_alternative<Uniforms::Gradient>(uType.ElementType))
        {
            const auto& gData = std::get<Uniforms::Gradient>(uType.ElementType);
            return gData.Default;
        }
        else if (std::holds_alternative<Uniforms::TexSampler>(uType.ElementType))
        {
            //TODO: See the note in the similar place in CompiledShader. Once we have that, remove this tiny default texture.
            const auto& tData = std::get<Uniforms::TexSampler>(uType.ElementType);
            return Defaults::GetDefaultTexParam(tData.Type)->GetViewFull();
        }
        else
        {
            BP_ASSERT_STR(false,
                            "Unexpected type for a uniform's 'ElementType'. Index: " +
                                std::to_string(uType.ElementType.index()));
            return false;
        }
    };
    
    //Helper that processes a specific uniform into the "params" field.
    //Assumes that structs are broken up into their individual fields.
    auto visitUniform = [&](const std::string& uName, const Uniforms::Type& uType)
    {
        BP_ASSERT_STR(params.find(uName) == params.end(),
                      "The Material parameter name '" + uName +
                          "' appears more than once in the shader definitions");
        if (uType.IsArray())
        {
            std::vector<UniformElement_t> buffer;
            buffer.resize(uType.ArrayCount);

            auto elementDefault = getTypeDefault(uType);
            for (auto& val : buffer)
                val = elementDefault;

            params[uName] = std::move(buffer);
        }
        else
        {
            params[uName] = getTypeDefault(uType);
        }
    };

    factory.GetUniformDefs().VisitAllUniforms(false, visitUniform);
}

void Material::ChangeVariant(CompiledShader& newVariant)
{
    currentVariant = &newVariant;

    if (isActive)
    {
        for (const auto& [uName, uValue] : params)
        {
            //Is this an array of data?
            if (std::holds_alternative<std::vector<UniformElement_t>>(uValue))
            {
                //TODO: Copy into a vector of CompiledShader::UniformSetValue_t
            }
            //Othrwise it's just one element.
            else
            {
                //Replace the loop variable with a local variable,
                //    so it can be used in lambdas.
                const decltype(uName)& uName = uName;
                //Most data types just pass right through to "SetUniform()".
                //However, some require conversion from a high-level type to something simpler
                //    (like Gradient to a Texture1D).
                std::visit(overload(
                    //Textures: get the handle.
                    //[&](const )
                    //All other types: just pass it through.
                    [&](auto&& t) { newVariant.SetUniform(uName, t); }
                ), uValue);
            }
        }
    }
}