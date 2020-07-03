#pragma once

#include "Map.h"


//A Buffer is a general-purpose array, stored in GPU memory,
//    for OpenGL to read and write.
//The most common use-case is to store mesh data.

//Buffers can be "mapped" to the CPU, allowing you to write/read them directly
//    as if they were a plain old C array.
//This is often more efficient than setting the buffer data the usual way,
//    e.x. you could read the mesh data from disk directly into this mapped memory.


namespace Bplus::GL::Buffers
{
    //A chunk of OpenGL data that can be used for all sorts of things --
    //    mesh vertices/indices, shader uniforms, compute buffers, etc.
    class BP_API Buffer
    {
    public:
        //TODO: Offer double- and triple-buffering to speed up synchronization between CPU and GPU.
        //TODO: Offer a persistent-mapped buffer as an alternative to glBufferSubData(). Maybe make Buffer an abstract base class.

        //Gets the Buffer with the given OpenGL handle.
        //Only works on the thread that OpenGL runs on.
        //Returns null if it can't be found.
        static const Buffer* Find(OglPtr::Buffer ptr);


        //Creates a buffer of the given byte-size,
        //    optionally with the ability to have its memory mapped onto the CPU
        //    for easy reads/writes.
        Buffer(uint64_t byteSize, bool canChangeData,
               const std::byte* initialData = nullptr,
               bool recommendStorageOnCPUSide = false);
        //Creates a buffer of the given size,
        //    optionally with the ability to have its memory mapped onto the CPU
        //    for easy reads/writes.
        template<typename T>
        Buffer(uint64_t nElements, bool canChangeData,
               const T* initialElements = nullptr,
               bool recommendStorageOnCPUSide = false)
            : Buffer(nElements * sizeof(T), canChangeData,
                     (const std::byte*)initialElements,
                     recommendStorageOnCPUSide) { }

        ~Buffer();

        //No copying allowed, but moves are fine.
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&);
        Buffer& operator=(Buffer&&);


        //Gets the current size of this buffer, in bytes.
        uint64_t GetByteSize() const { return byteSize; }
        //Gets the number of elements in this buffer,
        //    assuming it contains an array of type T.
        template<typename T>
        uint64_t GetSize() const
        {
            BPAssert((byteSize % sizeof(T)) == 0,
                     "Byte-size of this Buffer isn't a multiple of the element size");
            return byteSize / sizeof(T);
        }


        //Sets this buffer's data, or optionally just a subset of it.
        void SetBytes(const std::byte* newBytes,
                      Math::IntervalUL byteRange = { });
        //Sets this buffer's data, or optionally just a subset of it.
        template<typename T>
        void Set(const T* newData,
                 Math::IntervalUL elementRange = { })
        {
            SetBytes((const std::byte*)newData,
                     Math::IntervalUL::MakeMinSize(elementRange.MinCorner * sizeof(T),
                                                   elementRange.Size * sizeof(T)));
        }

        //Gets this buffer's data, or optionally just a subset of it.
        //Writes it into the given array.
        void GetBytes(std::byte* outBytes,
                      Math::IntervalUL byteRange = { }) const;
        //Gets this buffer's data, or optionally just a subset of it.
        //Writes it into the given array.
        template<typename T>
        void Get(T* outData,
                 Math::IntervalUL elementRange = { }) const
        {
            GetBytes((std::byte*)outData,
                     Math::IntervalUL::MakeMinSize(elementRange.MinCorner * sizeof(T),
                                                   elementRange.Size * sizeof(T)));
        }

        //Copies this buffer's data into the given one.
        //Optionally uses a subset of this buffer.
        //Optionally starts at a certain offset in the destination buffer.
        void CopyBytes(Buffer& destination,
                       Math::IntervalUL srcByteRange = { },
                       uint64_t destinationByteStart = 0) const;
        //Copies this buffer's data into the given one.
        //Optionally uses a subset of this buffer.
        //Optionally starts at a certain offset in the destination buffer.
        template<typename T>
        void Copy(Buffer& destination,
                  Math::IntervalUL srcElementRange = { },
                  uint64_t destinationElementStart = 0) const
        {
            CopyBytes(destination,
                      Math::IntervalUL::MakeMinSize(srcElementRange.MinCorner * sizeof(T),
                                                    srcElementRange.Size * sizeof(T)),
                      destinationElementStart * sizeof(T));
        }

    private:

        OglPtr::Buffer glPtr;
        uint64_t byteSize;
        bool canChangeData;
    };
}