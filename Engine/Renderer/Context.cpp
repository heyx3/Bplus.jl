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
    //Keep depth-testing on permanently.
    //You can still disable it via DepthTests::Off,
    //    and using glDisable() on this would have the side effect of disabling depth writes.
    //We already expose a separate mechanism for disabling those.
    glEnable(GL_DEPTH_TEST);

    //To keep things similarly simple for blending, keep blending on permanently
    //    and just use an Opaque blend state to effectively turn it off.
    glEnable(GL_BLEND);

    isScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    isDepthWriteEnabled = glIsEnabled(GL_DEPTH_WRITEMASK);
    currentVsync = (VsyncModes)SDL_GL_GetSwapInterval();

    //Containers for various OpenGL settings.
    int tempI;
    glm::fvec4 tempV4;

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

    //Get color blending settings.
    glGetIntegerv(GL_BLEND_SRC_RGB, &tempI);
    currentColorBlending.Src = (BlendFactors)tempI;
    glGetIntegerv(GL_BLEND_DST_RGB, &tempI);
    currentColorBlending.Dest = (BlendFactors)tempI;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &tempI);
    currentColorBlending.Op = (BlendOps)tempI;

    //Get alpha blending settings.
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &tempI);
    currentAlphaBlending.Src = (BlendFactors)tempI;
    glGetIntegerv(GL_BLEND_DST_ALPHA, &tempI);
    currentAlphaBlending.Dest = (BlendFactors)tempI;
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &tempI);
    currentAlphaBlending.Op = (BlendOps)tempI;

    //Get the blend constant.
    glGetFloatv(GL_BLEND_COLOR, &tempV4[0]);
    currentColorBlending.Constant = { tempV4.r, tempV4.g, tempV4.b };
    currentAlphaBlending.Constant = tempV4.a;
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

void Context::SetBlending(const BlendStateRGBA& state)
{
    //Don't waste time in the GPU driver if we're already in this blend state.
    BlendStateRGB newColorBlending{ state.Src, state.Dest, state.Op,
                                    { state.Constant.r, state.Constant.g, state.Constant.b } };
    BlendStateAlpha newAlphaBlending{ state.Src, state.Dest, state.Op, state.Constant.a };
    if ((newColorBlending == currentColorBlending) &
        (newAlphaBlending == currentAlphaBlending))
    {
        return;
    }

    currentColorBlending = newColorBlending;
    currentAlphaBlending = newAlphaBlending;

    glBlendFunc((GLenum)state.Src, (GLenum)state.Dest);
    glBlendEquation((GLenum)state.Op);
    glBlendColor(state.Constant.r, state.Constant.g, state.Constant.b, state.Constant.a);
}
void Context::SetColorBlending(const BlendStateRGB& state)
{
    if (state == currentColorBlending)
        return;

    currentColorBlending = state;
    glBlendFuncSeparate((GLenum)currentColorBlending.Src, (GLenum)currentColorBlending.Dest,
                        (GLenum)currentAlphaBlending.Src, (GLenum)currentAlphaBlending.Dest);
    glBlendEquationSeparate((GLenum)currentColorBlending.Op,
                            (GLenum)currentAlphaBlending.Op);
    glBlendColor(currentColorBlending.Constant.r,
                 currentColorBlending.Constant.g,
                 currentColorBlending.Constant.b,
                 currentAlphaBlending.Constant);
}
void Context::SetAlphaBlending(const BlendStateAlpha& state)
{
    if (state == currentAlphaBlending)
        return;

    currentAlphaBlending = state;
    glBlendFuncSeparate((GLenum)currentColorBlending.Src, (GLenum)currentColorBlending.Dest,
                        (GLenum)currentAlphaBlending.Src, (GLenum)currentAlphaBlending.Dest);
    glBlendEquationSeparate((GLenum)currentColorBlending.Op,
                            (GLenum)currentAlphaBlending.Op);
    glBlendColor(currentColorBlending.Constant.r,
                 currentColorBlending.Constant.g,
                 currentColorBlending.Constant.b,
                 currentAlphaBlending.Constant);
}