#include "Data.h"

using namespace Bplus;


bool GL::TrySDL(int returnCode, std::string& errOut, const char* prefix)
{
    if (returnCode != 0)
        errOut = std::string(prefix) + ": " + SDL_GetError();
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

void GL::StencilTest::FromToml(const toml::Value& tomlData)
{
    const char* status = "[null]";
    try
    {
        status = "Test";
        Test = IO::EnumFromString<ValueTests>("Test");

        status = "RefValue";
        RefValue = tomlData.as<GLint>();

        status = "Mask";
        Mask = tomlData.as<GLuint>();
    }
    catch (const IO::Exception& e)
    {
        throw IO::Exception(e, std::string("Error parsing StencilTest::") + status + ": ");
    }
}
toml::Value GL::StencilTest::ToToml() const
{
    toml::Value tomlData;
    tomlData["Test"] = Test._to_string();
    tomlData["RefValue"] = RefValue;

    BPAssert((int64_t)Mask == Mask,
             "Unable to serialize Mask; need to add native uint support to tinyTOML");
    tomlData["Mask"] = (int64_t)Mask;
    return tomlData;
}
bool GL::StencilTest::EditGUI(int popupMaxItemHeight)
{
    bool changed = false;

    changed |= ImGui::EnumCombo<ValueTests>("Test", Test, popupMaxItemHeight);
    changed |= ImGui::InputInt("Ref Value", &RefValue, 1, 10,
                               ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsDecimal);

    //TODO: What happens with larger mask values?
    int maskI = (int)Mask;
    changed |= ImGui::InputInt("Ref Value", &maskI, 1, 256,
                               ImGuiInputTextFlags_CharsHexadecimal);
    Mask = (GLuint)maskI;

    return changed;
}

void GL::StencilResult::FromToml(const toml::Value& tomlData)
{
    const char* status = "[null]";
    try
    {
        status = "OnFailStencil";
        OnFailStencil = IO::EnumFromString<StencilOps>(status);

        status = "OnPassStencilFailDepth";
        OnPassStencilFailDepth = IO::EnumFromString<StencilOps>(status);

        status = "OnPassStencilDepth";
        OnPassStencilDepth = IO::EnumFromString<StencilOps>(status);
    }
    catch (const IO::Exception& e)
    {
        throw IO::Exception(e, std::string("Error parsing StencilResult::") + status + ": ");
    }
}
toml::Value GL::StencilResult::ToToml() const
{
    toml::Value tomlData;
    tomlData["OnFailStencil"] = OnFailStencil._to_string();
    tomlData["OnPassStencilFailDepth"] = OnPassStencilFailDepth._to_string();
    tomlData["OnPassStencilDepth"] = OnPassStencilDepth._to_string();
    return tomlData;
}
bool GL::StencilResult::EditGUI(int popupMaxItemHeight)
{
    return ImGui::EnumCombo<StencilOps>("Failed Stencil", OnFailStencil,
                                        popupMaxItemHeight) |
           ImGui::EnumCombo<StencilOps>("Passed Stencil, failed Depth", OnPassStencilFailDepth,
                                        popupMaxItemHeight) |
           ImGui::EnumCombo<StencilOps>("Passed Stencil and Depth", OnPassStencilDepth,
                                        popupMaxItemHeight);
}