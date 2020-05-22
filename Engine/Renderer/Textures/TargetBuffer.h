#pragma once

#include <memory>

#include "Format.h"


namespace Bplus::GL::Textures
{
    //Sort of like a texture, but only able to be used by a Target to render into.
    //Cannot be sampled from like a real texture.
    class BP_API TargetBuffer
    {
    public:

        TargetBuffer(const Format& format, const glm::uvec2& size);
        ~TargetBuffer();

        //No copying, but moves are allowed.
        TargetBuffer(const TargetBuffer& cpy) = delete;
        TargetBuffer& operator=(const TargetBuffer& cpy) = delete;
        TargetBuffer(TargetBuffer&& from);
        TargetBuffer& operator=(TargetBuffer&& from)
        {
            //Destruct this instance, then reconstruct it in place.
            this->~TargetBuffer();
            new (this) TargetBuffer(std::move(from));

            return *this;
        }


        OglPtr::TargetBuffer GetOglPtr() const { return glPtr; }
        glm::uvec2 GetSize() const { return size; }
        Format GetFormat() const { return format; }

    private:

        OglPtr::TargetBuffer glPtr;
        glm::uvec2 size;
        Format format;
    };
}