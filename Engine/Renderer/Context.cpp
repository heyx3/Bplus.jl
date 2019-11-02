#include "Context.h"

using namespace Bplus;
using Context = GL::Context;


namespace
{
    thread_local Context* contextInstance = nullptr;
}

Context* Context::GetCurrentContext() { return contextInstance; }

Context::Context(SDL_Window* _owner, std::string& errMsg)
    : owner(_owner)
{
    if (contextInstance != nullptr)
    {
        if (contextInstance == this)
            errMsg = "This context has already been constructed!";
        else
            errMsg = "A context already exists on this thread \
that hasn't been cleaned up.";

        return;
    }

    //Configure/create the OpenGL context.
    if (!TrySDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,
        GLVersion_Major()),
        errMsg, "Error setting OpenGL context major") ||
        !TrySDL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,
            GLVersion_Minor()),
            errMsg, "Error setting OpenGL context minor") ||
        !TrySDL(sdlContext = SDL_GL_CreateContext(owner),
            errMsg, "Error initializing OpenGL context"))
    {
        return;
    }

    //We started OpenGL successfully!
    contextInstance = this;
    isInitialized = true;

    //Initialize GLEW.
    glewExperimental = GL_TRUE;
    auto glewError = glewInit();
    if (glewError != GLEW_OK)
    {
        errMsg = std::string("Error setting up GLEW: ") +
            (const char*)glewGetErrorString(glewError);
        return;
    }

    //Keep depth-testing on permanently.
    //You can still disable it via DepthTests::Off,
    //    and glDisable()-ing it would have the side effect of disabling depth writes;
    //    we already expose a separate mechanism for disabling those.
    glEnable(GL_DEPTH_TEST);

    RefreshDriverState();
}
Context::~Context()
{
    if (isInitialized)
    {
        SDL_GL_DeleteContext(sdlContext);

        assert(contextInstance == this);
        contextInstance = nullptr;
    }
}


void Context::RefreshDriverState()
{
    isScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    isDepthWriteEnabled = glIsEnabled(GL_DEPTH_WRITEMASK);
    currentVsync = (VsyncModes)SDL_GL_GetSwapInterval();

    int tempI;

    if (glIsEnabled(GL_CULL_FACE))
    {
        glGetIntegerv(GL_CULL_FACE_MODE, &tempI);
        currentCullMode = (FaceCullModes)tempI;
    }
    else
    {
        currentCullMode = FaceCullModes::Off;
    }

    glGetIntegerv(GL_DEPTH_FUNC, &tempI);
    currentDepthTest = (DepthTests)tempI;
}


void Context::Clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}
void Context::Clear(float depth)
{
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Context::Clear(float r, float g, float b, float a, float depth)
{
    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void Context::SetViewport(int minX, int minY, int width, int height)
{
    glViewport(minX, minY, width, height);
}


void Context::SetScissor(int minX, int minY, int width, int height)
{
    if (!isScissorEnabled)
    {
        glEnable(GL_SCISSOR_TEST);
        isScissorEnabled = true;
    }

    glScissor(minX, minY, width, height);
}
void Context::DisableScissor()
{
    if (isScissorEnabled)
    {
        glDisable(GL_SCISSOR_TEST);
        isScissorEnabled = false;
    }
}

bool Context::SetVsyncMode(GL::VsyncModes mode)
{
    int err = SDL_GL_SetSwapInterval((int)mode);

    //If it failed, maybe the hardware just doesn't support G-sync/FreeSync.
    if (err != 0 && mode == GL::VsyncModes::Adaptive)
        err = SDL_GL_SetSwapInterval((int)GL::VsyncModes::On);

    return err == 0;
}

void Context::SetFaceCulling(GL::FaceCullModes mode)
{
    if (mode == FaceCullModes::Off)
    {
        if (currentCullMode != FaceCullModes::Off)
        {
            glDisable(GL_CULL_FACE);
            currentCullMode = FaceCullModes::Off;
        }
    }
    else
    {
        if (currentCullMode == FaceCullModes::Off)
            glEnable(GL_CULL_FACE);

        if (currentCullMode != mode)
        {
            currentCullMode = mode;
            glCullFace((GLenum)mode);
        }
    }
}

void Context::SetDepthTest(GL::DepthTests newTest)
{
    //If we haven't initialized depth-testing yet, turn it on permanently.
    //Disabling depth-testing also disables depth writes,
    //    but we expose a separate mechanism for handling that.
    if ((GLenum)currentDepthTest == GL_INVALID_ENUM)
        glEnable(GL_DEPTH_TEST);
    
    if (currentDepthTest != newTest)
    {
        glDepthFunc((GLenum)newTest);
        currentDepthTest = newTest;
    }
}

void Context::SetDepthWrites(bool canWriteDepth)
{
    if (canWriteDepth != isDepthWriteEnabled)
    {
        isDepthWriteEnabled = canWriteDepth;
        if (isDepthWriteEnabled)
            glEnable(GL_DEPTH_WRITEMASK);
        else
            glDisable(GL_DEPTH_WRITEMASK);
    }
}