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

        //Gets the maximum number of render targets that can be output to at once.
        //Guaranteed by OpenGL to be at least 8.
        uint16_t GetMaxOutputRenderTargets() const { return maxOutputRenderTargets; }

        //Gets the driver-recommended maximum number of vertices for a single buffer.
        //Having more than this in a single buffer can result in a significant performance hit.
        uint32_t GetSoftMaxMeshVertices() const { return softMaxVertices; }
        //Gets the driver-recommended maximum number of indices for a single buffer.
        //Having more than this in a single buffer can result in a significant performance hit.
        uint32_t GetSoftMaxMeshIndices() const { return softMaxIndices; }

        //The maximum number of individual float/int/bool uniform values
        //    that can exist in a vertex shader.
        //Guaranteed by OpenGL to be at least 1024.
        uint32_t GetMaxUniformPrimitivesPerVertexShader() const { return maxUniformPrimitivesPerVertexShader; }
        //The maximum number of individual float/int/bool uniform values
        //    that can exist in a fragment shader.
        //Guaranteed by OpenGL to be at least 1024.
        uint32_t GetMaxUniformPrimitivesPerFragmentShader() const { return maxUniformPrimitivesPerFragmentShader; }


    private:

        Device(Context& context);

        uint16_t maxTexturesInShader, maxOutputRenderTargets;
        uint32_t softMaxVertices, softMaxIndices,
                 maxUniformPrimitivesPerVertexShader, maxUniformPrimitivesPerFragmentShader;
    };
}