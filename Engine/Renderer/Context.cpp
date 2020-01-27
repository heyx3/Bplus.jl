#include "Context.h"

using namespace Bplus;
using Context = GL::Context;


namespace
{
    struct ContextThreadData
    {
        Context* Instance = nullptr;

        std::vector<std::function<void()>> RefreshCallbacks, DestroyCallbacks;
    };
    thread_local ContextThreadData contextThreadData = ContextThreadData();
}

Context* Context::GetCurrentContext() { return contextThreadData.Instance; }

Context::Context(SDL_Window* _owner, std::string& errMsg)
    : owner(_owner)
{
    if (contextThreadData.Instance != nullptr)
    {
        if (contextThreadData.Instance == this)
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
    contextThreadData.Instance = this;
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

    RefreshState();
}
Context::~Context()
{
    if (isInitialized)
    {
        SDL_GL_DeleteContext(sdlContext);

        BPAssert(contextThreadData.Instance == this,
                 "More than one initialized Context in this thread");
        contextThreadData.Instance = nullptr;
    }

    for (const auto& callback : contextThreadData.DestroyCallbacks)
        callback();
}


void Context::RefreshState()
{
    //A handful of features will be left enabled permanently for simplicity;
    //    they can still be effectively disabled via their specific parameters.
    glEnable(GL_BLEND);
    glEnable(GL_STENCIL_TEST);
    //Depth-testing is particularly important to keep on, because disabling it
    //    has a side effect of disabling any depth writes.
    glEnable(GL_DEPTH_TEST);

    EnableScissor = glIsEnabled(GL_SCISSOR_TEST);
    EnableDepthWrite = glIsEnabled(GL_DEPTH_WRITEMASK);
    Vsync = VsyncModes::_from_integral(SDL_GL_GetSwapInterval());
    glGetBooleanv(GL_COLOR_WRITEMASK, (GLboolean*)(&ColorWriteMask[0]));

    //Containers for various OpenGL settings.
    GLint tempI;
    glm::fvec4 tempV4;

    if (glIsEnabled(GL_CULL_FACE))
    {
        glGetIntegerv(GL_CULL_FACE_MODE, &tempI);
        CullMode = FaceCullModes::_from_integral(tempI);
    }
    else
    {
        CullMode = FaceCullModes::Off;
    }

    glGetIntegerv(GL_DEPTH_FUNC, &tempI);
    DepthTest = ValueTests::_from_integral(tempI);

    //Get color blending settings.
    glGetIntegerv(GL_BLEND_SRC_RGB, &tempI);
    ColorBlending.Src = BlendFactors::_from_integral(tempI);
    glGetIntegerv(GL_BLEND_DST_RGB, &tempI);
    ColorBlending.Dest = BlendFactors::_from_integral(tempI);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &tempI);
    ColorBlending.Op = BlendOps::_from_integral(tempI);

    //Get alpha blending settings.
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &tempI);
    AlphaBlending.Src = BlendFactors::_from_integral(tempI);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &tempI);
    AlphaBlending.Dest = BlendFactors::_from_integral(tempI);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &tempI);
    AlphaBlending.Op = BlendOps::_from_integral(tempI);

    //Get the blend constant.
    glGetFloatv(GL_BLEND_COLOR, &tempV4[0]);
    ColorBlending.Constant = { tempV4.r, tempV4.g, tempV4.b };
    AlphaBlending.Constant = glm::vec1(tempV4.a);

    //Get the stencil tests and write ops.
    for (int faceI = 0; faceI < 2; ++faceI)
    {
        StencilTest& testData = (faceI == 0) ? StencilTestFront : StencilTestBack;
        StencilResult& resultData = (faceI == 0) ? StencilResultFront : StencilResultBack;
        GLuint& writeMask = (faceI == 0) ? StencilMaskFront : StencilMaskBack;

        GLenum side = (faceI == 0) ? GL_FRONT : GL_BACK,
               key_test = (faceI == 0) ? GL_STENCIL_FUNC : GL_STENCIL_BACK_FUNC,
               key_ref = (faceI == 0) ? GL_STENCIL_REF : GL_STENCIL_BACK_REF,
               key_valueMask = (faceI == 0) ? GL_STENCIL_VALUE_MASK : GL_STENCIL_BACK_VALUE_MASK,
               key_onFail = (faceI == 0) ? GL_STENCIL_FAIL : GL_STENCIL_BACK_FAIL,
               key_onFailDepth = (faceI == 0) ? GL_STENCIL_PASS_DEPTH_FAIL : GL_STENCIL_BACK_PASS_DEPTH_FAIL,
               key_onPass = (faceI == 0) ? GL_STENCIL_PASS_DEPTH_PASS : GL_STENCIL_BACK_PASS_DEPTH_PASS,
               key_writeMask = (faceI == 0) ? GL_STENCIL_WRITEMASK : GL_STENCIL_BACK_WRITEMASK;

        glGetIntegerv(key_test, &tempI);
        testData.Test = ValueTests::_from_integral(tempI);
        glGetIntegerv(key_ref, &tempI);
        testData.RefValue = tempI;
        glGetIntegerv(key_valueMask, &tempI);
        testData.Mask = (GLuint)tempI;

        glGetIntegerv(key_onFail, &tempI);
        resultData.OnFailStencil = StencilOps::_from_integral(tempI);
        glGetIntegerv(key_onFailDepth, &tempI);
        resultData.OnPassStencilFailDepth = StencilOps::_from_integral(tempI);
        glGetIntegerv(key_onPass, &tempI);
        resultData.OnPassStencilDepth = StencilOps::_from_integral(tempI);

        glGetIntegerv(key_writeMask, &tempI);
        writeMask = (GLuint)tempI;
    }

    //Update other systems that want to refresh.
    for (auto& f : contextThreadData.RefreshCallbacks)
        f();
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
    if (!EnableScissor)
    {
        glEnable(GL_SCISSOR_TEST);
        EnableScissor = true;
    }

    glScissor(minX, minY, width, height);
}
void Context::DisableScissor()
{
    if (EnableScissor)
    {
        glDisable(GL_SCISSOR_TEST);
        EnableScissor = false;
    }
}

bool Context::SetVsyncMode(GL::VsyncModes mode)
{
    int err = SDL_GL_SetSwapInterval((int)mode);

    //If it failed, maybe the hardware just doesn't support G-sync/FreeSync.
    if (err != 0 && mode == +GL::VsyncModes::Adaptive)
        err = SDL_GL_SetSwapInterval((int)GL::VsyncModes::On);

    return err == 0;
}

void Context::SetFaceCulling(GL::FaceCullModes mode)
{
    if (mode == +FaceCullModes::Off)
    {
        if (CullMode != +FaceCullModes::Off)
        {
            glDisable(GL_CULL_FACE);
            CullMode = +FaceCullModes::Off;
        }
    }
    else
    {
        if (CullMode == +FaceCullModes::Off)
            glEnable(GL_CULL_FACE);

        if (CullMode != mode)
        {
            CullMode = mode;
            glCullFace((GLenum)mode);
        }
    }
}

void Context::SetDepthTest(GL::ValueTests newTest)
{
    //If we haven't initialized depth-testing yet, turn it on permanently.
    //Disabling depth-testing also disables depth writes,
    //    but we expose a separate mechanism for handling that.
    if ((GLenum)DepthTest == GL_INVALID_ENUM)
        glEnable(GL_DEPTH_TEST);
    
    if (DepthTest != newTest)
    {
        glDepthFunc((GLenum)newTest);
        DepthTest = newTest;
    }
}

void Context::SetDepthWrites(bool canWriteDepth)
{
    if (canWriteDepth != EnableDepthWrite)
    {
        EnableDepthWrite = canWriteDepth;
        if (EnableDepthWrite)
            glEnable(GL_DEPTH_WRITEMASK);
        else
            glDisable(GL_DEPTH_WRITEMASK);
    }
}

void Context::SetColorWriteMask(glm::bvec4 canWrite)
{
    if (canWrite == ColorWriteMask)
        return;

    ColorWriteMask = canWrite;
    glColorMask(canWrite.r, canWrite.g, canWrite.b, canWrite.a);
}


GL::BlendStateRGBA Context::GetBlending() const
{
    //Make sure the same blend settings are being used for both RGB and Alpha.
    BlendStateAlpha colorBlendTest{ ColorBlending.Src, ColorBlending.Dest,
                                    ColorBlending.Op,
                                    AlphaBlending.Constant };
    BPAssert(AlphaBlending == colorBlendTest,
             "Alpha blend state and color blend state do not match up");

    return BlendStateRGBA{ ColorBlending.Src, ColorBlending.Dest,
                           ColorBlending.Op,
                           { ColorBlending.Constant, AlphaBlending.Constant } };
}

void Context::SetBlending(const GL::BlendStateRGBA& state)
{
    //Don't waste time in the GPU driver if we're already in this blend state.
    BlendStateRGB newColorBlending{ state.Src, state.Dest, state.Op,
                                    { state.Constant.r, state.Constant.g, state.Constant.b } };
    BlendStateAlpha newAlphaBlending{ state.Src, state.Dest, state.Op,
                                      glm::vec1(state.Constant.a) };
    if ((newColorBlending == ColorBlending) &
        (newAlphaBlending == AlphaBlending))
    {
        return;
    }

    ColorBlending = newColorBlending;
    AlphaBlending = newAlphaBlending;

    glBlendFunc((GLenum)state.Src, (GLenum)state.Dest);
    glBlendEquation((GLenum)state.Op);
    glBlendColor(state.Constant.r, state.Constant.g, state.Constant.b, state.Constant.a);
}
void Context::SetColorBlending(const GL::BlendStateRGB& state)
{
    if (state == ColorBlending)
        return;

    ColorBlending = state;
    glBlendFuncSeparate((GLenum)ColorBlending.Src, (GLenum)ColorBlending.Dest,
                        (GLenum)AlphaBlending.Src, (GLenum)AlphaBlending.Dest);
    glBlendEquationSeparate((GLenum)ColorBlending.Op,
                            (GLenum)AlphaBlending.Op);
    glBlendColor(ColorBlending.Constant.r,
                 ColorBlending.Constant.g,
                 ColorBlending.Constant.b,
                 AlphaBlending.Constant.x);
}
void Context::SetAlphaBlending(const GL::BlendStateAlpha& state)
{
    if (state == AlphaBlending)
        return;

    AlphaBlending = state;
    glBlendFuncSeparate((GLenum)ColorBlending.Src, (GLenum)ColorBlending.Dest,
                        (GLenum)AlphaBlending.Src, (GLenum)AlphaBlending.Dest);
    glBlendEquationSeparate((GLenum)ColorBlending.Op,
                            (GLenum)AlphaBlending.Op);
    glBlendColor(ColorBlending.Constant.r,
                 ColorBlending.Constant.g,
                 ColorBlending.Constant.b,
                 AlphaBlending.Constant.x);
}


const GL::StencilTest& Context::GetStencilTest() const
{
    //Make sure the same settings are being used for both front- and back-faces.
    BPAssert(StencilTestFront == StencilTestBack,
             "Front-face stencil test and back-face stencil test don't match");
    return StencilTestFront;
}

void Context::SetStencilTest(const GL::StencilTest& test)
{
    if ((StencilTestFront == test) & (StencilTestBack == test))
        return;

    StencilTestFront = test;
    StencilTestBack = test;

    glStencilFunc((GLenum)test.Test, test.RefValue, test.Mask);
}
void Context::SetStencilTestFrontFaces(const GL::StencilTest& test)
{
    if (test == StencilTestFront)
        return;

    StencilTestFront = test;
    glStencilFuncSeparate(GL_FRONT, (GLenum)test.Test, test.RefValue, test.Mask);
}
void Context::SetStencilTestBackFaces(const GL::StencilTest& test)
{
    if (test == StencilTestBack)
        return;

    StencilTestBack = test;
    glStencilFuncSeparate(GL_BACK, (GLenum)test.Test, test.RefValue, test.Mask);
}


const GL::StencilResult& Context::GetStencilResult() const
{
    //Make sure the same settings are being used for both front- and back-faces.
    BPAssert(StencilResultFront == StencilResultBack,
             "Front-face stencil result and back-face stencil result don't match");
    return StencilResultFront;
}

void Context::SetStencilResult(const GL::StencilResult& result)
{
    if ((StencilResultFront == result) & (StencilResultBack == result))
        return;

    StencilResultFront = result;
    StencilResultBack = result;
    glStencilOp((GLenum)result.OnFailStencil,
                (GLenum)result.OnPassStencilFailDepth,
                (GLenum)result.OnPassStencilDepth);
}
void Context::SetStencilResultFrontFaces(const GL::StencilResult& result)
{
    if (result == StencilResultFront)
        return;

    StencilResultFront = result;
    glStencilOpSeparate(GL_FRONT,
                        (GLenum)result.OnFailStencil,
                        (GLenum)result.OnPassStencilFailDepth,
                        (GLenum)result.OnPassStencilDepth);
}
void Context::SetStencilResultBackFaces(const GL::StencilResult& result)
{
    if (result == StencilResultBack)
        return;

    StencilResultBack = result;
    glStencilOpSeparate(GL_BACK,
                        (GLenum)result.OnFailStencil,
                        (GLenum)result.OnPassStencilFailDepth,
                        (GLenum)result.OnPassStencilDepth);
}


GLuint Context::GetStencilMask() const
{
    //Make sure the same settings are being used for both front- and back-faces.
    BPAssert(StencilMaskFront == StencilMaskBack,
             "Front-face stencil mask and back-face stencil mask don't match up");
    return StencilMaskFront;
}

void Context::SetStencilMask(GLuint mask)
{
    if ((StencilMaskFront == mask) & (StencilMaskBack == mask))
        return;

    StencilMaskFront = mask;
    StencilMaskBack = mask;
    glStencilMask(mask);
}
void Context::SetStencilMaskFrontFaces(GLuint mask)
{
    if (mask == StencilMaskFront)
        return;

    StencilMaskFront = mask;
    glStencilMaskSeparate(GL_FRONT, mask);
}
void Context::SetStencilMaskBackFaces(GLuint mask)
{
    if (mask == StencilMaskBack)
        return;

    StencilMaskBack = mask;
    glStencilMaskSeparate(GL_BACK, mask);
}