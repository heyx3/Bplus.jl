#pragma once

#include <random>

#define TEST_NO_MAIN
#include <acutest.h>

#include <B+/Renderer/Materials/CompiledShader.h>
#include "../SimpleApp.hpp"

using namespace Bplus;
using namespace Bplus::GL;


namespace
{
    ShaderCompileJob compiler = ShaderCompileJob([&]
        (const fs::path& path, std::stringstream& out) {
            auto pathStr = path.string();

            if (pathStr.substr(0, std::min(5ULL, pathStr.size())) == "FAIL-")
                return false;

            out << pathStr;
            return true;
        });

    void RunShaderInclude(std::string src,
                          std::function<void(const std::string&)> processResult)
    {
        compiler.PreProcessIncludes(src);
        processResult(src);
    }

    //Tests that running the shader #pragma include preprocessor on the given string
    //    changes it into the given "expected" string.
    void TestShaderInclude(const std::string& testName,
                           const std::string& src,
                           const std::string& expected,
                           std::function<void(const std::string&)> onFailure = [](const auto&) { })
    {
        TEST_CASE(testName.c_str());

        std::string actual = src;
        compiler.PreProcessIncludes(actual);
        std::string lineBreakVisual = "[[\\n]]\n";
        std::string cmdFriendlySrc = Strings::ReplaceNew(src, "\n", lineBreakVisual),
                    cmdFriendlyExpected = Strings::ReplaceNew(expected, "\n", lineBreakVisual),
                    cmdFriendlyActual = Strings::ReplaceNew(actual, "\n", lineBreakVisual);
        if (!TEST_CHECK_(actual == expected,
                         "Input (next line, inside braces):\n{%s}\n"
                             "----------------------\n"
                             "Expected (next line, inside braces):\n{%s}\n"
                             "----------------------\n"
                             "Output (next line, inside braces):\n{%s}",
                         cmdFriendlySrc.c_str(),
                         cmdFriendlyExpected.c_str(),
                         cmdFriendlyActual.c_str()))
        {
            //Find the character and line at which they're different.
            size_t charI, lineI, differenceI;
            bool wasDifferent = Strings::FindDifference(expected, actual,
                                                        differenceI, charI, lineI);

            //They have to be different somewhere; otherwise the test would have passed.
            assert(wasDifferent);

            //Report the nature of the failure.
            if (differenceI >= expected.size())
                TEST_MSG("Output has extra characters");
            else if (differenceI >= actual.size())
                TEST_MSG("Output has too few characters");
            else
                TEST_MSG("Input and output differ at line %i, character %i: "
                             "expected '%c' but got '%c'",
                         lineI, charI, expected[differenceI], actual[differenceI]);

            onFailure(actual);
        }
    }
    //Tests that running the shader #pragma include preprocessor on the given string
    //    does not change it at all.
    void TestShaderIncludeUnchanged(const std::string& testName,
                                    const std::string& src,
                                    std::function<void(const std::string&)> onFailure = [](const auto&) { })
    {
        TEST_CASE(testName.c_str());

        std::string actual = src;
        compiler.PreProcessIncludes(actual);
        if (!TEST_CHECK_(actual == src,
                         "Input (next line):\n%s\n"
                             "----------------------\n"
                             "Expected (next line):\n%s\n"
                             "----------------------\n"
                             "Output (next line):\n%s",
                         src.c_str(), src.c_str(), actual.c_str()))
        {
            //Find the character and line at which they're different.
            size_t charI, lineI, differenceI;
            bool wasDifferent = Strings::FindDifference(src, actual,
                                                        differenceI, charI, lineI);

            //They have to be different somewhere; otherwise the test would have passed.
            assert(wasDifferent);

            //Report the nature of the failure.
            if (differenceI >= src.size())
                TEST_MSG("Output has extra characters");
            else if (differenceI >= actual.size())
                TEST_MSG("Output has too few characters");
            else
                TEST_MSG("Input and output differ at line %i, character %i: "
                             "expected '%c' but got '%c'",
                         lineI, charI, src[differenceI], actual[differenceI]);

            onFailure(actual);
        }
    }
}

void TestShaderIncludeCommand()
{
    //Run some tests!
    TEST_CASE("Small tests");

    TestShaderIncludeUnchanged("Empty string", "");
    TestShaderIncludeUnchanged("Plain multi-line string",
                               "123454321 hi there\nHe,,,llo\nwo.,.,ef\
ji!)(*)!(*$)!($!oweijf");
    TestShaderIncludeUnchanged("Weird multi-line stuff with preprocessor symbols but no includes",
"#define A a\n\
         #   hello there      #\n\
# ## ### ####\n\
#\n\
\n\
#pragma haha  \n\
#pragma dontinclude\n\
#include \"this isn't a noticeable include statement\"\n\
#include <This isn't either>");
    
    TestShaderInclude("Basic include statement with brackets",
                      "#pragma include <hello>", "\n#line 0 1\nhello\n#line 1 0\n");
    TestShaderInclude("Basic include statement with quotes",
                      "#pragma include \"hello2\"", "\n#line 0 1\nhello2\n#line 1 0\n");
    TestShaderInclude("Putting quotes inside an angle-bracket include",
                      "#pragma include <a\"b\">", "\n#line 0 1\na\"b\"\n#line 1 0\n");
    TestShaderInclude("Putting angle-brackets inside a quoted include",
                      "#pragma include \"a<b>\"", "\n#line 0 1\na<b>\n#line 1 0\n");
    TestShaderInclude("Preserves white-space after the include statement",
                      "#pragma include <abcd> ", "\n#line 0 1\nabcd\n#line 1 0\n ");
    TestShaderInclude("Preserves white-space after the include statement",
                      "#pragma include \"abcd\"  ", "\n#line 0 1\nabcd\n#line 1 0\n  ");
    TestShaderInclude("Preserves text right after the include statement",
                      "#pragma include <abcd>efgh", "\n#line 0 1\nabcd\n#line 1 0\nefgh");
    RunShaderInclude("#pragma include FAIL-ldskjflksjdfksjdlkj",
                     [](const auto& resultStr) {
                         TEST_CASE("Simple failure");
                         TEST_CHECK_(strncmp(resultStr.c_str(), "#error", 6) == 0,
                                    "Failed include should result in an #error");
                     });
    TestShaderInclude("Ignore whitespace in between tokens in the include statement",
                      " #    pragma\t  include\t   \t    \t  <success.jpg>",
                      " \n#line 0 1\nsuccess.jpg\n#line 1 0\n");

    TestShaderInclude("Large file with many successful includes plus some gibberish",
"#pragma include <hello there>\n\
#pragma include \"hi\"\n\
3\n\
4 # pragma include \"30,000\" 50,000",
//-------------------------------------------
"\n#line 0 1\nhello there\n#line 1 0\n\
#line 0 2\nhi\n#line 2 0\n\
3\n\
4 \n\
#line 0 3\n30,000\n#line 3 0\n\
 50,000");

    //Do a more comprehensive test about error includes and multi-line strings.
    TEST_CASE("Big test with a multi-line string with multiple include errors");

    //The pre-processor modifies strings in-place.
    std::string shaderSrcPre =
        "abc123\n\
#pragma include FAIL-zxcv\n\
#pragma include FAIL-asdf\n\
#pragma incude FAIL-qwertyuiop\n\
def456\n\
\n\
#pragma include FAIL-123456789\n\
";
    std::string shaderSrcPost = shaderSrcPre;
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

//TODO: Test the 'including from disk' helper class.
//TODO: Test nested includes.
//TODO: Test actual shader compilation.