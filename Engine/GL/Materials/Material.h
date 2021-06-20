#pragma once

#include "Factory.h"

namespace Bplus::GL::Materials
{
    //A specific instance of a shader, with custom parameter values.
    //The shader is loaded from a Factory.
    class Material
    {
    public:

        //Gets the factory that manages this Material's shader variants.
        Factory& GetFactory() const { return *factory; }


        Material(Factory& factory) : factory(&factory), currentVariant(nullptr), isActive(false) { }
        virtual ~Material() { }


    protected:

        void ChangeVariant(CompiledShader& newVariant);

    private:

        CompiledShader* currentVariant;
        Factory* factory;

        //Whether this Material is currently the one that all drawing is done in.
        bool isActive;
    };
}