//Runs unit tests for BPlus.
//Uses the "acutest" single-header unit test library: https://github.com/mity/acutest

#include <acutest.h>

//AllTests.h is an auto-generated code file.
//It is generated as a pre-build step in Visual Studio,
//    so if you're getting an error about it not existing,
//    just hit "Compile" to fix.
#include "AllTests.h"


TEST_LIST = {
    { "Toml basic tests",         TomlBasic },
    { "Toml wrapping/unwrapping", TomlWrapping },
    { "Toml <=> primitives",      TomlPrimitives },
    { "Toml <=> BETTER_ENUM",     TomlEnums },
    { "Toml <=> GLM",             TomlGLM },

    { "Simple Apps", SimpleApps },

    //{ "Bplus::GL::Buffer basic tests", BuffersBasic },

    { NULL, NULL }
};