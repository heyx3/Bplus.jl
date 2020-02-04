#pragma once

#include <optional>
#include "Data.h"


namespace Bplus::GL
{
    //A chunk of OpenGL data that can be used for all sorts of things --
    //    mesh vertices/indices, shader uniforms, compute buffers, etc.
    class BP_API Buffer
    {
    public:

        Buffer();
        ~Buffer();

        Buffer(Buffer&&);
        Buffer& operator=(Buffer&&);
            
        //Forbid automatic copying to avoid performance bugs.
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;


        void SetIndexData(uint16_t *indices, size_t count);
        void SetIndexData(uint32_t *indices, size_t count);

        //TODO: SetVertexData.


    private:

        OglPtr::Buffer dataPtr;
    };
}