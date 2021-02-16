#pragma once

#include <memory>

#include "Format.h"


namespace Bplus::GL::Textures
{
    //Sort of like a texture, but only able to be rendered into by a Target;
    //    it cannot be sampled from.
    class BP_API TargetBuffer
    {
    public:

        TargetBuffer(const Format& format, const glm::uvec2& size);
        ~TargetBuffer();

        //No copying, but moves are allowed.
        TargetBuffer(const TargetBuffer& cpy) = delete;
        TargetBuffer& operator=(const TargetBuffer& cpy) = delete;
        TargetBuffer(TargetBuffer&& src);
        TargetBuffer& operator=(TargetBuffer&& src)
        {
            //Call deconstructor, then move constructor.
            //Only bother changing things if they represent different handles.
            if (this != &src)
            {
                this->~TargetBuffer();
                new (this) TargetBuffer(std::move(src));
            }

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