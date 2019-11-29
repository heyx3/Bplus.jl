#pragma once

#include "MaterialPermutation.h"

namespace Bplus::GL
{
    //Holds multiple compiled permutations of a compiled shader,
    //    plus render settings.
    class Material
    {
    public:
        
        //The render state this Material should be used with.
        //Note that you can modify these settings as desired.
        RenderState RenderSettings;

        //Gets the original render settings defined for this Material.
        const RenderState GetDefaultRenderSettings() const { return defaultRenderSettings; }


        const MaterialPermutation& GetPermutation(MaterialPermutation::ID id) const { return permutations.find(id)->second; }


    private:

        RenderState defaultRenderSettings;

        std::unordered_map<MaterialPermutation::ID, MaterialPermutation> permutations;
    };
}