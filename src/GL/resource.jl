"
A mutable OpenGL object which cannot be copied, and whose fields should not be set directly.
Resources should be created within a Context, and can be cleaned up with `Base.close()`.
The resource's OpenGL handle must be set to a null value after the resource is closed,
    so that it's easy to see if a resource has been destroyed.
"
abstract type AbstractResource end

"
Gets the OpenGL handle for a resource.
By default, tries to access the `handle` property.
"
get_ogl_handle(r::AbstractResource) = r.handle
"
Gets whether a resource as already been destroyed.
By default, checks if its OpenGL handle is null.
"
is_destroyed(r::AbstractResource) = (r.handle == typeof(r.handle)())

Base.close(r::AbstractResource) = error("Forgot to implement close() for ", typeof(r))

Base.setproperty!(::AbstractResource, name::Symbol, val) = error("Cannot set the field of a AbstractResource! ",
                                                                 r, ".", name, " = ", val)
Base.deepcopy(::AbstractResource) = error("Do not try to copy OpenGL resources")

# Unfortunately, OpenGL allows implementations to re-use the handles of destroyed textures;
#    otherwise, I'd use that for hashing/equality.

export AbstractResource, get_ogl_handle, is_destroyed