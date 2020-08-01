#pragma once

#include <optional>
#include <filesystem>

#include "../Context.h"

//TODO: Put shader stuff into its own namespace.


namespace Bplus::GL
{
    //A function that can read a file and write its contents to the given buffer,
    //    returning whether the file was successfully loaded.
    //Used to implement "include" statements in shader code.
    using FileContentsLoader = bool(const fs::path& file,
                                    std::stringstream& output);


    //An implementation of "include" statements for shaders, for the most common use-case:
    //    loading the files from disk with a relative path,
    //    and caching the ones that have already been loaded.
    class BP_API ShaderIncluderFromFiles
    {
    public:

        using Cache_t = std::unordered_map<std::string, std::string>;


        ShaderIncluderFromFiles(fs::path _relativePath) : relativePath(_relativePath) { }


        fs::path GetRelativePath() const { return relativePath; }        
        void SetRelativePath(const fs::path& newPath) { relativePath = newPath; }

        const Cache_t& GetCache() const { return fileCache; }
        void ClearCache() { fileCache.clear(); }
        void SetCacheEntry(const fs::path& relativePath, const std::string& contents);

        
        std::string ToFullPath(const fs::path& relativePath) const;

        //Reads the file from the given path and returns it.
        //Returns nothing if the file couldn't be loaded.
        std::optional<std::string> GetFile(const fs::path& relativePath);

        //Reads the file from the given path and appends its contents to the given buffer.
        //Returns whether the file was successfully loaded.
        bool GetFile(const fs::path& relativePath, std::stringstream& output);


    private:

        fs::path relativePath;
        Cache_t fileCache;
    };


    //A binary blob representing a previously-compiled shader.
    //You can usually cache these to avoid recompiling shaders.
    struct BP_API PreCompiledShader
    {
        GLenum Header;
        std::vector<std::byte> Data;

        PreCompiledShader(OglPtr::ShaderProgram compiledShader);
    };


    //A shader that can be loaded and compiled,
    //    with a bit of pre-processing to support "#pragma include" statements.
    //The file path in an include statement can use forward- or back-slashes,
    //    and the path string can be surrounded by double-quotes or angle brackets.
    class BP_API ShaderCompileJob
    {
    public:
        
        //A cap on the number of '#pragma include' statements that one file can make,
        //    to prevent infinite loops.
        //Starts at 100; feel free to increase this value if necessary.
        static size_t MaxIncludesPerFile;


        //Shader source codes (any empty ones are considered nonexistent):
        std::string VertexSrc, FragmentSrc,
                    GeometrySrc;
        //TODO: ComputeSource?
        
        //When a "#pragma include" statement appears in the shader code,
        //    this function loads the file's contents.
        std::function<FileContentsLoader> IncludeImplementation;

        //A pre-compiled version of this shader which this instance can attempt to use first.
        //The shader source code is still needed as a fallback.
        std::optional<PreCompiledShader> CachedBinary;


        //Pre-processes the given shader source to execute any "#pragma include(...)" statements.
        //Edits the string in-place.
        void PreProcessIncludes(std::string& sourceStr) const;

        //Compiles this program into an OpenGL object and returns it.
        //If the CachedBinary field exists but wasn't loaded successfully,
        //    it will be modified to contain the new up-to-date compiled binaries.
        //Additionally, the shader source strings in this instance
        //    will be modifed by the pre-processor,
        //    so that you can see what was actually compiled.
        //Returns an error message (or an empty string on success),
        //    plus a boolean indicating whether the CachedBinary field had to be modified.
        std::tuple<std::string, bool> Compile(OglPtr::ShaderProgram& outPtr);
    };
}