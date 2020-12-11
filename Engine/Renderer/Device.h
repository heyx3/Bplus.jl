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


        //Gets the maximum number of color textures that a Target can have attached to it.
        //Guaranteed by OpenGL to be at least 8.
        uint32_t GetMaxTargetColorAttachments() const { return maxColorAttachments; }
        //Gets the maximum number of color textures that a shader can output to at once.
        uint32_t GetMaxTargetColorOutputs() const { return maxColorOutputs; }


        //Gets the driver-recommended maximum number of vertices for a single buffer.
        //Having more than this in a single buffer can result in a significant performance hit.
        uint32_t GetSoftMaxMeshVertices() const { return softMaxVertices; }
        //Gets the driver-recommended maximum number of indices for a single buffer.
        //Having more than this in a single buffer can result in a significant performance hit.
        uint32_t GetSoftMaxMeshIndices() const { return softMaxIndices; }


        //Gets the maximum number of different textures that can be given to a shader,
        //    including both actual Textures and Images.
        uint32_t GetMaxTexturesInShader() const { return maxTexturesInShader; }

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

        uint32_t softMaxVertices, softMaxIndices,
                 maxTexturesInShader, maxColorAttachments, maxColorOutputs,
                 maxUniformPrimitivesPerVertexShader, maxUniformPrimitivesPerFragmentShader;
    };
}