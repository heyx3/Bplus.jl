#include "Storage.h"
 
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
        //Given that we want the first pixel to map to t=0,
        //    and the last pixel to map to t=1,
        //    I'm pretty sure that:
        //  * We use (width - 1) to calculate the texel size
        //  * We do NOT add a half-pixel offset to each pixel's T value.

        uint32_t width = tex.GetSize().x;
        buffer.resize(width);

        float texel = 1.0f / (width - 1);
        for (uint32_t x = 0; x < width; ++x)
            buffer[x] = gradient.Get(x * texel);

        tex.Set_Color(buffer.data());
    }
}


Storage::Storage(const Definitions& defs)
{
    defs.VisitAllUniforms([&](const std::string& uName, const Type& uType)
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

void Storage::SetGradient(const std::string& name,
                          const Uniforms::GradientValue_t& newValue)
{
    BP_ASSERT_STR(gradients.find(name) != gradients.end(),
                  "Can't find storage for gradient uniform '" + name + "'");
    
    FillGradient(gradients[name], bufferRGBA, newValue);
}
const Textures::Texture1D& Storage::GetGradient(const std::string& name) const
{
    BP_ASSERT_STR(gradients.find(name) != gradients.end(),
                  "Can't find storage for gradient uniform '" + name + "'");

    return gradients.at(name);
}