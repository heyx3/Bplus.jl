#pragma once

#include "Factory.h"

namespace Bplus::GL::Materials
{
    //TODO: Add uniform tracking at the Material level that calls into CompiledShader uniform support.
    //TODO: Move all buffer/texture-handling up to Material, so that CompiledShader just takes a bare OglPtr.
    //TODO: Make CompiledShader more of a helper class that belongs to Material, no static tracking of stuff. Unit test apps will have to be updated.

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