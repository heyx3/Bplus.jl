#pragma once

#include "CompiledShader.h"

namespace Bplus::GL
{
    //Holds multiple compiled permutations of a shader,
    //    plus render settings.
    //This is an abstract class; child classes define how to
    //    compile specific permutations.
    class Material
    {
    public:
        
        //The render state this Material should be used with.
        //Note that you can modify these settings as desired.
        RenderState RenderSettings;


        Material() { }
        Material(const RenderState& settings) : RenderSettings(settings),
                                                defaultRenderSettings(settings) { }

        //Copying Materials is not allowed.
        Material(const Material& cpy) = delete;
        Material& operator=(const Material& copy) = delete;

        //Moving Materials is fine.
        Material(Material&& src) = default;
        Material& operator=(Material&& src) = default;

        virtual ~Material() { }


        //Gets the original render settings defined for this Material.
        const RenderState GetDefaultRenderSettings() const { return defaultRenderSettings; }

        //Gets (or creates/compiles) the given permutation of this material.
        const CompiledShader& GetPermutation(CompiledShader::ID id);


    protected:

        //Create and compile the given permutation of this material.
        virtual CompiledShader CompilePermutation(CompiledShader::ID id) = NULL;


    private:

        RenderState defaultRenderSettings;

        std::unordered_map<CompiledShader::ID, CompiledShader> permutations;
    };
}