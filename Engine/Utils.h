#pragma once

#include "Platform.h"

#include <vector>
#include <cstddef>
#include <functional>


//TODO: Make these files ".inl" and simplify their includes.
#include "Utils/Functions.h"
#include "Utils/Hashing.h"
#include "Utils/StrongTypedef.h"
#include "Utils/Lazy.h"
#include "Utils/Bool.h"
#include "Utils/BPAssert.h"


//The BETTER_ENUM() macro, to define an enum
//    with added string conversions and iteration.
//There is one downside: to use a literal value, it must be prepended with a '+',
//    e.x. "if (mode == +VsyncModes::Off)".
//#define BETTER_ENUMS_API BP_API
//NOTE: The #define above was removed because it screws up usage from outside this library,
//    and I'm pretty sure it's not even needed in the first place.
//
//Before including the file, I'm adding a custom modification to enable
//    default constructors for these enum values:
#define BETTER_ENUMS_DEFAULT_CONSTRUCTOR(Enum) public: Enum() = default;
//
#include <better_enums.h>


//The name of the folder where all engine and app content goes.
//This folder is automatically copied to the output directory when building the engine.
#define BPLUS_CONTENT_FOLDER "content"

//The relative path to the engine's own content folder.
//Apps should not put their own stuff in this folder.
#define BPLUS_ENGINE_CONTENT_FOLDER (BPLUS_CONTENT_FOLDER "/engine")