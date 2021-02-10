#pragma once

#include "../../RenderLibs.h"
#include "../Data.h"
#include "../../Math/Box.hpp"

//Defines a buffer's mapped memory (see Buffer.h).

namespace Bplus::GL::Buffers
{
    //How a buffer's CPU-side "map" can be used.
    BETTER_ENUM(MapUses, uint_fast8_t,
        //Mapping is only for reading the buffer.
        ReadOnly = GL_MAP_READ_BIT,
        //Mapping is only for writing to the buffer.
        WriteOnly = GL_MAP_WRITE_BIT,
        //Mapping is for both reading and writing buffer data.
        ReadWrite = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT
    );

    //How much effort must be taken to sync a buffer on the GPU
    //    with its memory map on the CPU.
    BETTER_ENUM(MapSyncModes, uint_fast8_t,
        //For as long as the buffer is mapped onto the CPU,
        //    no OpenGL actions will be taken that read from or write to it.
        None = 0,
        //OpenGL may read from or write to the buffer while it's mapped.
        //However, there is no automatic syncing
        //    between the CPU version of the data and the GPU version.
        //Note that you can still manually sync them with OpenGL memory barriers and fences.
        Basic = GL_MAP_PERSISTENT_BIT,
        //OpenGL will make sure that the CPU and GPU notice each others' changes to the buffer.
        //However, note that GPU scheduling is a complicated black box,
        //    and you may want   to do extra work to ensure that all GPU writes are finished
        //    before reading the mapped memory, such as using an OpenGL memory fence.
        Full = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
    );


    //Specifications for how a buffer can be mapped onto the CPU for easy reads/writes.
    //Less permissive uses provide more room for driver optimization.
    struct BP_API MapAbility
    {
        MapUses Usage;
        MapSyncModes Sync;
    };
}