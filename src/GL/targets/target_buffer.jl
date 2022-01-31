"""
Something like a texture, but highly optimized for rendering into it and not much else --
    you cannot easily manipulate or sample its data.
The OpenGL term for this is "renderbuffer".

The main use for this is as a depth or stencil buffer,
    any time you don't care about sampling depth/stencil data
    (it can still be used for depth/stencil tests, just not sampled in shaders).

This is a Resource, but it's managed internally by Target instances.
You probably won't interact with it much yourself.
"""
mutable struct TargetBuffer <: Resource
    handle::Ptr_TargetBuffer
    size::v2i
    format::TexFormat
end

function TargetBuffer(size::Vec2{<:Integer}, format::TexFormat)
    @bp_check(exists(get_context()), "Can't create a TargetBuffer without a Context")

    gl_format::Optional{GLenum} = get_ogl_enum(format)
    @bp_check(exists(gl_format),
              "Invalid format given to a TargetBuffer: ", format)
    @bp_check(!(format isa E_CompressedFormats),
              "Can't use a compressed format (", format, ") for a TargetBuffer")

    handle = Ptr_TargetBuffer(get_from_ogl(gl_type(Ptr_TargetBuffer), glCreateRenderbuffers, 1))
    glNamedRenderbufferStorage(handle, gl_format, size...)

    return TargetBuffer(handle, v2i(size), format)
end

function Base.close(b::TargetBuffer)
    glDeleteRenderbuffers(1, Ref(gl_type(b.handle)))
    setfield!(b, :handle, Ptr_TargetBuffer())
end

export TargetBuffer