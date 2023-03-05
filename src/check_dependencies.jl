using ModernGL
if GLsizei == 64
    error("ModernGL package should use a custom fork that adds some ARB extensions. ",
          " Try adding the package with `] add https://github.com/heyx3/ModernGL.jl#extensions`.")
end