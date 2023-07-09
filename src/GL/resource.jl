"
A mutable OpenGL object which cannot be copied, and whose fields should not be set directly.
Resources should be created within a Context, and can be cleaned up with `Base.close()`.
The resource's OpenGL handle must be set to a null value after the resource is closed,
    so that it's easy to see if a resource has been destroyed.
"
abstract type Resource end #TODO: Rename 'AbstractResource'

"
Gets the OpenGL handle for a resource.
By default, tries to access the `handle` property.
"
get_ogl_handle(r::Resource) = r.handle
"
Gets whether a resource as already been destroyed.
By default, checks if its OpenGL handle is null.
"
is_destroyed(r::Resource) = (r.handle == typeof(r.handle)())

Base.close(r::Resource) = error("Forgot to implement close() for ", typeof(r))

Base.setproperty!(::Resource, name::Symbol, val) = error("Cannot set the field of a Resource! ",
                                                         r, ".", name, " = ", val)
Base.deepcopy(::Resource) = error("Do not try to copy OpenGL resources")

# Unfortunately, OpenGL allows implementations to re-use the handles of destroyed textures;
#    otherwise, I'd use that for hashing/equality.

println("#TODO: All resources need a try/catch around their constructor, making sure to destroy-themselves-then-rethrow in the catch. Make a helper function here for it")
#TODO: All resources know about their owning Context, and double-check that they're not used in the wrong thread before any expensive ops

export Resource, get_ogl_handle, is_destroyed