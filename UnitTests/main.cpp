//Runs unit tests for BPlus.
//Uses the "acutest" single-header unit test library: https://github.com/mity/acutest


//Before running tests, we want to make BP_ASSERT fail in a way that acutest can handle.
#include <string>
#include <B+/Utils/BPAssert.h>
namespace
{
    bool didAssertFail;
    std::string fakeExceptionMsg;

    void SetupTesting()
    {
        Bplus::SetAssertFunc([](bool condition, const char* msg) {
            if (!condition)
            {
                didAssertFail = true;
                fakeExceptionMsg = msg;
            }
        });
    }
}

//This custom version of acutest's TEST_EXCEPTION_ is very similar to the original,
//    but with the additional use of the "didAssertHappen" flag above.
#define TEST_EXCEPTION_(code, exctype, ...)                                    \
    do {                                                                       \
        bool exc_ok__ = false;                                                 \
        didAssertFail = false;                                                 \
        fakeExceptionMsg = "";                                                 \
        const char *msg__ = NULL;                                              \
        try {                                                                  \
            code;                                                              \
            if (didAssertFail) {                                               \
                exc_ok__ = true;                                               \
                fakeExceptionMsg = "BP_ASSERT fail: " + fakeExceptionMsg;      \
                msg__ = fakeExceptionMsg.data();                               \
            } else {                                                           \
                msg__ = "No exception thrown.";                                \
            }                                                                  \
        } catch(exctype const&) {                                              \
            exc_ok__= true;                                                    \
        } catch(...) {                                                         \
            msg__ = "Unexpected exception thrown.";                            \
        }                                                                      \
        test_check__(exc_ok__, __FILE__, __LINE__, __VA_ARGS__);               \
        if(msg__ != NULL)                                                      \
            test_message__("%s", msg__);                                       \
    } while(0)

//Import acutest.
#define TEST_STARTUP() SetupTesting()
#include <acutest.h>


//Import the tests.
//AllTests.h is an auto-generated code file.
//It is generated as a pre-build step in Visual Studio,
//    so if you're getting an error about it not existing,
//    just hit "Compile" to fix.
#include "AllTests.h"


TEST_LIST = {
    { "ToBaseString()", TestToBaseString },
    { "Strings::StartsWith()", TestStringStartsWith },
    { "Strings::EndsWith()", TestStringEndsWith },
    { "Strings::Replace() and ::ReplaceNew()", TestStringReplace },

    { "Toml basic tests",         TomlBasic      },
    { "Toml wrapping/unwrapping", TomlWrapping   },
    { "Toml <=> primitives",      TomlPrimitives },
    { "Toml <=> BETTER_ENUM",     TomlEnums      },
    { "Toml <=> GLM",             TomlGLM        },

    { "Math: Plain Number-crunching",    PlainMath },
    { "Math: GLM Helpers",               GLMHelpers },

    { "Bplus::GL::Buffers::Buffer basic",     BufferBasic   },
    { "Bplus::GL::Buffers::Buffer get/set data", BufferGetSetData },

    { "Bplus::GL::Texture creation", TextureCreation },
    { "Bplus::GL::Texture get/set data", TextureSimpleGetSetData },

    { "Bplus::GL::Target basic usage", TestTargetBasic },

    { "Shader #pragma include preprocessor", TestShaderIncludeCommand },

    { "SceneTree basic manipulation", ST_BasicManipulation },

    //TODO: Tests for image asset loading, and Utils/Streams.hpp

    //The below tests are interactive apps, so they're normally disabled to simplify testing.
    //{ "Simple App", SimpleApp },
    //{ "Basic Rendering App", BasicRenderApp },
    //{ "Advanced Textures App", AdvancedTexturesApp },

    { NULL, NULL }
};