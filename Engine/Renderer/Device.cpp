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
    //If there is no context, make sure there is no Device either.
    if (Context::GetCurrentContext() == nullptr)
    {
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
    GLint tempI;

    //Get 'Max Textures in Shader'
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &tempI);
    BPAssert(tempI >= 0, "'Max Textures in Shader' is negative??");
    BPAssert(tempI <= std::numeric_limits<decltype(maxTexturesInShader)>().max(),
             "'Max Textures in Shader' is larger than its type can fit");
    maxTexturesInShader = (decltype(maxTexturesInShader))tempI;

    //When the context is destroyed, destroy this Device.
    Context::RegisterCallback_Destroyed([]()
    {
        threadData.Device.reset();
    });
}