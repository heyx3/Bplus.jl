#include "Buffer.h"

#include "../Context.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Buffers;


namespace
{
    thread_local struct ThreadBufferData {
        bool initializedYet = false;
        std::unordered_map<OglPtr::Buffer, const Buffer*> buffersByOglPtr;
    } threadData;

    void CheckInit()
    {
        if (!threadData.initializedYet)
        {
            threadData.initializedYet = true;

            std::function<void()> refreshContext = []()
            {
                //Nothing needs to be done right now,
                //    but this is kept here just in case it becomes useful.
            };
            refreshContext();
            Context::RegisterCallback_RefreshState(refreshContext);

            Context::RegisterCallback_Destroyed([]()
            {
                //Make sure all buffers have been cleaned up.
                //TODO: Use OpenGL's debug utilities to give the buffers names and provide more info here.
                BPAssert(threadData.buffersByOglPtr.size() == 0,
                         "Buffer memory leaks!");

                threadData.buffersByOglPtr.clear();
            });
        }
    }
}

namespace
{
    Math::IntervalUL ProcessDefaultRange(Math::IntervalUL rangeParam,
                                         uint64_t fullSize)
    {
        if (rangeParam == Math::IntervalUL{ })
            rangeParam = Math::IntervalUL::MakeSize(glm::u64vec1{ fullSize });

        return rangeParam;
    }
}


const Buffer* Buffer::Find(OglPtr::Buffer ptr)
{
    auto found = threadData.buffersByOglPtr.find(ptr);
    return (found == threadData.buffersByOglPtr.end()) ?
               nullptr :
               found->second;
}

Buffer::Buffer(uint64_t byteSize, bool _canChangeData,
               const std::byte* initialData,
               bool storeOnCPUSide)
    : byteSize(byteSize), canChangeData(_canChangeData),
      glPtr(GlCreate(glCreateBuffers))
{
    threadData.buffersByOglPtr[glPtr] = this;

    GLbitfield flags = 0;
    if (storeOnCPUSide)
        flags |= GL_CLIENT_STORAGE_BIT;
    if (canChangeData)
        flags |= GL_DYNAMIC_STORAGE_BIT;

    glNamedBufferStorage(glPtr.Get(), (GLsizeiptr)byteSize,
                         initialData, flags);
}
Buffer::~Buffer()
{
    if (!glPtr.IsNull())
    {
        threadData.buffersByOglPtr.erase(glPtr);
        glDeleteBuffers(1, &glPtr.Get());
    }
}

Buffer::Buffer(Buffer&& src)
    : glPtr(src.glPtr),
      byteSize(src.byteSize),
      canChangeData(src.canChangeData)
{
    src.glPtr = OglPtr::Buffer::Null();

    //Update the static reference to this buffer.
    auto found = threadData.buffersByOglPtr.find(glPtr);
    BPAssert(found != threadData.buffersByOglPtr.end(),
             "Un-indexed Buffer detected");
    found->second = this;
}
Buffer& Buffer::operator=(Buffer&& src)
{
    //Call deconstructor, then move constructor.
    //Only bother changing things if they represent different handles.
    if (this != &src)
    {
        this->~Buffer();
        new (this) Buffer(std::move(src));
    }

    return *this;
}

void Buffer::SetBytes(const std::byte* data, Math::IntervalUL range)
{
    BPAssert(canChangeData,
             "Can't change this buffer's data after creation");
    
    range = ProcessDefaultRange(range, byteSize);
    BPAssert(range.GetMaxCornerInclusive().x < byteSize,
             "Trying to set data past the end of this buffer");

    glNamedBufferSubData(glPtr.Get(),
                         (GLintptr)range.MinCorner.x,
                         (GLsizeiptr)range.Size.x,
                         data);
}
void Buffer::GetBytes(std::byte* data, Math::IntervalUL range) const
{
    range = ProcessDefaultRange(range, byteSize);
    BPAssert(range.GetMaxCornerInclusive().x < byteSize,
             "Trying to read data past the end of this buffer");

    glGetNamedBufferSubData(glPtr.Get(),
                            (GLintptr)range.MinCorner.x,
                            (GLsizeiptr)range.Size.x,
                            data);
}
void Buffer::CopyBytes(Buffer& dest,
                       Math::IntervalUL srcRange,
                       uint64_t destOffset) const
{
    srcRange = ProcessDefaultRange(srcRange, byteSize);
    BPAssert(srcRange.GetMaxCornerInclusive().x < byteSize,
             "Trying to copy data past the end of the source buffer");
    BPAssert(destOffset + srcRange.Size.x < dest.byteSize,
             "Trying to copy data past the end of the destination buffer");

    glCopyNamedBufferSubData(glPtr.Get(), dest.glPtr.Get(),
                             (GLintptr)srcRange.MinCorner.x,
                             (GLintptr)destOffset,
                             (GLsizeiptr)srcRange.Size.x);
}