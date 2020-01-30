#pragma once

#include "../Context.h"
#include "Uniforms.h"

#include <unordered_map>
#include <unordered_set>


namespace Bplus::GL
{
    #pragma region Old Uniform data representation
    #if 0

    //Manages a set of shader uniforms for a specific compiled shader,
    //    including tracking which ones are "dirty" (a.k.a. ones that have been changed recently).
    class BP_API UniformSet
    {
    public:
        
        template<typename Uniform_t>
        using UMap = std::unordered_map<std::string, Uniform_t>;

        using UPtrMap = std::unordered_map<std::string, Ptr::ShaderUniform>;
        using UNameSet = std::unordered_set<std::string>;


        //The below methods get the total number of uniforms.
        size_t GetVectorUniformsCount() const;
        size_t GetMatrixUniformsCount() const;
        size_t GetTextureUniformsCount() const;
        auto GetTotalUniformsCount() const { return GetVectorUniformsCount() + GetMatrixUniformsCount() + GetTextureUniformsCount(); }
        
        //The below methods get the total number of "dirty" uniforms,
        //    i.e. ones that have been changed since the last "Clean()".
        size_t GetDirtyVectorUniformsCount() const;
        size_t GetDirtyMatrixUniformsCount() const;
        size_t GetDirtyTextureUniformsCount() const;
        auto GetTotalDirtyUniformsCount() const { return GetDirtyVectorUniformsCount() + GetDirtyMatrixUniformsCount() + GetDirtyTextureUniformsCount(); }

        //The below methods handle the mapping of uniform name to shader pointer.
        const UPtrMap& GetUniformPtrs() const { return uniformPtrs; }
        void SetUniformPtr(const std::string& name, Ptr::ShaderUniform value) { uniformPtrs[name] = value; }

        #pragma region Getters and setters for uniform data

        //Note that, to reduce heap usage,
        //    the various collections inside this class aren't allocated until
        //    they are first used.
        //All of the below functions inside this #region count as "using" them.
        //If you're just checking the size of a collection, use the above helper functions.

        const UMap<VectorUniform>& GetVectors() const { return vectorUniforms.Get(); }
        const UNameSet& GetDirtyVectors() const { return dirtyVectors.Get(); }

        const UMap<MatrixUniform>& GetMatrices() const { return matrixUniforms.Get(); }
        const UNameSet& GetDirtyMatrices() const { return dirtyMatrices.Get(); }

        const UMap<TextureUniform>& GetTextures() const { return textureUniforms.Get(); }
        const UNameSet& GetDirtyTextures() const { return dirtyTextures.Get(); }

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

        Lazy<UNameSet> dirtyVectors, dirtyMatrices, dirtyTextures;
        UPtrMap uniformPtrs;
    };

    #endif
    #pragma endregion
}