#include "UniformSet.h"


size_t Bplus::GL::UniformSet::GetVectorUniformsCount() const
{
    return vectorUniforms.IsCreated() ?
               vectorUniforms.Cast().size() :
               0;
}
size_t Bplus::GL::UniformSet::GetMatrixUniformsCount() const
{
    return matrixUniforms.IsCreated() ?
               matrixUniforms.Cast().size() :
               0;
}
size_t Bplus::GL::UniformSet::GetTextureUniformsCount() const
{
    return textureUniforms.IsCreated() ?
               textureUniforms.Cast().size() :
               0;
}

size_t Bplus::GL::UniformSet::GetDirtyVectorUniformsCount() const
{
    return dirtyVectors.IsCreated() ?
               dirtyVectors.Cast().size() :
               0;
}
size_t Bplus::GL::UniformSet::GetDirtyMatrixUniformsCount() const
{
    return dirtyMatrices.IsCreated() ?
               dirtyMatrices.Cast().size() :
               0;
}
size_t Bplus::GL::UniformSet::GetDirtyTextureUniformsCount() const
{
    return dirtyTextures.IsCreated() ?
               dirtyTextures.Cast().size() :
               0;
}

void Bplus::GL::UniformSet::Set(const std::string& uniformName, const VectorUniform& uniformValue)
{
    dirtyVectors.Get().insert(uniformName);
    vectorUniforms.Get()[uniformName] = uniformValue;
}
void Bplus::GL::UniformSet::Set(const std::string& uniformName, const MatrixUniform& uniformValue)
{
    dirtyMatrices.Get().insert(uniformName);
    matrixUniforms.Get()[uniformName] = uniformValue;
}
void Bplus::GL::UniformSet::Set(const std::string& uniformName, const TextureUniform& uniformValue)
{
    dirtyTextures.Get().insert(uniformName);
    textureUniforms.Get()[uniformName] = uniformValue;
}

void Bplus::GL::UniformSet::Clean()
{
    if (dirtyVectors.IsCreated())
        dirtyVectors.Cast().clear();

    if (dirtyMatrices.IsCreated())
        dirtyMatrices.Cast().clear();

    if (dirtyTextures.IsCreated())
        dirtyTextures.Cast().clear();
}