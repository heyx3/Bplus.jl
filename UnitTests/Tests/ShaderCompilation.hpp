#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Materials/CompiledShader.h>
#include "../SimpleApp.hpp"

using namespace Bplus;
using namespace Bplus::GL;


void TestShaderIncludeCommand()
{
    //Spin up a shader compiler that "loads files"
    //    by just pasting the file name.
    //Except for any file beginning with "FAIL-",
    //    which fails to load.
    ShaderCompileJob compiler;
    compiler.IncludeImplementation =
        [](const fs::path& path, std::stringstream& out)
        {
            auto pathStr = path.string();

            if (pathStr.substr(0, std::min(5ULL, pathStr.size())) == "FAIL-")
                return false;

            out << pathStr;
            return true;
        };


    //The pre-processor modifies strings in-place.
    std::string shaderSrcPre, shaderSrcPost;

    //This macro tests that a string passes a given test after parsing.
#define TEST_PREPROCESS_INCLUDES(testStr, test, testName) \
    shaderSrcPre = testStr; \
    shaderSrcPost = shaderSrcPre; \
    compiler.PreProcessIncludes(shaderSrcPost); \
    if (!TEST_CHECK_(test, testName)) \
        TEST_MSG("Input (next line):\n%s\n---------------------------\nOutput (next line):\n%s", shaderSrcPre.c_str(), shaderSrcPost.c_str())

    //This macro tests that one string parses into another string.
#define TEST_PREPROCESS_INCLUDES_BASIC(testStr, resultStr, testName) \
    TEST_PREPROCESS_INCLUDES(testStr, shaderSrcPost == resultStr, testName)

    //This macro tests that a string does not change after parsing.
#define TEST_PREPROCESS_INCLUDES_UNCHANGED(testStr, testName) \
    TEST_PREPROCESS_INCLUDES_BASIC(testStr, testStr, testName)

    //Run some tests!
    TEST_CASE("Small tests");

    TEST_PREPROCESS_INCLUDES_UNCHANGED("", "Empty string");
    TEST_PREPROCESS_INCLUDES_UNCHANGED(
        "123454321 hi there\nHe,,,llo\nwo.,.,ef\
ji!)(*)!(*$)!($!oweijf",
        "Plain multi-line string"
    );
    TEST_PREPROCESS_INCLUDES_UNCHANGED(
        "#define A a\n\
         #   hello there      #\n\
# ## ### ####\n\
#\n\
\n\
#pragma haha  \n\
#pragma dontinclude\n\
#include \"this isn't a noticeable include statement\"\n\
#include <This isn't either>",
        "Weird multi-line stuff with preprocessor symbols but no includes"
    );

    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include <hello>", "\n#line 0 1\nhello\n#line 1 0\n",
                                   "Basic include statement with brackets");
    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include \"hello2\"", "\n#line 0 1\nhello2\n#line 1 0\n",
                                   "Basic include statement with quotes");
    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include <a\"b\">", "\n#line 0 1\na\"b\"\n#line 1 0\n",
                                   "Putting quotes inside an angle-bracket include");
    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include \"a<b>\"", "\n#line 0 1\na<b>\n#line 1 0\n",
                                   "Putting angle-brackets  inside a quoted include");
    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include <abcd> ", "\n#line 0 1\nabcd\n#line 1 0\n ",
                                   "Preserves white-space after the include statement");
    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include \"abcd\"  ", "\n#line 0 1\nabcd\n#line 1 0\n  ",
                                   "Preserves white-space after the include statement");
    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include \"abcd\"  ", "\n#line 0 1\nabcd\n#line 1 0\n  ",
                                   "Preserves white-space after the include statement");
    TEST_PREPROCESS_INCLUDES_BASIC("#pragma include <abcd>efgh", "\n#line 0 1\nabcd\n#line 1 0\nefgh",
                                   "Preserves text right after the include statement");
    TEST_PREPROCESS_INCLUDES("#pragma include FAIL-ldskjflksjdfksjdlkj",
                             strncmp(shaderSrcPost.c_str()  BP_COMMA
                                     "#error"               BP_COMMA
                                     6) == 0,
                             "Failed include should result in an #error"
    );
    TEST_PREPROCESS_INCLUDES_BASIC(" #    pragma\t  include\t   \t    \t  <success.jpg>",
                                   " \n#line 0 1\nsuccess.jpg\n#line 1 0\n",
                                   "Ignore whitespace in between tokens in the include statement");

    TEST_PREPROCESS_INCLUDES_BASIC
    (
        "#pragma include <hello there>\n\
#pragma include \"hi\"\n\
3\n\
4 # pragma include \"30,000\" 50,000",
//------------------------------------------
        "hello there\n\
hi\n\
3\n\
4 30,000 50,000",
        "\n#line 0 1\nhello there\n\#line 1 0\n\
#line 0 2\nhi\n#line 2 0\n\
3\n\
4 \n\
#line 0 3\n30,000\n#line 3 0\n\
 50,000",
//------------------------------------------
        "Large file with many successful includes plus some gibberish"
    );

#undef TEST_PREPROCESS_INCLUDES_BASIC
#undef TEST_PREPROCESS_INCLUDES_UNCHANGED
#undef TEST_PREPROCESS_INCLUDES


    //Do a more comprehensive test about error includes and multi-line strings.
    TEST_CASE("Big test with a multi-line string with multiple include errors");
    shaderSrcPre =
        "abc123\n\
#pragma include FAIL-zxcv\n\
#pragma include FAIL-asdf\n\
#pragma incude FAIL-qwertyuiop\n\
def456\n\
\n\
#pragma include FAIL-123456789\n\
";
    shaderSrcPost = shaderSrcPre;
    compiler.PreProcessIncludes(shaderSrcPre);

    //Split the processed shader into individual lines to run tests.
    std::vector<std::string> lines;
    std::stringstream lineReader(shaderSrcPost);
    std::string tempLine;
    while (std::getline(lineReader, tempLine, '\n'))
        lines.push_back(tempLine);

    //Run the tests.
    TEST_ASSERT_(lines.size() == 8, "Expected 8 lines");
    TEST_ASSERT_(lines[0] == "abac123", "Line [0] should be unchanged");
    TEST_ASSERT_(Strings::StartsWith(lines[1], "#error")
                    && lines[1].find("zxcv") != std::string::npos,
                 "Line [1] should be an #error about including 'zxcv'");
    TEST_ASSERT_(Strings::StartsWith(lines[2], "#error")
                    && lines[2].find("asdf") != std::string::npos,
                 "Line [2] should be an #error about including 'asdf'");
    TEST_ASSERT_(lines[3] == "#pragma incude FAIL-qwertyuiop",
                 "Line [3] is an intentional typo and should be left unchanged");
    TEST_ASSERT_(lines[4] == "def456", "Line [4] should be unchanged");
    TEST_ASSERT_(lines[5] == "", "Line [5] is empty");
    TEST_ASSERT_(Strings::StartsWith(lines[6], "#error")
                    && lines[6].find("123456789") != std::string::npos,
                 "Line [6] should be an #error about including '123456789'");
}

//TODO: Test the including from disk helper class.
//TODO: Test nested includes.
//TODO: Test actual shader compilation.