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


PreCompiledShader::PreCompiledShader(OglPtr::ShaderProgram program)
{
    GLint byteSize;
    glGetProgramiv(program.Get(), GL_PROGRAM_BINARY_LENGTH, &byteSize);
    BP_ASSERT(byteSize > 0, "Program isn't successfully compiled");

    Data.resize(byteSize);
    glGetProgramBinary(program.Get(), byteSize, nullptr, &Header, Data.data());
}


size_t ShaderCompileJob::MaxIncludesPerFile = 100;

void ShaderCompileJob::Clear(bool removeCachedBinary)
{
    VertexSrc.clear();
    GeometrySrc.clear();
    FragmentSrc.clear();

    if (removeCachedBinary)
        CachedBinary.reset();
    else if (CachedBinary.has_value())
        CachedBinary->Data.clear();
}

void ShaderCompileJob::PreProcessIncludes()
{
    if (!VertexSrc.empty())
        PreProcessIncludes(VertexSrc);
    if (!FragmentSrc.empty())
        PreProcessIncludes(FragmentSrc);
    if (!GeometrySrc.empty())
        PreProcessIncludes(GeometrySrc);
}

void ShaderCompileJob::PreProcessIncludes(std::string& sourceStr) const
{
    std::stringstream strBuffer;

    //Search the code sequentially, skipping over comments, until we find an include statement.
    //Replace it with the contents of the named file,
    //    then continue from the beginning of that inserted file
    //    so that any nested includes are caught.
    //Set a hard max on the number of includes that can be processed,
    //    to avoid infinite loops.
    //Use #line commands to manage line numbers so that compile errors make sense.
    std::vector<size_t> lineStack, fileIndexStack;
    lineStack.push_back(1);
    fileIndexStack.push_back(0);
    size_t nextFileIndex = 1;
    size_t includeCount = 0;
    enum class CommentModes { None, SingleLine, MultiLine };
    CommentModes commentMode = CommentModes::None;
    for (size_t i = 0; i < sourceStr.size(); ++i)
    {
        bool isEOF = (i == sourceStr.size() - 1),
             isNextEOF = isEOF | (i + 1 == sourceStr.size() - 1);
        auto thisChar = sourceStr[i],
             nextChar = isEOF ? '\0' : sourceStr[i + 1],
             nextChar2 = isNextEOF ? '\0' : sourceStr[i + 2];

        //A null terminator signifies the end of an included sub-file.
        if (thisChar == '\0')
        {
            lineStack.pop_back();
            fileIndexStack.pop_back();

            //Remove the null terminator.
            sourceStr.erase(i, 1);
            i -= 1;
        }
        //If this is a line break, count it.
        else if (thisChar == '\n' || thisChar == '\r')
        {
            lineStack.back() += 1;
            
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
            lineStack.back() += 1;
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
                        //NOTE: Originally a lot of the error messages included the term
                        //    '#pragma include', but that led to an infinite loop
                        //    of the parser attempting to parse it, changing the include statement
                        //    to an error, then attempting to parse that error...
                        strBuffer.str("");

                        //Skip ahead to the first non-white-space character,
                        //    which should be the start of the path.
                        j += includeLen;
                        while (j < sourceStr.size() && (sourceStr[j] == ' ' || sourceStr[j] == '\t'))
                            j += 1;

                        //Process the character we just found.
                        if (j == sourceStr.size() || sourceStr[j] == '\n' || sourceStr[j] == '\r')
                            strBuffer << "#error No file given in 'pragma include' statement";
                        else if (sourceStr[j] != '<' && sourceStr[j] != '"')
                            strBuffer << "#error Unexpected symbol in a 'pragma include'; \
expected a path, starting with a double-quote '\"' or angle-bracket '<'";
                        else
                        {
                            //The path name starts after this character.
                            j += 1;
                            size_t pathStart = j;
                         
                            //Find the end of the path name.
                            char expectedPathEnd = (sourceStr[j - 1] == '<') ? '>' : '"';
                            j += 1; //TODO: Remove this, right? And add a check for an empty #include
                            while (j < sourceStr.size() && sourceStr[j] != expectedPathEnd &&
                                   sourceStr[j] != '\n' && sourceStr[j] != '\r')
                            {
                                j += 1;
                            }
                            if (j == sourceStr.size() || sourceStr[j] == '\n' || sourceStr[j] == '\r')
                                strBuffer << "#error unexpected end of 'pragma include' statement; \
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
                                                 " 'pragma include' statements in one file";
                                }
                                else
                                {
                                    includeCount += 1;

                                    //Try to load the file.
                                    //If it succeeds, insert a #line statement before and after the file contents.
                                    //If it fails, replace it with an #error message.
                                    strBuffer << "\n#line 1 " << nextFileIndex << "\n";
                                    nextFileIndex += 1;
                                    if (IncludeImplementation(fs::path(pathName), strBuffer))
                                    {
                                        //Insert the #line command to put things back to normal.
                                        //Also insert a null terminator to represent the point
                                        //    where the line number should be popped back off
                                        //    the stack.
                                        //We have to be careful to insert it on its own,
                                        //    or it'll interrupt whatever string it's a part of!
                                        strBuffer << "\n#line " << lineStack.back() <<
                                                     " " << fileIndexStack.back() <<
                                                    '\n' << '\0';
                                        fileIndexStack.push_back(nextFileIndex - 1);
                                        lineStack.push_back(1);
                                    }
                                    else
                                    {
                                        strBuffer.str("");
                                        strBuffer << "#error unable to 'pragma include' file: ";

                                        //Edge-case: make sure the file name doesn't have
                                        //    '#pragma include' in it, or this parser loops forever.
                                        Strings::Replace(pathName, "#", "#\\\\");
                                        strBuffer << pathName;
                                    }
                                }
                            }
                        }

                        //Finally, replace this pragma with the file contents.
                        //Note that 'j' will always be the first index *after* the relevant substring;
                        //    we do not want to remove the character at index j.
                        sourceStr.erase(i, j - i + 1);
                        sourceStr.insert(i, strBuffer.str());
                    }
                }
            }
        }
    }
}


namespace
{
    //A buffer used during shader compilation.
    //Made thread_local so that shader compilation is thread-safe.
    thread_local std::vector<char> compileErrorMsgBuffer;

    //Attempts to compile the given shader program (vertex, or fragment, or compute, etc).
    //Returns the error message, or an empty string if it was successful.
    std::string TryCompile(GLuint shaderObject)
    {
        glCompileShader(shaderObject);

        GLint isCompiled = 0;
        glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE)
        {
            GLint msgLength = 0;
            glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &msgLength);

            compileErrorMsgBuffer.resize(msgLength, '\0');
            glGetShaderInfoLog(shaderObject, msgLength, &msgLength,
                               compileErrorMsgBuffer.data());

            return std::string(compileErrorMsgBuffer.data());
        }

        return "";
    }
}

std::tuple<std::string, bool> ShaderCompileJob::Compile(OglPtr::ShaderProgram& outPtr)
{
    outPtr = OglPtr::ShaderProgram(glCreateProgram());

    //Try to use the pre-compiled binary blob.
    bool updateBinary = false;
    if (CachedBinary.has_value())
    {
        glProgramBinary(outPtr.Get(), CachedBinary.value().Header,
                        CachedBinary.value().Data.data(),
                        (GLsizei)CachedBinary.value().Data.size());

        GLint linkStatus;
        glGetProgramiv(outPtr.Get(), GL_LINK_STATUS, &linkStatus);
        if (linkStatus == GL_TRUE)
            return std::make_tuple("", false);
        else
            updateBinary = true;
    }

    //Generate the OpenGL/extensions declarations for the top of the shader files.
    std::string shaderPrefix = Context::GLSLVersion();
    shaderPrefix += "\n";
    for (auto extension : Context::GLSLExtensions())
    {
        shaderPrefix += extension;
        shaderPrefix += "\n";
    }
    //Add a preprocessor definition that resets the line count.
    shaderPrefix += "\n#line 1 0\n";

    //Store per-shader information into an array for easier processing:
    struct StageData {
        std::string* Source;
        const char* StageName;
        GLenum Stage;
        GLuint Handle;
    };
    std::vector<StageData> shaderTypes;
    for (auto& dataSource : std::array{
            StageData{ &VertexSrc,   "vertex",   GL_VERTEX_SHADER,   0 },
            StageData{ &GeometrySrc, "geometry", GL_GEOMETRY_SHADER, 0 },
            StageData{ &FragmentSrc, "fragment", GL_FRAGMENT_SHADER, 0 }
         })
    {
        if (dataSource.Source->size() > 0)
            shaderTypes.push_back(dataSource);
    }

    //For each shader type that was given, try to insert the header
    //    if it doesn't exist already.
    for (auto& shaderData : shaderTypes)
    {
        auto& shaderSrc = *shaderData.Source;
        if (shaderSrc.size() < shaderPrefix.size() ||
            shaderSrc.compare(0, shaderPrefix.size(), shaderPrefix) != 0)
        {
            shaderSrc.insert(0, shaderPrefix);
        }
    }

    //Create and compile each shader type.
    for (auto& shaderData : shaderTypes)
    {
        //Create the shader's OpenGL object.
        shaderData.Handle = glCreateShader(shaderData.Stage);

        //Set the source code.
        auto* shaderSrcPtr = shaderData.Source->c_str();
        glShaderSource(shaderData.Handle, 1, &shaderSrcPtr, nullptr);

        //Try to compile it.
        auto errorMsg = TryCompile(shaderData.Handle);
        if (errorMsg.size() > 0)
        {
            errorMsg = std::string("Error compiling ") + shaderData.StageName +
                          ": " + errorMsg;
            
            //Clean up OpenGL stuff.
            for (auto& shaderData : shaderTypes)
                glDeleteShader(shaderData.Handle);
            glDeleteProgram(outPtr.Get());
            outPtr = OglPtr::ShaderProgram::Null();

            return std::make_tuple(errorMsg, false);
        }
    }

    //Next, link all the shaders together.
    for (auto& shaderData : shaderTypes)
        glAttachShader(outPtr.Get(), shaderData.Handle);
    glLinkProgram(outPtr.Get());

    //Clean up the shader objects.
    //Note that they aren't actually deleted until the program itself is deleted,
    //    or the shader objects are manually detatched from the program.
    for (auto& shaderData : shaderTypes)
        glDeleteShader(shaderData.Handle);

    //Check the result of the link.
    GLint isSuccessful = 0;
    glGetProgramiv(outPtr.Get(), GL_LINK_STATUS, &isSuccessful);
    if (isSuccessful == GL_FALSE)
    {
        GLint msgLength = 0;
        glGetProgramiv(outPtr.Get(), GL_INFO_LOG_LENGTH, &msgLength);

        std::vector<char> msgBuffer(msgLength);
        glGetProgramInfoLog(outPtr.Get(), msgLength, &msgLength, msgBuffer.data());

        glDeleteProgram(outPtr.Get());
        outPtr = OglPtr::ShaderProgram::Null();

        return std::make_tuple("Error linking shader stages together : " +
                                   std::string(msgBuffer.data()),
                               false);
    }

    //If the link was successful, we need to "detach" the shader objects
    //    from the main program object, so that they can be cleaned up.
    for (auto& shaderData : shaderTypes)
        glDetachShader(outPtr.Get(), shaderData.Handle);

    //Finally, update the cached binary if necessary.
    if (updateBinary)
        CachedBinary = { outPtr };

    return std::make_tuple("", updateBinary);
}