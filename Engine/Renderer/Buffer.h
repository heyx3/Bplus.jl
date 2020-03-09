#pragma once

#include "Data.h"


namespace Bplus::GL
{
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

        //Sets this buffer's data, erasing all previous data.
        void SetData(const std::byte* bytes, size_t count,
                     BufferHints_Frequency frequency, BufferHints_Purpose purpose)
        {
            if (frequency == hintFrequency && purpose == hintPurpose && count == byteSize)
                ChangeData(bytes);
            else
            {
                hintFrequency = frequency;
                hintPurpose = purpose;
                SetNewData(bytes, count);
            }
        }
        //Sets this buffer's data, erasing all previous data.
        //The template parameter is the type of data in the buffer,
        //    which is used to convert the index and size into bytes.
        template<typename T>
        void SetData(const T* data, size_t count,
                     BufferHints_Frequency frequency, BufferHints_Purpose purpose)
        {
            SetData((std::byte*)data, count, frequency, purpose);
        }

        //Changes the data in this buffer, without re-allocating it.
        //Optionally can change only a part of the buffer.
        void ChangeData(const std::byte* bytes,
                        size_t offset = 0, size_t count = 0);

        //Gets the data from the buffer and writes it into the given array.
        //Optionally reads only a subset of the buffer.
        void GetData(std::byte* outBytes, size_t offset = 0, size_t count = 0)
        {
            if (count < 1)
                count = byteSize;
            GetByteData(outBytes, offset, count);
        }
        //Gets the data from the buffer and writes it into the given array.
        //Optionally reads only a subset of the buffer.
        //The template parameter is the type of data in the buffer,
        //    which is used to convert the index and size into bytes.
        template<typename T>
        void GetData(T* outData, size_t offset = 0, size_t count = 0)
        {
            GetData((std::byte*)outData, offset * sizeof(T), count * sizeof(T));
        }

        //Copies this buffer's data into the given one.
        //Optionally uses a subset of the source or destination buffer.
        void CopyTo(Buffer& dest,
                    size_t srcByteStartI = 0, size_t srcByteSize = 0,
                    size_t destByteStartI = 0) const;
        //Copies this buffer's data into the given one.
        //Optionally uses a subset of the source or destination buffer.
        //The template parameter is the type of data in the buffer,
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
        //The template parameter is the type of data in the buffer,
        //    which is used to convert the index and size into bytes.
        template<typename T>
        Buffer Clone(size_t startI = 0, size_t count = 0) const
        {
            return Clone(startI * sizeof(T), count * sizeof(T));
        }


    private:

        OglPtr::Buffer dataPtr;
        size_t byteSize;
        BufferHints_Frequency hintFrequency;
        BufferHints_Purpose hintPurpose;

        void SetNewData(const std::byte* data, size_t newBufferSize);

        void GetByteData(std::byte* outData,
                         size_t offset, size_t byteCount) const;

        GLenum GetUsageHint() const;
    };
}