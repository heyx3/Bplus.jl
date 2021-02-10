#include "ImGuiInterfaces.h"

#include <vector>

using namespace Bplus;

namespace
{
    thread_local ImGuiSDLInterface* currentSDLInterface = nullptr;
    thread_local ImGuiOpenGLInterface* currentOGLInterface = nullptr;
}

ImGuiSDLInterface* ImGuiSDLInterface::GetThreadInstance()
{
    return currentSDLInterface;
}
ImGuiSDLInterface::ImGuiSDLInterface(SDL_Window* _mainWindow, SDL_GLContext _glContext)
    : mainWindow(_mainWindow), glContext(_glContext)
{
    IM_ASSERT(currentSDLInterface == nullptr);
    currentSDLInterface = this;
}
ImGuiSDLInterface::~ImGuiSDLInterface()
{
    IM_ASSERT(currentSDLInterface == this);
    currentSDLInterface = nullptr;
}

ImGuiOpenGLInterface* ImGuiOpenGLInterface::GetThreadInstance()
{
    return currentOGLInterface;
}
ImGuiOpenGLInterface::ImGuiOpenGLInterface(const char* _glslVersion)
    : glslVersion(_glslVersion)
{
    IM_ASSERT(currentOGLInterface == nullptr);
    currentOGLInterface = this;
}
ImGuiOpenGLInterface::~ImGuiOpenGLInterface()
{
    IM_ASSERT(currentOGLInterface == this);
    currentOGLInterface = nullptr;
}