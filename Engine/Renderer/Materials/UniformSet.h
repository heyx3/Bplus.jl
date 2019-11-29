#pragma once

#include "../Context.h"
#include "Uniforms.h"

#include <unordered_map>
#include <unordered_set>


namespace Bplus::GL
{
    //Manages a set of shader uniforms, including tracking which ones are "dirty",
    //    a.k.a. ones that have been changed recently.
    class BP_API UniformSet
    {
    public:
        
        template<typename Uniform_t>
        using UMap = std::unordered_map<std::string, Uniform_t>;

        using NameSet = std::unordered_set<std::string>;

        size_t GetVectorUniformsCount() const;
        size_t GetMatrixUniformsCount() const;
        size_t GetTextureUniformsCount() const;
        auto GetTotalUniformsCount() const { return GetVectorUniformsCount() + GetMatrixUniformsCount() + GetTextureUniformsCount(); }
        
        size_t GetDirtyVectorUniformsCount() const;
        size_t GetDirtyMatrixUniformsCount() const;
        size_t GetDirtyTextureUniformsCount() const;
        auto GetTotalDirtyUniformsCount() const { return GetDirtyVectorUniformsCount() + GetDirtyMatrixUniformsCount() + GetDirtyTextureUniformsCount(); }

        
        #pragma region Getters and setters for uniform data

        //Note that, to reduce heap usage,
        //    the various collections inside this class aren't allocated until
        //    they are first used.
        //All of the below functions count as a "usage".
        //If you're just checking the size of a collection, use the above helper functions.

        const UMap<VectorUniform>& GetVectors() const { return vectorUniforms.Get(); }
        const NameSet& GetDirtyVectors() const { return dirtyVectors.Get(); }

        const UMap<MatrixUniform>& GetMatrices() const { return matrixUniforms.Get(); }
        const NameSet& GetDirtyMatrices() const { return dirtyMatrices.Get(); }

        const UMap<TextureUniform>& GetTextures() const { return textureUniforms.Get(); }
        const NameSet& GetDirtyTextures() const { return dirtyTextures.Get(); }

        void Set(const std::string& uniformName, const VectorUniform& uniformValue);
        void Set(const std::string& uniformName, const MatrixUniform& uniformValue);
        void Set(const std::string& uniformName, const TextureUniform& uniformValue);

        #pragma endregion


        //Resets all uniforms' "dirty" flag.
        void Clean();


    private:

        Lazy<UMap<VectorUniform>> vectorUniforms;
        Lazy<UMap<MatrixUniform>> matrixUniforms;
        Lazy<UMap<TextureUniform>> textureUniforms;

        Lazy<NameSet> dirtyVectors, dirtyMatrices, dirtyTextures;
    };
}