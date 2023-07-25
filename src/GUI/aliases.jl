const gVec2 = CImGui.LibCImGui.ImVec2
const gVec4 = CImGui.LibCImGui.ImVec4
const gColor = CImGui.LibCImGui.ImColor

Base.convert(::Type{gVec2}, v::Vec2) = gVec2(v.data...)
Base.convert(::Type{gVec4}, v::Vec4) = gVec4(v.data...)
Base.convert(::Type{gColor}, v::Vec4) = gColor(v.data...)

Base.convert(::Type{Vec{2, F}}, v::gVec2) where {F} = Vec{2, F}(v.x, v.y)
Base.convert(::Type{Vec{4, F}}, v::gVec4) where {F} = Vec{4, F}(v.x, v.y, v.z, v.w)