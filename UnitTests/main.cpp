//Runs unit tests for BPlus.
//Uses the "acutest" single-header unit test library: https://github.com/mity/acutest

#include <acutest.h>

//AllTests.h is an auto-generated code file.
//It is generated as a pre-build step in Visual Studio,
//    so if you're getting an error about it not existing,
//    just hit "Compile" to fix.
#include "AllTests.h"


TEST_LIST = {
   /* { "ToBaseString()", TestToBaseString },
    { "Strings::StartsWith()", TestStringStartsWith },
    { "Strings::EndsWith()", TestStringEndsWith },
    { "Strings::Replace() and ::ReplaceNew()", TestStringReplace },

    { "Toml basic tests",         TomlBasic      },
    { "Toml wrapping/unwrapping", TomlWrapping   },
    { "Toml <=> primitives",      TomlPrimitives },
    { "Toml <=> BETTER_ENUM",     TomlEnums      },
    { "Toml <=> GLM",             TomlGLM        },

    { "Bplus::GL::Buffers::Buffer basic",     BufferBasic   },
    { "Bplus::GL::Buffers::Buffer get/set data", BufferGetSetData },

    { "Bplus::GL::Texture creation", TextureCreation },
    { "Bplus::GL::Texture get/set data", TextureSimpleGetSetData },

    { "Bplus::GL::Target basic usage", TestTargetBasic },

    { "Shader #pragma include preprocessor", TestShaderIncludeCommand },*/

    //The below tests are interactive apps, so they're normally disabled to simplify testing.
    //{ "Simple Apps", SimpleApps },
    { "Basic Rendering App", BasicRenderApp },

    { NULL, NULL }
};