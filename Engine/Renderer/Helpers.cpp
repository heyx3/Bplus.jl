#include "Helpers.h"

#include "../RenderLibs.h"

using namespace Bplus;


bool GL::TrySDL(int returnCode, std::string& errOut, const char* prefix)
{
    if (returnCode != 0)
        errOut == std::string(prefix) + ": " + SDL_GetError();
    return returnCode == 0;
}

bool GL::TrySDL(void* shouldntBeNull, std::string& errOut, const char* prefix)
{
    if (shouldntBeNull == nullptr)
        return TrySDL(-1, errOut, prefix);
    else
        return true;
}

bool GL::UsesConstant(BlendFactors b)
{
    switch (b)
    {
        case BlendFactors::ConstantColor:
        case BlendFactors::ConstantAlpha:
        case BlendFactors::InverseConstantColor:
        case BlendFactors::InverseConstantAlpha:
            return true;

        default: return false;
    }
}


//TODO: Implement all the string shit.