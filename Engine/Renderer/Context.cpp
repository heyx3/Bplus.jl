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
    //A handful of features will be left enabled permanently for simplicity;
    //    they can still be effectively disabled via their specific parameters.
    glEnable(GL_BLEND);
    glEnable(GL_STENCIL_TEST);
    //Depth-testing is particularly important to keep on, because disabling it
    //    has a side effect of disabling any depth writes.
    glEnable(GL_DEPTH_TEST);

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
    currentDepthTest = (ValueTests)tempI;

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

    //Get the stencil tests and write ops.
    for (int faceI = 0; faceI < 2; ++faceI)
    {
        StencilTest& testData = (faceI == 0) ? stencilTestFront : stencilTestBack;
        StencilResult& resultData = (faceI == 0) ? stencilResultFront : stencilResultBack;

        GLenum side = (faceI == 0) ? GL_FRONT : GL_BACK,
               key_test = (faceI == 0) ? GL_STENCIL_FUNC : GL_STENCIL_BACK_FUNC,
               key_ref = (faceI == 0) ? GL_STENCIL_REF : GL_STENCIL_BACK_REF,
               key_valueMask = (faceI == 0) ? GL_STENCIL_VALUE_MASK : GL_STENCIL_BACK_VALUE_MASK,
               key_onFail = (faceI == 0) ? GL_STENCIL_FAIL : GL_STENCIL_BACK_FAIL,
               key_onFailDepth = (faceI == 0) ? GL_STENCIL_PASS_DEPTH_FAIL : GL_STENCIL_BACK_PASS_DEPTH_FAIL,
               key_onPass = (faceI == 0) ? GL_STENCIL_PASS_DEPTH_PASS : GL_STENCIL_BACK_PASS_DEPTH_PASS,
               key_writeMask = (faceI == 0) ? GL_STENCIL_WRITEMASK : GL_STENCIL_BACK_WRITEMASK;

        glGetIntegerv(key_test, &tempI);
        testData.Test = (ValueTests)tempI;
        glGetIntegerv(key_ref, &tempI);
        testData.RefValue = tempI;
        static_assert(false, "How to get stencil test mask fron int -> uint?");

        glGetIntegerv(key_onFail, &tempI);
        resultData.OnFailStencil = (StencilOps)tempI;
        glGetIntegerv(key_onFailDepth, &tempI);
        resultData.OnPassStencilFailDepth = (StencilOps)tempI;
        glGetIntegerv(key_onPass, &tempI);
        resultData.OnPassStencilDepth = (StencilOps)tempI;

        static_assert(false, "How to get stencil write mask fron int -> uint?");
    }
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

void Context::SetDepthTest(GL::ValueTests newTest)
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


GL::BlendStateRGBA Context::GetBlending() const
{
    //Make sure the same blend settings are being used for both RGB and Alpha.
    BlendStateAlpha colorBlendTest{ currentColorBlending.Src, currentColorBlending.Dest,
                                    currentColorBlending.Op,
                                    currentAlphaBlending.Constant };
    assert(currentAlphaBlending == colorBlendTest);

    return BlendStateRGBA{ currentColorBlending.Src, currentColorBlending.Dest,
                           currentColorBlending.Op,
                           { currentColorBlending.Constant, currentAlphaBlending.Constant } };
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


const GL::StencilTest& Context::GetStencilTest() const
{
    //Make sure the same settings are being used for both front- and back-faces.
    assert(stencilTestFront == stencilTestBack);
    return stencilTestFront;
}

void Context::SetStencilTest(const StencilTest& test)
{
    if ((stencilTestFront == test) & (stencilTestBack == test))
        return;

    stencilTestFront = test;
    stencilTestBack = test;

    glStencilFunc((GLenum)test.Test, test.RefValue, test.Mask);
}
void Context::SetStencilTestFrontFaces(const StencilTest& test)
{
    if (test == stencilTestFront)
        return;

    stencilTestFront = test;
    glStencilFuncSeparate(GL_FRONT, (GLenum)test.Test, test.RefValue, test.Mask);
}
void Context::SetStencilTestBackFaces(const StencilTest& test)
{
    if (test == stencilTestBack)
        return;

    stencilTestBack = test;
    glStencilFuncSeparate(GL_BACK, (GLenum)test.Test, test.RefValue, test.Mask);
}


const GL::StencilResult& Context::GetStencilResult() const
{
    //Make sure the same settings are being used for both front- and back-faces.
    assert(stencilResultFront == stencilResultBack);
    return stencilResultFront;
}

void Context::SetStencilResult(const StencilResult& result)
{
    if ((stencilResultFront == result) & (stencilResultBack == result))
        return;

    stencilResultFront = result;
    stencilResultBack = result;
    glStencilOp((GLenum)result.OnFailStencil,
                (GLenum)result.OnPassStencilFailDepth,
                (GLenum)result.OnPassStencilDepth);
}
void Context::SetStencilResultFrontFaces(const StencilResult& result)
{
    if (result == stencilResultFront)
        return;

    stencilResultFront = result;
    glStencilOpSeparate(GL_FRONT,
                        (GLenum)result.OnFailStencil,
                        (GLenum)result.OnPassStencilFailDepth,
                        (GLenum)result.OnPassStencilDepth);
}
void Context::SetStencilResultBackFaces(const StencilResult& result)
{
    if (result == stencilResultBack)
        return;

    stencilResultBack = result;
    glStencilOpSeparate(GL_BACK,
                        (GLenum)result.OnFailStencil,
                        (GLenum)result.OnPassStencilFailDepth,
                        (GLenum)result.OnPassStencilDepth);
}


GLuint Context::GetStencilMask() const
{
    //Make sure the same settings are being used for both front- and back-faces.
    assert(stencilMaskFront == stencilMaskBack);
    return stencilMaskFront;
}

void Context::SetStencilMask(GLuint mask)
{
    if ((stencilMaskFront == mask) & (stencilMaskBack == mask))
        return;

    stencilMaskFront = mask;
    stencilMaskBack = mask;
    glStencilMask(mask);
}
void Context::SetStencilMaskFrontFaces(GLuint mask)
{
    if (mask == stencilMaskFront)
        return;

    stencilMaskFront = mask;
    glStencilMaskSeparate(GL_FRONT, mask);
}
void Context::SetStencilMaskBackFaces(GLuint mask)
{
    if (mask == stencilMaskBack)
        return;

    stencilMaskBack = mask;
    glStencilMaskSeparate(GL_BACK, mask);
}