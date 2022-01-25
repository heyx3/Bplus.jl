using ReadOnlyArrays
if !@isdefined(ReadOnlyVector)
    error("ReadOnlyArrays package seems to be using an earlier version than v0.2.0.",
          " Try adding it with `] add ReadOnlyArrays#master`.")
end

using ModernGL
if GLsizei == 64
    error("ModernGL package should use a custom fork that adds required ARB extensions. ",
          " Try adding it with `] add https://github.com/heyx3/ModernGL.jl#extensions`.")
end