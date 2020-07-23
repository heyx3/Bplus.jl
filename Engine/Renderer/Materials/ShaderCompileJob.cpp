#include "ShaderCompileJob.h"

using namespace Bplus;
using namespace Bplus::GL;

namespace fs { using namespace std::filesystem; }


fs::path ShaderIncluderFromFiles::ToFullPath(const fs::path& path) const
{
    //Make sure the path is unambiguous by using fs::canonical().
    //Otherwise we may have problems using it as a key in the cache.
    return fs::canonical(relativePath / path);
}

void ShaderIncluderFromFiles::SetCacheEntry(const fs::path& key, const std::string& value)
{
    fileCache[ToFullPath(key)] = value;
}

std::optional<std::string> ShaderIncluderFromFiles::GetFile(const fs::path& relativePath)
{
    std::string contents;
    if (GetFile(relativePath, contents))
        return contents;
    else
        return std::nullopt;
}
bool ShaderIncluderFromFiles::GetFile(const std::filesystem::path& relativePath,
                                      std::string& output)
{
    auto fullPath = ToFullPath(relativePath);
    
    //If it exists in the cache already, retrieve it.
    auto cachedValue = fileCache.find(fullPath);
    if (cachedValue != fileCache.end())
    {
        output += cachedValue->second;
        return true;
    }

    //Otherwise, try to load it and store in the cache.
    std::string fileContents;
    if (IO::LoadEntireFile(fullPath, fileContents))
    {
        fileCache[fullPath] = fileContents;
        output += fileContents;
        return true;
    }
    else
    {
        return false;
    }
}



bool ShaderCompileJob::GetCompiledBinary(OglPtr::ShaderProgram program,
                                         std::vector<std::byte>& outBytes)
{
    GLint byteSize;
    glGetProgramiv(program.Get(), GL_PROGRAM_BINARY_LENGTH, &byteSize);
    if (byteSize == 0)
        return false;

    size_t offset = outBytes.size();
    outBytes.resize(outBytes.size() + byteSize);
    GLenum format;
    glGetProgramBinary(program.Get(), byteSize, nullptr, &format, &outBytes[offset]);

    return true;
}

void ShaderCompileJob::PreProcessIncludes(std::string& sourceStr) const
{
    //TODO: Implement.
}

OglPtr::ShaderProgram ShaderCompileJob::Compile() const
{
    //TODO: Implement.
}