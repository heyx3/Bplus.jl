#include "Device.h"

#include "Context.h"

using namespace Bplus;
using namespace Bplus::GL;

namespace
{
    thread_local struct
    {
        std::unique_ptr<Device> Device;

    } threadData;
}


Device* Device::GetContextDevice()
{
    if (Context::GetCurrentContext() == nullptr)
    {
        //If there is no context, make sure there is no Device either.
        BPAssert(threadData.Device == nullptr,
                 "There is a device despite there being no context!");
    }
    //If there IS a context, but there is no Device yet, create it.
    else if (threadData.Device == nullptr)
    {
        threadData.Device.reset(new Device(*Context::GetCurrentContext()));
    }

    return threadData.Device.get();
}

Device::Device(Context& context)
{
    BPAssert(Context::GetCurrentContext() != nullptr,
             "Device created before context!");

    GLint tempI;

    #define LOAD_UINT(oglEnum, description, var) \
        glGetIntegerv(oglEnum, &tempI); \
        BPAssert(tempI >= 0, "'" description "' is negative??"); \
        BPAssert((decltype(var))tempI <= std::numeric_limits<decltype(var)>().max(), \
                 "'" description "' is larger than its type can fit"); \
        var = (decltype(var))tempI

    LOAD_UINT(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
              "Max Textures in Shader",
              maxTexturesInShader);
    LOAD_UINT(GL_MAX_COLOR_ATTACHMENTS,
              "Max Color Attachments per Target",
              maxColorTargets);
    LOAD_UINT(GL_MAX_ELEMENTS_VERTICES,
              "Soft Max Vertices in Buffer",
              softMaxVertices);
    LOAD_UINT(GL_MAX_ELEMENTS_INDICES,
              "Soft Max Indices in Buffer",
              softMaxIndices);
    LOAD_UINT(GL_MAX_VERTEX_UNIFORM_COMPONENTS,
              "Max Uniform Components in Vertex Shader",
              maxUniformPrimitivesPerVertexShader);
    LOAD_UINT(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
              "Max Uniform Components in Fragment Shader",
              maxUniformPrimitivesPerFragmentShader);

    //When the context is destroyed, destroy this Device.
    Context::RegisterCallback_Destroyed([]()
    {
        threadData.Device.reset();
    });
}