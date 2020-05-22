#include "Buffer.h"

#include "Context.h"

using namespace Bplus;
using namespace Bplus::GL;


namespace
{
    thread_local struct ThreadBufferData {
        bool initializedYet = false;
        const Buffer* currentBuffers[Bplus::GL::BufferModes::_size_constant];

        ThreadBufferData() {
            for (size_t i = 0; i < Bplus::GL::BufferModes::_size_constant; ++i)
                currentBuffers[i] = nullptr;
        }
    } threadData;

    void CheckInit()
    {
        if (!threadData.initializedYet)
        {
            threadData.initializedYet = true;

            std::function<void()> refreshContext = []()
            {
                
            };
            refreshContext();
            Context::RegisterCallback_RefreshState(refreshContext);

            Context::RegisterCallback_Destroyed([]()
            {

            });
        }
    }
}


const Buffer* Buffer::GetCurrentBuffer(BufferModes slot)
{
    return threadData.currentBuffers[slot._to_index()];
}

Buffer::Buffer()
    : hintFrequency(Bplus::GL::BufferHints_Frequency::SetOnce_ReadOften),
      hintPurpose(Bplus::GL::BufferHints_Purpose::SetCPU_ReadGPU),
      byteSize(0)
{
    CheckInit();

    glCreateBuffers(1, &dataPtr.Get());
}
Buffer::~Buffer()
{
    if (!dataPtr.IsNull())
        glDeleteBuffers(1, &dataPtr.Get());

    for (size_t i = 0; i < Bplus::GL::BufferModes::_size_constant; ++i)
        if (threadData.currentBuffers[i] == this)
            threadData.currentBuffers[i] = nullptr;
}

Buffer::Buffer(Buffer&& src)
    : dataPtr(src.dataPtr),
      hintFrequency(src.hintFrequency),
      hintPurpose(src.hintPurpose),
      byteSize(src.byteSize)
{
    src.dataPtr = OglPtr::Buffer(OglPtr::Buffer::Null);
}
Buffer& Buffer::operator=(Buffer&& src)
{
    //Call deconstructor, then move constructor.
    this->~Buffer();
    new (this) Buffer(std::move(src));

    return *this;
}

void Buffer::CopyTo(Buffer& dest,
                    size_t srcByteStartI, size_t srcByteSize,
                    size_t destByteStartI) const
{
    //Handle default parameter values.
    if (srcByteSize < 1)
        srcByteSize = byteSize;

    //Do error-checking.
    BPAssert(srcByteStartI + srcByteSize <= byteSize,
             "Trying to copy past the end of the source buffer");
    BPAssert(destByteStartI + srcByteSize <= dest.byteSize,
             "Trying to copy past the end of the destination buffer");

    //Finally, do the actual copy.
    glCopyNamedBufferSubData(dataPtr.Get(), dest.dataPtr.Get(),
                             srcByteStartI, destByteStartI,
                             srcByteSize);
}
Buffer Buffer::Clone(size_t startByteI, size_t byteCount) const
{
    Buffer newB;
    CopyTo(newB, startByteI, byteCount);
    return newB;
}

GLenum Buffer::GetUsageHint() const
{
    GLenum glPurposeHints[BufferHints_Purpose::_size_constant];
    switch (hintFrequency)
    {
        case BufferHints_Frequency::SetOnce_ReadRarely:
            glPurposeHints[(+BufferHints_Purpose::SetCPU_ReadGPU)._to_index()] = GL_STREAM_DRAW;
            glPurposeHints[(+BufferHints_Purpose::SetGPU_ReadCPU)._to_index()] = GL_STREAM_READ;
            glPurposeHints[(+BufferHints_Purpose::OnlyGPU)._to_index()] = GL_STREAM_COPY;
            break;
        case BufferHints_Frequency::SetOnce_ReadOften:
            glPurposeHints[(+BufferHints_Purpose::SetCPU_ReadGPU)._to_index()] = GL_STATIC_DRAW;
            glPurposeHints[(+BufferHints_Purpose::SetGPU_ReadCPU)._to_index()] = GL_STATIC_READ;
            glPurposeHints[(+BufferHints_Purpose::OnlyGPU)._to_index()] = GL_STATIC_COPY;
            break;
        case BufferHints_Frequency::UseOften:
            glPurposeHints[(+BufferHints_Purpose::SetCPU_ReadGPU)._to_index()] = GL_DYNAMIC_DRAW;
            glPurposeHints[(+BufferHints_Purpose::SetGPU_ReadCPU)._to_index()] = GL_DYNAMIC_READ;
            glPurposeHints[(+BufferHints_Purpose::OnlyGPU)._to_index()] = GL_DYNAMIC_COPY;
            break;

        default:
            std::string msg = "Unknown frequency hint: ";
            msg += hintFrequency._to_string();
            BPAssert(false, msg.c_str());
            break;
    }

    return glPurposeHints[hintPurpose._to_index()];
}

void Buffer::SetNewData(const std::byte* data, size_t newSize)
{
    byteSize = newSize;
    glNamedBufferData(dataPtr.Get(), newSize, data, GetUsageHint());
}
void Buffer::ChangeData(const std::byte* data, size_t offset, size_t count)
{
    BPAssert(offset + count <= byteSize,
             "Trying to write past the end of the buffer");

    glNamedBufferSubData(dataPtr.Get(), offset, count, data);
}

void Buffer::GetByteData(std::byte* outData,
                         size_t offset, size_t count) const
{
    glGetNamedBufferSubData(dataPtr.Get(), offset, count, outData);
}