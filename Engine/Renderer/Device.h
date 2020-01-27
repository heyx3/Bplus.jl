#pragma once

#include "../Utils.h"


namespace Bplus::GL
{
    class BP_API Context;

    //Information about the specific hardware OpenGL is running on
    //   (more specifically, that device's limits).
    class BP_API Device
    {
    public:

        //Gets the device data for the OpenGL context on this thread,
        //    or nullptr if an OpenGL context doesn't exist on this thread.
        static Device* GetContextDevice();


        //Gets the maximum number of different textures that can be given to a shader,
        //    including both actual Textures and Images.
        uint16_t GetMaxTexturesInShader() const { return maxTexturesInShader; }


    private:

        Device(Context& context);

        uint16_t maxTexturesInShader;
    };
}