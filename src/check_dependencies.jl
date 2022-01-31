using ModernGL
if GLsizei == 64
    error("ModernGL package should use a custom fork that adds required ARB extensions. ",
          " Try adding it with `] add https://github.com/heyx3/ModernGL.jl#extensions`.")
end