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


BufferMap::BufferMap(Buffer& source, bool read, bool write)
    : src(&source), canRead(read), canWrite(write)
{
    //Convert parameters to OpenGL enum values.
    GLenum readWrite;
    if (canRead & canWrite)
        readWrite = GL_READ_WRITE;
    else if (canRead)
        readWrite = GL_READ_ONLY;
    else
    {
        BPAssert(canWrite, "Neither read nor write were passed into BufferMap()");
        readWrite = GL_WRITE_ONLY;
    }

    //Create the map.
    dataPtr = glMapNamedBuffer(src->dataPtr.Get(), readWrite);
}
BufferMap::~BufferMap()
{
    if (dataPtr != nullptr)
        glUnmapNamedBuffer(src->dataPtr.Get());
}

BufferMap::BufferMap(BufferMap&& srcM)
    : canRead(srcM.canRead), canWrite(srcM.canWrite)
{
    src = srcM.src;
    srcM.src = nullptr;

    dataPtr = srcM.dataPtr;
    srcM.dataPtr = nullptr;
}
BufferMap& BufferMap::operator=(BufferMap&& srcM)
{
    //Call deconstructor, then move constructor.
    this->~BufferMap();
    new (this) BufferMap(std::move(srcM));

    return *this;
}

const std::byte* BufferMap::GetBytes() const
{
    BPAssert(canRead, "Buffer isn't readable");
    return (const std::byte*)dataPtr;
}
std::byte* BufferMap::GetBytes_Writable()
{
    BPAssert(canWrite, "Buffer isn't writable");
    return (std::byte*)dataPtr;
}



Buffer::Buffer()
    : hintFrequency(Bplus::GL::BufferHints_Frequency::Static),
      hintPurpose(Bplus::GL::BufferHints_Purpose::Draw),
      byteSize(0)
{
    CheckInit();

    glCreateBuffers(1, &dataPtr.Get());
}
Buffer::~Buffer()
{
    if (dataPtr != OglPtr::Buffer::Null)
        glDeleteBuffers(1, &dataPtr.Get());

    for (size_t i = 0; i < Bplus::GL::BufferModes::_size_constant; ++i)
        if (threadData.currentBuffers[i] == this)
            threadData.currentBuffers[i] = nullptr;
}

Buffer::Buffer(Buffer&& src)
    : dataPtr(src.dataPtr),
      hintFrequency(Bplus::GL::BufferHints_Frequency::Static),
      hintPurpose(Bplus::GL::BufferHints_Purpose::Draw),
      byteSize(src.byteSize),
      map(std::move(src.map))
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
    BPAssert(!IsMapped(),
             "Trying to read a buffer that is currently mapped");
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

BufferMap& Buffer::Map(bool readable, bool writable)
{
    BPAssert(!IsMapped(), "Buffer is already mapped!");
    map.emplace(*this, readable, writable);
    return *map;
}
void Buffer::Unmap()
{
    BPAssert(IsMapped(), "Trying to unmap a buffer that isn't mapped");
    map.reset();
}

GLenum Buffer::GetUsageHint() const
{
    GLenum glPurposeHints[BufferHints_Purpose::_size_constant];
    switch (hintFrequency)
    {
        case BufferHints_Frequency::Stream:
            glPurposeHints[(+BufferHints_Purpose::Draw)._to_index()] = GL_STREAM_DRAW;
            glPurposeHints[(+BufferHints_Purpose::Read)._to_index()] = GL_STREAM_READ;
            glPurposeHints[(+BufferHints_Purpose::Copy)._to_index()] = GL_STREAM_COPY;
            break;
        case BufferHints_Frequency::Static:
            glPurposeHints[(+BufferHints_Purpose::Draw)._to_index()] = GL_STATIC_DRAW;
            glPurposeHints[(+BufferHints_Purpose::Read)._to_index()] = GL_STATIC_READ;
            glPurposeHints[(+BufferHints_Purpose::Copy)._to_index()] = GL_STATIC_COPY;
            break;
        case BufferHints_Frequency::Dynamic:
            glPurposeHints[(+BufferHints_Purpose::Draw)._to_index()] = GL_DYNAMIC_DRAW;
            glPurposeHints[(+BufferHints_Purpose::Read)._to_index()] = GL_DYNAMIC_READ;
            glPurposeHints[(+BufferHints_Purpose::Copy)._to_index()] = GL_DYNAMIC_COPY;
            break;

        default:
            std::string msg = "Unknown frequency hint: ";
            msg += hintFrequency._to_string();
            BPAssert(false, msg.c_str());
            break;
    }

    return glPurposeHints[hintPurpose._to_index()];
}

void Buffer::SetByteData(const std::byte* data,
                         size_t offset, size_t count)
{
    BPAssert(offset + count <= byteSize,
             "Trying to write past the end of a buffer");

    //If currently memory-mapped, use the map.
    if (IsMapped())
    {
        BPAssert(map->CanWrite(),
                 "Trying to write to a buffer that is currently read-only-mapped");
        BPAssert(byteSize == count,
                 "Trying to change the size of a mapped buffer");

        memcpy(map->GetBytes_Writable() + offset, data, count);
    }
    //Otherwise, use the usual way of setting buffer data.
    else if (offset == 0)
    {
        byteSize = count;
        glNamedBufferData(dataPtr.Get(), count, data, GetUsageHint());
    }
    else
    {
        glNamedBufferSubData(dataPtr.Get(), offset, count, data);
    }
}
void Buffer::GetByteData(std::byte* outData,
                         size_t offset, size_t count) const
{
    //If currently memory-mapped, use the map.
    if (IsMapped())
    {
        BPAssert(map->CanRead(),
                 "Trying to read from a buffer that is currently write-only-mapped");

        memcpy(outData, map->GetBytes(), count);
    }
    else
    {
        glGetNamedBufferSubData(dataPtr.Get(), offset, count,
                                outData);
    }
}