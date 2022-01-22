"
A mutable OpenGL object which cannot be copied, and whose fields should not be set directly.
Resources should be created within a Context, and can be cleaned up with Base.close().
"
abstract type Resource end

"
Gets the OpenGL handle for a resource.
By default, tries to access the `handle` property.
"
get_ogl_handle(r::Resource) = r.handle

Base.close(r::Resource) = error("Forgot to implement close() for ", typeof(r))

Base.setproperty!(::Resource, name::Symbol, val) = error("Cannot set the field of a Resource! ",
                                                         r, ".", name, " = ", val)
Base.deepcopy(::Resource) = error("Do not try to copy OpenGL resources")

#TODO: All resources know about their owning Context, and double-check that they're not used in the wrong thread before any expensive ops

export Resource, get_ogl_handle