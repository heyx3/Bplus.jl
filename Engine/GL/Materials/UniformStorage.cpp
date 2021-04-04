#include "UniformStorage.h"
 
using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Uniforms;
using namespace Bplus::GL::Materials;


namespace
{
    void FillGradient(Textures::Texture1D& tex,
                      std::vector<glm::fvec4>& buffer,
                      const GradientValue_t& gradient)
    {
        uint32_t width = tex.GetSize().x;
        float texel = 1.0f / width;
        buffer.resize(width);

        for (uint32_t x = 0; x < width; ++x)
        {
            float u = (x + 0.5f) * texel;
            buffer[x] = gradient.Get(u);
        }

        tex.Set_Color(buffer.data());
    }
}


UniformStorage::UniformStorage(const UniformDefinitions& defs)
{
    defs.VisitAllUniforms([&](const std::string& uName, const UniformType& uType)
    {
        if (std::holds_alternative<Uniforms::Gradient>(uType.ElementType))
        {
            BP_ASSERT_STR(gradients.find(uName) == gradients.end(),
                          "More than one definition of gradient uniform '" + uName + "'");

            const auto& gData = std::get<Uniforms::Gradient>(uType.ElementType);

            Textures::Texture1D value(glm::uvec1{ gData.Resolution },
                                      gData.Format, 1,
                                      Textures::Sampler<1>());
            FillGradient(value, bufferRGBA, gData.Default);

            gradients[uName] = std::move(value);
        }
    });
}

void UniformStorage::SetGradient(const std::string& name,
                                 const Uniforms::GradientValue_t& newValue)
{
    BP_ASSERT_STR(gradients.find(name) != gradients.end(),
                  "Can't find storage for gradient uniform '" + name + "'");
    
    FillGradient(gradients[name], bufferRGBA, newValue);
}
const Textures::Texture1D& UniformStorage::GetGradient(const std::string& name) const
{
    BP_ASSERT_STR(gradients.find(name) != gradients.end(),
                  "Can't find storage for gradient uniform '" + name + "'");

    return gradients.at(name);
}