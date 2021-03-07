#pragma once

#include "CompiledShader.h"

namespace Bplus::GL::Materials
{
    //Abstract base class for a group of shaders that can be seamlessly swapped out
    //    based on shader-compile-time settings.
    //Each individual compiled shader is called a "variant".
    //Child classes should not inherit from this, but from the below "_Factory" class.
    class BP_API Factory
    {
    public:
        virtual ~Factory() { }
    };


    //A shader Factory that generates variants
    //    based on the contents of a particular data structure.
    //The shader variants are cached using the data structure as the key,
    //    so it must implement hashing and equality.
    //Instances of this class are not thread-safe.
    template<typename ShaderVariantData_t>
    class BP_API _Factory : public Factory
    {
        #pragma region Check validity of ShaderVariantData_t

        static_assert(Bplus::is_std_hashable_v<ShaderVariantData_t>,
                      "Shader variant data must be hashable");
        //Unfortunately, we can't easily assert that the variant data type implements equality.

        #pragma endregion

    public:

        CompiledShader& GetVariant(const ShaderVariantData_t& data)
        {
            auto found = knownShaders.find(data);
            if (found == knownShaders.end())
                found = knownShaders.emplace(data, MakeVariant(data)).first;
            return found->second;
        }

    protected:

        virtual CompiledShader* MakeVariant(const ShaderVariantData_t& data) = 0;

    private:

        using CachedShaderMap = std::unordered_map<ShaderVariantData_t, CompiledShader>;
        CachedShaderMap knownShaders;
    };
}