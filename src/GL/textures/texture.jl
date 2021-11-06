#TODO: Texture2DMSAA class.
#TODO: Copying from one texture to another (and from framebuffer into texture? it's redundant though).
#TODO: Memory Barriers. https://www.khronos.org/opengl/wiki/Memory_Model#Texture_barrier

"
Some kind of organized color data which can be 'sampled'.
This includes typical 2D textures, as well as 3D textures and 'cubemaps'.
"
mutable struct Texture <: Resource
    handle::Ptr_Texture
    type::E_TexTypes

    format::TexFormat

    # 3D textures have a full 3-dimensional size.
    # 2D/Cubemap textures have a constant Z size of 1.
    # 1D textures have a contant YZ size of 1.
    size::v3u

    # 3D textures have a full 3D sampler.
    # Other textures ignore the higher dimensions which don't apply to them.
    sampler::Sampler{3}

    # If this is a hybrid depth/stencil texture,
    #    only one of those two components can be sampled at a time.
    depth_stencil_sampling::Optional{E_DepthStencilSources}
end
export Texture

function Base.close(t::Texture)
    glDeleteTextures(1, Ref(t.handle))
    setfield!(t, :handle, Ptr_Texture())
end


############################
#       Constructors       #
############################


"Creates a 1D texture"
function Texture( format::TexFormat,
                  width::Integer
                  ;
                  sampler::Sampler{1} = Sampler{1}(),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing
                )
    return generate_texture(
        TexTypes.oneD, format, v3u(width, 1, 1),
        convert(Sampler{3}, sampler),
        depth_stencil_sampling,
        nothing
    )
end
function Texture( format::TexFormat,
                  initial_data::PixelBuffer{1}
                  ;
                  sampler::Sampler{1} = Sampler{1}(),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing
                )
    return generate_texture(
        TexTypes.oneD, format, v3u(length(initial_data), 1, 1),
        convert(Sampler{3}, sampler),
        depth_stencil_sampling,
        initial_data
    )
end

"Creates a 2D texture"
function Texture( format::TexFormat,
                  size::Vec2{<:Integer}
                  ;
                  sampler::Sampler{2} = Sampler{2}(),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing
                )
    return generate_texture(
        TexTypes.twoD, format, v3u(size, 1),
        convert(Sampler{3}, sampler),
        depth_stencil_sampling,
        nothing
    )
end
function Texture( format::TexFormat,
                  initial_data::PixelBuffer{2}
                  ;
                  sampler::Sampler{2} = Sampler{2}(),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing
                )
    return generate_texture(
        TexTypes.twoD, format,
        v3u(Vec(size(initial_data)), 1),
        convert(Sampler{3}, sampler),
        depth_stencil_sampling,
        initial_data
    )
end

"Creates a 3D texture"
function Texture( format::TexFormat,
                  size::Vec3{<:Integer}
                  ;
                  sampler::Sampler{3} = Sampler{3}(),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing
                )
    return generate_texture(
        TexTypes.threeD, format, size,
        sampler,
        depth_stencil_sampling,
        nothing
    )
end
function Texture( format::TexFormat,
                  initial_data::PixelBuffer{3}
                  ;
                  sampler::Sampler{3} = Sampler{3}(),
                  depth_stencil_sampling::Optional{E_DepthStencilSources} = nothing
                )
    return generate_texture(
        TexTypes.threeD, format,
        v3u(size(initial_data)),
        sampler,
        depth_stencil_sampling,
        initial_data
    )
end

#TODO: Handle cubemap (constructor, and new behavior in generate_texture()), after implementing cube.jl

function generate_texture( type::E_TexTypes,
                           format::TexFormat,
                           size::v3u,
                           sampler::Sampler{3},
                           depth_stencil_sampling::Optional{E_DepthStencilSources},
                           initial_data::Union{ Nothing,
                                                #= #TODO:Cubemap data buffer type =#
                                                @unionspec(PixelBuffer, 1, 2, 3, 4)
                                              }
                         )::Texture
    # Verify the format/sampling settings are valid for this texture.
    @bp_check(exists(get_ogl_enum(format)),
              "Texture format isn't valid: ", sprint(show, format))
    if is_depth_and_stencil(format)
        @bp_check(exists(depth_stencil_sampling),
                  "Using a hybrid depth/stencil texture but didn't provide a depth_stencil_sampling mode")
    else
        @bp_check(isnothing(depth_stencil_sampling),
                  "Can't provide a depth_stencil_sampling mode to a texture that isn't hybrid depth/stencil")
    end
    if !(format isa E_DepthStencilFormats) || is_stencil_only(format)
        @bp_check(isnothing(sampler.depth_comparison_mode),
                    "Can't use a 'shadow sampler' for a non-depth texture")
    end
    @bp_check(is_supported(format, type),
              "Texture type ", type, " doesn't support the format ", sprint(show, format))

    tex::Texture = Texture(get_from_ogl(gl_type(Ptr_Texture), glCreateTextures, 1),
                           type, format, size, sampler, depth_stencil_sampling)

    #TODO: Configure the texture with OpenGL calls.
    #TODO: Upload the initial data, if given

    return tex
end
#TODO: Implement generate_texture()


##########################
#    Public Interface    #
##########################

get_ogl_handle(t::Texture) = t.handle

#TODO: All texture functions