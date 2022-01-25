"""
A reference to part or all of a texture, to be attached to a Target and rendered into.

You can attach any texture at any mip level.
In the case of 3D and cubemap textures,
    you can optionally attach a specific "layer" of that texture.
The layers of a cubemap texture are its faces, ordered in the usual way.
The layers of a 3D texture are its Z-slices.

Note that mip levels and layers are counted starting at 1,
    to be consistent with Julia's 1-based convention.
"""
struct TargetOutput
    mip_level::Int
    tex::Texture
    layer::Union{Nothing, E_CubeFaces, Int}
end

function Base.show(io::IO, o::TargetOutput)
    if o.mip_level != 1
        print(io, "Mip #", o.mip_level, " of a ")
    else
        print(io, "A ")
    end

    print(io, tex.type, " texture")

    if o.layer isa Nothing
        # Print nothing in this case.
    elseif o.layer isa E_CubeFaces
        print(io, "; face ", o.layer)
    elseif o.layer isa Int
        print(io, "; Z-slice #", o.layer)
    else
        print(io, "ERROR_CASE(", typeof(o.layer), ")")
    end
end

"Gets this output's render size."
output_size(t::TargetOutput)::uvec2 = t.tex.size.xy

"Gets this output's render layer."
output_layer(t::TargetOutput) = output_layer(t.layer)
output_layer(::Nothing) = 1
output_layer(face::E_CubeFaces) = enum_to_index(face)
output_layer(z_slice::Int) = z_slice

"Gets the number of layers in this output."
output_layer_count(t::TargetOutput)::Int = output_layer_count(t.tex.type, t.layer)
output_layer_count(tex_type::E_TexTypes, ::Nothing) =
    if tex_type in (TexTypes.oneD, TexTypes.twoD)
        1
    elseif tex_type == TexTypes.threeD
        Int(tex_type.size.z)
    elseif tex_type == TexTypes.cube_map
        6
    end
output_layer_count(E_TexTypes, ::Union{Int, E_CubeFaces}) = 1

export TargetOutput