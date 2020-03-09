#pragma once

#include <optional>
#include "Data.h"


namespace Bplus::GL
{
    class BP_API Buffer;

    //A CPU-side mapping of a Buffer's data.
    //You can manipulate this data as if it's normal CPU data,
    //    but it's actually coming from the GPU.
    //This class is managed entirely by the Buffer class below;
    //    you cannot create it directly, and it is usually recommended
    //    to use Buffer::SetData() and Buffer::GetData() instead.
    class BP_API BufferMap
    {
    public:

        BufferMap(BufferMap&&);
        BufferMap& operator=(BufferMap&&);

        //No copying.
        BufferMap(const BufferMap&) = delete;
        BufferMap& operator=(const BufferMap&) = delete;


        //Note that there is possible undefined behavior here:
        //    we don't/can't protect against reading from a write-only buffer,
        //    so users of this class have to make sure they respect the limits.

        const Buffer& GetSource() const { return *src; }
                bool  CanRead()   const { return canRead; }
                bool  CanWrite()  const { return canWrite; }

        const std::byte* GetBytes()          const;
              std::byte* GetBytes_Writable()      ;
              
        template<typename T> const T* GetData()          const { return (const T*)GetBytes(); }
        template<typename T>       T* GetData_Writable()       { return (      T*)GetBytes_Writable(); }


    private:

        friend class Buffer;

        Buffer* src;
        void* dataPtr;

        bool canRead, canWrite;

        BufferMap(Buffer& source, bool supportRead, bool supportWrite);
        ~BufferMap();
    };



    //A chunk of OpenGL data that can be used for all sorts of things --
    //    mesh vertices/indices, shader uniforms, compute buffers, etc.
    class BP_API Buffer
    {
    public:

        //Gets the buffer currently-occupying the given slot.
        //Returns nullptr if no buffer is.
        static const Buffer* GetCurrentBuffer(BufferModes slot);


        Buffer();
        ~Buffer();

        Buffer(Buffer&&);
        Buffer& operator=(Buffer&&);
            
        //Forbid automatic copying to avoid performance traps.
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;


        //Gets the current size of this buffer, in bytes.
        size_t GetByteSize() const { return byteSize; }

        //Gets the number of elements in this buffer,
        //    assuming it contains elements of type T.
        template<typename T>
        size_t GetCount() const
        {
            BPAssert((byteSize % sizeof(T)) == 0,
                     "Byte-size of this Buffer isn't a multiple of the element size");
            return byteSize / sizeof(T);
        }


        //Creates space for the buffer, without initializing its elements.
        //You do NOT need to call this before "SetData()".
        void Init(size_t byteCount,
                  BufferHints_Frequency frequency, BufferHints_Purpose purpose)
        {
            SetData<std::byte>(nullptr, byteCount, frequency, purpose);
        }

        //TODO: SetData should take a dest offset.

        //Sets this buffer's data, erasing all previous data.
        void SetData(const std::byte* bytes, size_t count,
                     BufferHints_Frequency frequency, BufferHints_Purpose purpose)
        {
            hintFrequency = frequency;
            hintPurpose = purpose;
            SetByteData(bytes, count);
        }
        //Sets this buffer's data, erasing all previous data.
        template<typename T>
        void SetData(const T* data, size_t count,
                     BufferHints_Frequency frequency, BufferHints_Purpose purpose)
        {
            SetData((std::byte*)data, count, frequency, purpose);
        }

        //TODO: GetData and GetData<>.

        //Copies this buffer's data into the given one.
        //Optionally uses a subset of the source or destination buffer.
        void CopyTo(Buffer& dest,
                    size_t srcByteStartI = 0, size_t srcByteSize = 0,
                    size_t destByteStartI = 0) const;
        //Copies this buffer's data into the given one.
        //Optionally uses a subset of the source or destination buffer.
        //The template paramter is the type of data in the buffer,
        //    which is used to convert the index and size into bytes.
        template<typename T>
        void CopyTo(Buffer& dest,
                    size_t srcStartI = 0, size_t srcCount = 0,
                    size_t destStartI = 0) const
        {
            CopyTo(dest,
                   srcStartI * sizeof(T), srcCount * sizeof(T),
                   destStartI * sizeof(T));
        }


        //Explicitly creates a separate copy of the buffer.
        //Optionally takes a subset of the original buffer.
        Buffer Clone(size_t startByteI = 0, size_t byteCount = 0) const;
        //Explicitly creates a separate copy of the buffer.
        //Optionally takes a subset of the original buffer.
        //The template paramter is the type of data in the buffer,
        //    which is used to convert the index and size into bytes.
        template<typename T>
        Buffer Clone(size_t startI = 0, size_t count = 0) const
        {
            return Clone(startI * sizeof(T), count * sizeof(T));
        }

        //Creates a map for this buffer.
        //Unmap() must be called before calling this again.
        BufferMap& Map(bool readable, bool writable);
        //Clears the map for this buffer.
        void Unmap();
        
        //Gets the current map for this buffer, or null if one doesn't exist.
        BufferMap* GetMap() { return map.has_value() ? &map.value() : nullptr; }
        //Gets whether this buffer is currently mapped to CPU memory.
        bool IsMapped() const { return map.has_value(); }


    private:

        friend class BufferMap;
        std::optional<BufferMap> map;

        OglPtr::Buffer dataPtr;
        size_t byteSize;
        BufferHints_Frequency hintFrequency;
        BufferHints_Purpose hintPurpose;

        void SetByteData(const std::byte* data,
                         size_t offset, size_t byteCount);
        void GetByteData(std::byte* outData,
                         size_t offset, size_t byteCount) const;

        GLenum GetUsageHint() const;
    };
}