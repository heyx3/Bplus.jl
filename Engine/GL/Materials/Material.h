#pragma once

#include "Factory.h"

namespace Bplus::GL::Materials
{
    //A shader program, combined with parmeter values.
    //More precisely, a set of swappable shader programs that have
    //    different compile-time flags.
    class Material
    {
    public:

        //Gets the factory that compiles this material's shader variants.
        Factory& GetOrigin() const { return *origin; }


        Material(Factory& origin) : origin(&origin), currentVariant(nullptr), isActive(false) { }
        virtual ~Material() { }


    protected:

        void ChangeVariant(CompiledShader& newVariant);

    private:

        CompiledShader* currentVariant;
        Factory* origin;

        //Whether this Material is currently the one that all drawing is done in.
        bool isActive;
    };
}