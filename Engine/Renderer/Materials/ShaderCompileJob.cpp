#include "ShaderCompileJob.h"

using namespace Bplus;
using namespace Bplus::GL;


std::string ShaderIncluderFromFiles::ToFullPath(const fs::path& path) const
{
    //Make sure the path is unambiguous by using fs::canonical().
    //Otherwise we may have problems using it as a key in the cache.
    return fs::canonical(relativePath / path).string();
}

void ShaderIncluderFromFiles::SetCacheEntry(const fs::path& key, const std::string& value)
{
    fileCache[ToFullPath(key)] = value;
}

std::optional<std::string> ShaderIncluderFromFiles::GetFile(const fs::path& relativePath)
{
    std::stringstream contents;
    if (GetFile(relativePath, contents))
        return contents.str();
    else
        return std::nullopt;
}
bool ShaderIncluderFromFiles::GetFile(const std::filesystem::path& relativePath,
                                      std::stringstream& output)
{
    auto fullPath = ToFullPath(relativePath);
    
    //If it exists in the cache already, retrieve it.
    auto cachedValue = fileCache.find(fullPath);
    if (cachedValue != fileCache.end())
    {
        output << cachedValue->second;
        return true;
    }

    //Otherwise, try to load it and store in the cache.
    std::string fileContents;
    if (IO::LoadEntireFile(fullPath, fileContents))
    {
        fileCache[fullPath] = fileContents;
        output << fileContents;
        return true;
    }
    else
    {
        return false;
    }
}


size_t ShaderCompileJob::MaxIncludesPerFile = 100;

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
    //TODO: write "#line 0 [n]" right above an include, and "#line [x] 0" right after the include ends.
    //TODO: If the include fails, write "#error Couldn't load [path]"

    std::stringstream strBuffer;

    //Search the code sequentially, skipping over comments, until we find an include statement.
    //Replace it with the contents of the named file,
    //    then continue from the beginning of that inserted file
    //    so that any nested includes are caught.
    //Set a hard max on the number of includes that can be processed,
    //    to avoid infinite loops.
    //Use #line commands to keep line numbers in line with the origial files.
    size_t currentLine = 1,
           nextFileI = 1;
    size_t includeCount = 0;
    enum class CommentModes { None, SingleLine, MultiLine };
    CommentModes commentMode = CommentModes::None;
    for (size_t i = 0; i < sourceStr.size(); ++i)
    {
        bool isEOF = (i == sourceStr.size() - 1),
             isNextEOF = (i + 1 == sourceStr.size() - 1);
        auto thisChar = sourceStr[i],
             nextChar = isEOF ? '\0' : sourceStr[i + 1],
             nextChar2 = isNextEOF ? '\0' : sourceStr[i + 2];

        //If this is a line break, count it.
        if (thisChar == '\n' || thisChar == '\r')
        {
            currentLine += 1;
            
            //If we were in a single-line comment, end it.
            if (commentMode == CommentModes::SingleLine)
                commentMode = CommentModes::None;

            //Some line breaks are two characters long -- \n\r or \r\n.
            if (nextChar != thisChar && (nextChar == '\n' || nextChar == '\r'))
                i += 1;
        }
        //If this is an escaped line break, we should ignore it
        //   (but still increment the line number).
        else if (thisChar == '\\' && (nextChar == '\n' || nextChar == '\r'))
        {
            currentLine += 1;
            i += 1;

            //Some line breaks are two characters long -- \n\r or \r\n.
            if (nextChar2 != nextChar && (nextChar2 == '\n' || nextChar2 == '\r'))
                i += 1;
        }
        //If we are inside a multi-line comment, ignore anything else
        //    and keep moving forward until we exit it.
        else if (commentMode == CommentModes::MultiLine)
        {
            if (thisChar == '*' && nextChar == '/')
            {
                i += 1;
                commentMode = CommentModes::None;
            }
        }
        //If we're in a single-line comment, don't bother looking at anything else.
        else if (commentMode == CommentModes::SingleLine)
        {
            BP_NOOP;
        }
        //If this is the start of a single-line comment, mark that down.
        else if (thisChar == '/' && nextChar == '/')
        {
            commentMode = CommentModes::SingleLine;
        }
        //If this is the start of a multi-line comment, mark that down.
        else if (thisChar == '/' && nextChar == '*')
        {
            commentMode = CommentModes::MultiLine;
        }
        //If this is a '#' sign, look for the rest of the command.
        else if (thisChar == '#')
        {
            //White-space between the '#' and the actual command is allowed.
            //Skip over that stuff.
            size_t j = i + 1;
            while (j < sourceStr.size() && (sourceStr[j] == ' ' || sourceStr[j] == '\t'))
                j += 1;

            //Is the next token the word "pragma"?
            const char* pragma = "pragma";
            const size_t pragmaLen = strlen(pragma);
            if (j + (pragmaLen - 1) < sourceStr.size() &&
                sourceStr.compare(j, pragmaLen, pragma) == 0)
            {
                //White-space between 'pragma' and 'include' is required.
                //Skip over that stuff.
                j += pragmaLen;
                if (j < sourceStr.size() && (sourceStr[j] == ' ' || sourceStr[j] == '\t'))
                {
                    while (j < sourceStr.size() && (sourceStr[j] == ' ' || sourceStr[j] == '\t'))
                        j += 1;

                    //Is the next token the word "include"?
                    const char* include = "include";
                    const size_t includeLen = strlen(include);
                    if (j + (includeLen - 1) < sourceStr.size() &&
                        sourceStr.compare(j, includeLen, include) == 0)
                    {
                        //We're definitely going to 'include' something,
                        //    whether it's an actual file or an error message.
                        strBuffer.str("");

                        //Skip ahead to the first non-white-space character,
                        //    which should be the start of the path.
                        while (j < sourceStr.size() && (sourceStr[j] == ' ' || sourceStr[j] == '\t'))
                            j += 1;

                        //Process the character we just found.
                        if (j == sourceStr.size() || sourceStr[j] == '\n' || sourceStr[j] == '\r')
                            strBuffer << "#error No file given in '#pragma include' statement";
                        else if (sourceStr[j] != '<' && sourceStr[j] != '"')
                            strBuffer << "#error Unexpected symbol in a '#pragma include'; \
expected a path, starting with a double-quote '\"' or angle-bracket '<'";
                        else
                        {
                            //The path name starts after this character.
                            j += 1;
                            size_t pathStart = j;
                         
                            //Find the end of the path name.
                            char expectedPathEnd = (sourceStr[j] == '<') ? '>' : '"';
                            j += 1;
                            while (j < sourceStr.size() && sourceStr[j] != expectedPathEnd &&
                                   sourceStr[j] != '\n' && sourceStr[j] != '\r')
                            {
                                j += 1;
                            }
                            if (j == sourceStr.size() || sourceStr[j] == '\n' || sourceStr[j] == '\r')
                                strBuffer << "#error unexpected end of '#pragma include' statement; \
expected double-quote '\"' or angle-bracket '>' to close it";
                            else
                            {
                                size_t pathEnd = j - 1;
                                auto pathName = sourceStr.substr(pathStart,
                                                                 pathEnd - pathStart + 1);

                                //If we've included too many files already,
                                //    stop and print an error message.
                                if (includeCount >= MaxIncludesPerFile)
                                {
                                    strBuffer << "#error Infinite loop detected: more than " << includeCount <<
                                                 " '#pragma include' statements in one file";
                                }
                                else
                                {
                                    includeCount += 1;

                                    //Try to load the file.
                                    //If it succeeds, insert a #line statement before and after the file contents.
                                    //If it fails, replace it with an #error message.
                                    strBuffer << "#line 0 " << nextFileI << "\n";
                                    nextFileI += 1;
                                    if (IncludeImplementation(fs::path(pathName), strBuffer))
                                    {
                                        strBuffer << "\n#line " << currentLine << " 0";
                                    }
                                    else
                                    {
                                        strBuffer.str("#error unable to '#pragma include' file: ");
                                        strBuffer << pathName;
                                    }
                                }
                            }
                        }

                        //Finally, replace this pragma with the file contents.
                        //Note that 'j' will always be the first index *after* the relevant substring;
                        //    we do not want to remove the character at index j.
                        sourceStr.erase(i, j - i);
                        sourceStr.insert(i, strBuffer.str());
                        j -= 1;
                    }
                }
            }
        }
    }
}

std::string ShaderCompileJob::Compile(OglPtr::ShaderProgram& outPtr)
{
    //TODO: Implement.

    return "";
}