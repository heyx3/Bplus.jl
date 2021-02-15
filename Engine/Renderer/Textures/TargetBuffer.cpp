#include "TargetBuffer.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Textures;


TargetBuffer::TargetBuffer(const Format& _format,
                           const glm::uvec2& _size)
    : glPtr(OglPtr::TargetBuffer::null),
      size(_size), format(_format)
{
    BPAssert(_format.GetOglEnum() != GL_NONE,
             "Invalid format for TargetBuffer");
    BPAssert(!_format.IsCompressed(),
             "Can't render to compressed formats");

    glCreateRenderbuffers(1, &glPtr.Get());
    glNamedRenderbufferStorage(glPtr.Get(), format.GetOglEnum(),
                               (GLsizei)size.x, (GLsizei)size.y);
}
TargetBuffer::~TargetBuffer()
{
    if (!glPtr.IsNull())
        glDeleteRenderbuffers(1, &glPtr.Get());
}

TargetBuffer::TargetBuffer(TargetBuffer&& from)
    : glPtr(from.glPtr), size(from.size), format(from.format)
{
    from.glPtr = OglPtr::TargetBuffer::Null();
}