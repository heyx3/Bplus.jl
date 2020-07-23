#pragma once

#include <optional>
#include <filesystem>

#include "../Context.h"


namespace Bplus::GL
{
    //A function that can read a file and append its contents to the given string,
    //    returning whether the file was successfully loaded.
    //Used to implement "include" statements in shader code.
    using FileContentsAppender = bool(const std::filesystem::path& file,
                                      std::string& output);

    //An implementation of "include" statements for shaders, for the most common use-case:
    //    loading the files from disk with a relative path,
    //    and caching the ones that have already been loaded.
    class BP_API ShaderIncluderFromFiles
    {
    public:

        using Cache_t = std::unordered_map<std::filesystem::path, std::string>;


        ShaderIncluderFromFiles(std::filesystem::path _relativePath) : relativePath(_relativePath) { }


        std::filesystem::path GetRelativePath() const { return relativePath; }        
        void SetRelativePath(const std::filesystem::path& newPath) { relativePath = newPath; }

        const Cache_t& GetCache() const { return fileCache; }
        void ClearCache() { fileCache.clear(); }
        void SetCacheEntry(const std::filesystem::path& relativePath, const std::string& contents);

        
        std::filesystem::path ToFullPath(const std::filesystem::path& relativePath) const;

        //Reads the file from the given path and returns it.
        //Returns nothing if the file couldn't be loaded.
        std::optional<std::string> GetFile(const std::filesystem::path& relativePath);

        //Reads the file from the given path and appends its contents to the given buffer.
        //Returns whether the file was successfully loaded.
        bool GetFile(const std::filesystem::path& relativePath, std::string& output);


    private:

        std::filesystem::path relativePath;
        Cache_t fileCache;
    };


    //A shader that can be loaded and compiled,
    //    with a bit of pre-processing to support "#pragma include(...)" statements.
    class BP_API ShaderCompileJob
    {
    public:
        
        //Gets a compiled binary blob of the given shader program.
        //This blob should replace the need for compilation,
        //    at least until the user's hardware/drivers change.
        //Returns true if it succeeded, or false if the program isn't valid.
        static bool GetCompiledBinary(OglPtr::ShaderProgram program,
                                      std::vector<std::byte>& outData);


        //Shader source codes (any empty ones are considered nonexistent):
        std::string VertexSrc, FragmentSrc,
                    GeometrySrc;
        //TODO: ComputeSource?
        
        //Given a "#pragma include(...)" statement in the GLSL, this function should
        //    load that "file" and append it to the given output string.
        std::function<FileContentsAppender> IncludeImplementation;

        //If non-empty, this is a pre-compiled binary blob representing this shader.
        //This blob will be attempted to 
        std::vector<std::byte> CachedBinaryProgram;


        //Pre-processes the given shader source to execute any "#pragma include(...)" statements.
        //Edits the string in-place.
        void PreProcessIncludes(std::string& sourceStr) const;

        //Compiles this program into an OpenGL object and returns it.
        OglPtr::ShaderProgram Compile() const;
    };
}