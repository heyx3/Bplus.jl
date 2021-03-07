#include "Material.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


void Material::ChangeVariant(CompiledShader& newVariant)
{
    if (isActive)
    {
        newVariant.Activate();

        //TODO: Update uniforms for the new variant.
    }

    currentVariant = &newVariant;
}