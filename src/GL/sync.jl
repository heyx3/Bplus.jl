"
Waits until OpenGL is done executing all previously-issued render commands.

This has limited usefulness, but here are a few scenarios:

* You are sharing resources with another context (not natively supported in B+ anyway),
    and must ensure the shared resources are done being written to by the source context.
* You are tracking down a driver bug or tricky compute bug,
    and want to invoke this after every command to track down the one that crashes.
* You are working around a driver bug.
"
function gl_execute_everything()
    glFinish()
end
export gl_execute_everything
#NOTE: glFlush() is not provided here because it doesn't seem useful for anything these days.

"
If you want to sample from the same texture you are rendering to,
    which is legal as long as the read area and write area are distinct,
    you should call this afterwards to ensure the written pixels can be read.

This helps you ping-pong within a single target rather than having to set up two of them.
"
function gl_flush_texture_writes_in_place()
    glTextureBarrier()
end
export gl_flush_texture_writes_in_place


@bp_gl_bitfield(MemoryActions::GLbitfield,
    mesh_vertex_data = GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT,
    mesh_index_data = GL_ELEMENT_ARRAY_BARRIER_BIT,

    buffer_download_or_upload = GL_BUFFER_UPDATE_BARRIER_BIT,
    buffer_uniforms = GL_UNIFORM_BARRIER_BIT,
    buffer_storage = GL_SHADER_STORAGE_BARRIER_BIT,

    texture_samples = GL_TEXTURE_FETCH_BARRIER_BIT,
    texture_simple_views = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT,
    texture_upload_or_download = GL_TEXTURE_UPDATE_BARRIER_BIT,

    target_attachments = GL_FRAMEBUFFER_BARRIER_BIT,

    # Not yet relevant to B+, but will be one day:
    indirect_draw = GL_COMMAND_BARRIER_BIT,
    texture_async_download_or_upload = GL_PIXEL_BUFFER_BARRIER_BIT,
    buffer_map_usage = GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT,
    buffer_atomics = GL_ATOMIC_COUNTER_BARRIER_BIT,
    queries = GL_QUERY_BUFFER_BARRIER_BIT,

    @USE_IN_FRAGMENT_SHADERS(texture_samples | texture_simple_views | target_attachments |
                             buffer_uniforms | buffer_storage | buffer_atomics)
)

"
Inserts a memory barrier in OpenGL so that the given operations
  definitely happen *after* all shader operations leading up to this call.

For example, if you ran a compute shader to write to texture and are about to sample from it,
  call `gl_catch_up_to(SyncTypes.texture_samples)`
"
function gl_catch_up_before(actions::E_MemoryActions)
    glMemoryBarrier(actions)
end
"
Inserts a memory barrier in OpenGL, like `gl_catch_up_before()`,
    but specifically for the subset of the framebuffer that was just rendered to,
    in preparation for more fragment shader behavior.

The actions you pass in must be a subset of `MemoryActions.USE_IN_FRAGMENT_SHADERS`.
"
function gl_catch_up_renders_before(actions::E_MemoryActions)
    glMemoryBarrierByRegion(actions)
end

export gl_catch_up_before, gl_catch_up_renders_before,
       E_MemoryActions, MemoryActions