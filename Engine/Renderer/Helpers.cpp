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

bool GL::operator==(const GL::StencilTest& a, const GL::StencilTest& b)
{
    return (a.Test == b.Test) &
           (a.RefValue == b.RefValue) &
           (a.Mask == b.Mask);
}
bool GL::operator==(const GL::StencilResult& a, const GL::StencilResult& b)
{
    return (a.OnFailStencil == b.OnFailStencil) &
           (a.OnPassStencilFailDepth == b.OnPassStencilFailDepth) &
           (a.OnPassStencilDepth == b.OnPassStencilDepth);
}

void GL::StencilTest::Read(const toml::Value& tomlData)
{
    const char* status = "[null]";
    try
    {
        status = "Test";
        Test = IO::EnumFromString<ValueTests>(tomlData, "Test");

        status = "RefValue";
        RefValue = IO::TomlGet<GLint>(tomlData, "RefValue");

        status = "Mask";
        Mask = IO::FromTomlInt(IO::TomlGet<int64_t>(tomlData, "Mask"));
    }
    catch (const IO::Exception& e)
    {
        throw IO::Exception(e, std::string("Error parsing StencilTest::") + status + ": ");
    }
}
void GL::StencilTest::Write(toml::Value& tomlData) const
{
    tomlData["Test"] = Test._name();
    tomlData["RefValue"] = RefValue;
    tomlData["Mask"] = IO::ToTomlInt(Mask);
}

void GL::StencilResult::Read(const toml::Value& tomlData)
{
    const char* status = "[null]";
    try
    {
        status = "OnFailStencil";
        OnFailStencil = IO::EnumFromString<StencilOps>(tomlData, status);

        status = "OnPassStencilFailDepth";
        OnPassStencilFailDepth = IO::EnumFromString<StencilOps>(tomlData, status);

        status = "OnPassStencilDepth";
        OnPassStencilDepth = IO::EnumFromString<StencilOps>(tomlData, status);
    }
    catch (const IO::Exception& e)
    {
        throw IO::Exception(e, std::string("Error parsing StencilResult::") + status + ": ");
    }
}
void GL::StencilResult::Write(toml::Value& tomlData) const
{
    tomlData["OnFailStencil"] = OnFailStencil._name();
    tomlData["OnPassStencilFailDepth"] = OnPassStencilFailDepth._name();
    tomlData["OnPassStencilDepth"] = OnPassStencilDepth._name();
}