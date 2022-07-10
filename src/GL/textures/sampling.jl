
########################
#       Filtering      #
########################

# The behavior of a texture when you sample past its boundaries.
@bp_gl_enum(WrapModes::GLenum,
    # Repeat the texture indefinitely, creating a tiling effect.
    repeat = GL_REPEAT,
    # Repeat the texture indefinitely, but mirror it across each edge.
    mirrored_repeat = GL_MIRRORED_REPEAT,
    # Clamp the coordinates so that the texture outputs its last edge pixels
    #    when going past its border.
    clamp = GL_CLAMP_TO_EDGE,
    # Outputs a custom color when outside the texture.
    custom_border = GL_CLAMP_TO_BORDER  #TODO: Support CustomBorder sampling. Add a field to Sampler for the border color, and NOTE that bindless textures have a very limited set of allowed border colors.
)
export WrapModes, E_WrapModes


# The filtering mode for a texture's pixels.
@bp_gl_enum(PixelFilters::GLenum,
    # "Nearest" sampling. Individual pixels are crisp and square.
    rough = GL_NEAREST,
    # "Linear" sampling. Blends the nearest neighbor pixels together.
    smooth = GL_LINEAR
)
export PixelFilters, E_PixelFilters

"Mip-maps can be sampled in the same way as neighboring pixels, or turned off entirely"
const MipFilters = Optional{E_PixelFilters}
export MipFilters

"Converts a texture 'mag' filter into an OpenGL enum value"
get_ogl_enum(pixel_filtering::E_PixelFilters) = GLenum(pixel_filtering)
"Converts a texture 'min' filter into an OpenGL enum value"
function get_ogl_enum(pixel_filtering::E_PixelFilters, mip_filtering::MipFilters)
    if pixel_filtering == PixelFilters.rough
        if isnothing(mip_filtering)
            return GL_NEAREST
        elseif mip_filtering == PixelFilters.rough
            return GL_NEAREST_MIPMAP_NEAREST
        elseif mip_filtering == PixelFilters.smooth
            return GL_NEAREST_MIPMAP_LINEAR
        else
            error("Unhandled case: ", mip_filtering)
        end
    elseif pixel_filtering == PixelFilters.smooth
        if isnothing(mip_filtering)
            return GL_LINEAR
        elseif mip_filtering == PixelFilters.rough
            return GL_LINEAR_MIPMAP_NEAREST
        elseif mip_filtering == PixelFilters.smooth
            return GL_LINEAR_MIPMAP_LINEAR
        else
            error("Unhandled case: ", mip_filtering)
        end
    else
        error("Unhandled case: ", pixel_filtering)
    end
end
# Note: get_ogl_enum() is already defined and exported by other GL code


######################
#       Sampler      #
######################

#TODO: Rename to TexSampler to avoid conflicts with the Random std module

"Information about a sampler for an N-dimensional texture."
Base.@kwdef struct Sampler{N}
    # The wrapping mode along each individual axis
    wrapping::Union{E_WrapModes, Vec{N, E_WrapModes}} = WrapModes.repeat

    pixel_filter::E_PixelFilters = PixelFilters.smooth
    mip_filter::MipFilters = pixel_filter

    # Anisotropic filtering.
    # Should have a value between 1 and get_context().device.max_anisotropy.
    anisotropy::Float32 = one(Float32)

    # Offsets the mip level calculation, if using mipmapping.
    # For example, a value of 1 essentially forces all samples to go up one mip level.
    mip_offset::Float32 = zero(Float32)
    # Sets the boundaries of the mip levels used in sampling.
    # As usual, 1 is the first mip (i.e. original texture), and
    #    higher values represent smaller mips.
    mip_range::IntervalU = Box_minmax(UInt32(1), UInt32(1001))

    # If this is a depth (or depth-stencil) texture,
    #    this setting makes it a "shadow" sampler.
    depth_comparison_mode::Optional{E_ValueTests} = nothing

    # Whether cubemaps should be sampled seamlessly.
    # This comes from the extension 'ARB_seamless_cubemap_per_texture',
    #    and is important for bindless cubemap textures.
    cubemap_seamless::Bool = true
end

# StructTypes doesn't have any way to deserialize an immutable @kwdef struct.
#    
StructTypes.StructType(::Type{<:Sampler}) = StructTypes.UnorderedStruct()
function StructTypes.construct(values::Vector{Any}, T::Type{<:Sampler})
    kw_args = NamedTuple()
    if isassigned(values, 1)
        kw_args = (kw_args..., wrapping=values[1])
    end
    if isassigned(values, 2)
        kw_args = (kw_args..., pixel_filter=values[2])
    end
    if isassigned(values, 3)
        kw_args = (kw_args..., mip_filter=values[3])
    end
    if isassigned(values, 4)
        kw_args = (kw_args..., anisotropy=values[4])
    end
    if isassigned(values, 5)
        kw_args = (kw_args..., mip_offset=values[5])
    end
    if isassigned(values, 6)
        kw_args = (kw_args..., mip_range=values[6])
    end
    if isassigned(values, 7)
        kw_args = (kw_args..., depth_comparison_mode=values[7])
    end
    if isassigned(values, 8)
        kw_args = (kw_args..., cubemap_seamless=values[8])
    end

    return T(; kw_args...)
end

# The "wrapping" being a union slightly complicates equality/hashing.
Base.:(==)(a::Sampler{N}, b::Sampler{N}) where {N} = (
    (a.pixel_filter == b.pixel_filter) &&
    (a.mip_filter == b.mip_filter) &&
    (a.anisotropy == b.anisotropy) &&
    (a.mip_offset == b.mip_offset) &&
    (a.mip_range == b.mip_range) &&
    (a.depth_comparison_mode == b.depth_comparison_mode) &&
    (a.cubemap_seamless == b.cubemap_seamless) &&
    if a.wrapping isa E_WrapModes
        if b.wrapping isa E_WrapModes
            a.wrapping == b.wrapping
        else
            all(i -> b.wrapping[i] == a.wrapping, 1:N)
        end
    else
        if b.wrapping isa E_WrapModes
            all(i -> a.wrapping[i] == b.wrapping, 1:N)
        else
            a.wrapping == b.wrapping
        end
    end
)
Base.hash(s::Sampler{N}, u::UInt) where {N} = hash(
    tuple(
        s.pixel_filter, s.mip_filter, s.anisotropy,
        s.mip_offset, s.mip_range,
        s.depth_comparison_mode, s.cubemap_seamless,
        if s.wrapping isa E_WrapModes
            Vec(i -> s.wrapping, N)
        else
            s.wrapping
        end
    ),
    u
)

# Convert a sampler to a different-dimensional one.
# Fill in the extra dimensions with a constant pre-chosen value,
#    to make it easier to use 3D samplers as dictionary keys.
Base.convert(::Type{Sampler{N2}}, s::Sampler{N1}) where {N1, N2} = Sampler{N2}(
    if s.wrapping isa E_WrapModes
        s.wrapping
    else
        Vec(ntuple(i -> s.wrapping[i], Val(min(N1, N2)))...,
            ntuple(i -> WrapModes.repeat, Val(max(0, N2 - N1)))...)
    end,
    s.pixel_filter, s.mip_filter,
    s.anisotropy, s.mip_offset, s.mip_range,
    s.depth_comparison_mode,
    s.cubemap_seamless
)

"Gets a sampler's wrapping mode across all axes, assuming they're all the same"
@inline function get_wrapping(s::Sampler)
    if s.wrapping isa E_WrapModes
        return s.wrapping
    else
        @bp_gl_assert(all(w-> w==s.wrapping[1], s.wrapping),
                      "Sampler's wrapping setting has different values along each axis: ", s.wrapping)
        return s.wrapping[1]
    end
end

"Applies a sampler's settings to a texture"
apply(s::Sampler, tex::Ptr_Texture) = apply_impl(s, convert(GLuint, tex), glTextureParameteri, glTextureParameterf)
"Applies a sampler's settings to an OpenGL sampler object"
apply(s::Sampler, tex::Ptr_Sampler) = apply_impl(s, convert(GLuint, tex), glSamplerParameteri, glSamplerParameterf)

"Implementation for apply() with samplers"
function apply_impl( s::Sampler{N},
                     ptr::GLuint,
                     gl_set_func_i::Function,
                     gl_set_func_f::Function
                   ) where {N}
    context::Optional{Context} = get_context()
    @bp_check(exists(context), "Can't apply sampler settings to a texture outside of an OpenGL context")

    # Set filtering.
    gl_set_func_i(ptr, GL_TEXTURE_MIN_FILTER, get_ogl_enum(s.pixel_filter, s.mip_filter))
    gl_set_func_i(ptr, GL_TEXTURE_MAG_FILTER, get_ogl_enum(s.pixel_filter))

    # Set anisotropy.
    @bp_check(s.anisotropy >= 1,
              "Sampler anisotropy of ", s.anisotropy, " is too low!",
                " It should be between 1 and ", context.device.max_anisotropy, ", inclusive")
    @bp_check(s.anisotropy <= context.device.max_anisotropy,
              "Sampler anisotropy of ", s.anisotropy,
                 " is above the GPU driver's max value of ",
                 context.device.max_anisotropy)
    gl_set_func_f(ptr, GL_TEXTURE_MAX_ANISOTROPY, s.anisotropy)

    # Set mip bias.
    @bp_check(s.mip_range.min >= 1,
              "Sampler's mip range must start at 1 or above. The requested range is: ", s.mip_range)
    gl_set_func_f(ptr, GL_TEXTURE_BASE_LEVEL, s.mip_range.min - 1)
    gl_set_func_f(ptr, GL_TEXTURE_MAX_LEVEL, max_inclusive(s.mip_range) - 1)
    gl_set_func_f(ptr, GL_TEXTURE_LOD_BIAS, s.mip_offset) #TODO: Does GL_TEXTURE_LOD_BIAS directly represent a "mip offset"?

    # Depth comparison.
    if exists(s.depth_comparison_mode)
        gl_set_func_i(ptr, GL_TEXTURE_COMPARE_MODE, GLint(GL_COMPARE_REF_TO_TEXTURE))
        gl_set_func_i(ptr, GL_TEXTURE_COMPARE_FUNC, GLint(s.depth_comparison_mode))
    else
        gl_set_func_i(ptr, GL_TEXTURE_COMPARE_MODE, GLint(GL_NONE))
    end

    # Set wrapping.
    # Interesting note: OpenGL does *not* care if you try to set this value for higher dimensions
    #    than the texture actually has.
    wrapping_keys = (GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R)
    wrapping_vec = if s.wrapping isa E_WrapModes
                       Vec(i -> s.wrapping, N)
                   else
                       s.wrapping
                   end

    for i::Int in 1:N
        gl_set_func_i(ptr, wrapping_keys[i], GLint(wrapping_vec[i]))
    end

    # Set cubemap sampling.
    gl_set_func_i(ptr, GL_TEXTURE_CUBE_MAP_SEAMLESS, GLint(GL_TRUE))
end


export Sampler, get_wrapping, apply,
       SwizzleRGBA



##############################
#   Depth/Stencil Sampling   #
##############################

# Textures containing hybrid depth/stencil data can be sampled in a few ways.
@bp_gl_enum(DepthStencilSources::GLenum,
    # The depth component is sampled, acting like a Red-only float/normalized int texture.
    depth = GL_DEPTH_COMPONENT,
    # The stencil component is sampled, acting like a Red-only uint texture.
    stencil = GL_STENCIL_INDEX
)
export DepthStencilSources, E_DepthStencilSources


#################################
#   Sampler Service Interface   #
#################################

# Samplers can be re-used between textures, so you only need to define a few sampler objects
#    across an entire rendering context.

"Gets an OpenGL sampler object matching the given settings."
@inline get_sampler(settings::Sampler)::Ptr_Sampler = get_sampler(get_context(), settings)
function get_sampler(context::Context, settings::Sampler{N})::Ptr_Sampler where {N}
    if N < 3
        get_sampler(context, convert(Sampler{3}, settings))
    else
        service::SamplerService = get_sampler_service(context)
        if !haskey(service, settings)
            handle = Ptr_Sampler(get_from_ogl(gl_type(Ptr_Sampler), glCreateSamplers, 1))
            apply(settings, handle)

            service[settings] = handle
            return handle
        else
            return service[settings]
        end
    end
end

"
Ensures the service which provides sampler objects is initialized.
This will get called within the constructor of Texture,
    so that it's always available before it's first needed
    without having to check every single time a sampler is requested.
"
function init_sampler_service(context::Context)
    try_register_service(context, SERVICE_NAME_SAMPLERS) do
        return Service(SamplerService(),
                       on_destroyed = close_sampler_service)
    end
end


######################################
#   Sampler Service Implementation   #
######################################

const SERVICE_NAME_SAMPLERS = :sampler_global_lookup
const SamplerService = Dict{Sampler{3}, Ptr_Sampler}

function close_sampler_service(s::SamplerService)
    for handle in values(s)
        glDeleteSamplers(1, Ref(gl_type(handle)))
    end
    empty!(s)
end

get_sampler_service(context::Context)::SamplerService = get_service(context, SERVICE_NAME_SAMPLERS)