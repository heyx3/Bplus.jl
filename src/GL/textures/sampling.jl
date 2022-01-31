
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

"Information about a sampler for an N-dimensional texture"
struct Sampler{N}
    # The wrapping mode along each individual axis
    wrapping::Vec{N, E_WrapModes}

    pixel_filter::E_PixelFilters
    mip_filter::MipFilters

    # Anisotropic filtering.
    # Should have a value between 1 and get_context().device.max_anisotropy.
    anisotropy::Float32

    # Offsets the mip level calculation, if using mipmapping.
    # For example, a value of 1 essentially forces all samples to go up one mip level.
    mip_offset::Float32
    # Sets the boundaries of the mip levels used in sampling.
    # As usual, 1 is the firt mip (i.e. original texture), and
    #    higher values represent smaller mips.
    mip_range::IntervalU

    # If this is a depth (or depth-stencil) texture,
    #    this setting makes it a "shadow" sampler.
    depth_comparison_mode::Optional{E_ValueTests}
end

"Simplified constructors"
Sampler{N}( wrapping,
            filter = PixelFilters.smooth,
            mip_filter = filter
          ) where {N} = Sampler{N}(
    wrapping=wrapping,
    filter=filter,
    mip_filter=mip_filter
)
Sampler{N}(; wrapping = WrapModes.clamp,
             filter = PixelFilters.smooth,
             anisotropy = @f32(1),
             mip_filter = filter,
             mip_offset = @f32(0),
             mip_range = Box_minmax(UInt(1), UInt(1001)),
             depth_comparison_mode = nothing
          ) where {N} = Sampler{N}(
    Vec(ntuple(i->wrapping, N)), filter, mip_filter,
    anisotropy,
    mip_offset, mip_range,
    depth_comparison_mode
)
Sampler( wrapping::NTuple{N, E_WrapModes},
         filter,
         anisotropy = @f32(1),
         mip_filter = filter,
         mip_offset = @f32(0),
         mip_range = Box_minmax(UInt(1), UInt(1001))
       ) where {N} = Sampler{N}(
    wrapping, filter, mip_filter,
    anisotropy,
    mip_offset, mip_range,
    depth_comparison_mode
)

# Convert a sampler to a different-dimensional one.
# Fill in the extra dimensions with a constant pre-chosen value,
#    to make it easier to use 3D samplers as dictionary keys.
Base.convert(::Type{Sampler{N2}}, s::Sampler{N1}) where {N1, N2} = Sampler{N2}(
    Vec(ntuple(i -> s.wrapping[i], Val(min(N1, N2)))...,
        ntuple(i -> WrapModes.repeat, Val(max(0, N2 - N1)))...),
    s.pixel_filter, s.mip_filter,
    s.anisotropy, s.mip_offset, s.mip_range,
    s.depth_comparison_mode
)

"Gets a sampler's wrapping mode across all axes, assuming they're all the same"
@inline function get_wrapping(s::Sampler)
    @bp_gl_assert(all(w-> w==s.wrapping[1], s.wrapping),
                  "Sampler's wrapping setting has different values along each axis: ", s.wrapping)
    return s.wrapping[1]
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
    for i::Int in 1:N
        gl_set_func_i(ptr, wrapping_keys[i], GLint(s.wrapping[i]))
    end
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