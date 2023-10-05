bp_gl_context(v2i(800, 500), "Test: GLtextures") do context::Context
    function try_clearing_texture(tex::Texture,
                                  clear_value, clear_subset::TexSubset,
                                  read_subset::TexSubset)
        clear_tex_pixels(tex, clear_value, clear_subset)

        read_subset_area::Box = get_subset_range(read_subset, tex.size)
        pixels_buffer = Array{typeof(clear_value)}(undef, size(read_subset_area)...)
        get_tex_pixels(tex, pixels_buffer, read_subset)

        for idx in 1:vsize(pixels_buffer)
            pixel = idx + min_inclusive(read_subset_area)
            actual = pixels_buffer[idx]
            if clear_value != actual
                error("Cleared texture to ", clear_value, " (in subset ", read_subset_area, "), ",
                        "but pixel ", pixel, " (array element ", idx, "), is ", actual)
            end
        end
    end
    function try_getting_texture(tex::Texture, subset::TexSubset,
                                 expected_flattened::Vector)
        read_subset_area::Box = get_subset_range(read_subset, tex.size)
        pixels_buffer = Array{typeof(clear_value)}(undef, size(read_subset_area)...)
        get_tex_pixels(tex, pixels_buffer, read_subset)

        actual_flattened = reshape(pixels_buffer, (length(pixels_buffer), ))
        for (idx, expected, actual) in zip(1:typemax(Int), expected_flattened, actual_flattened)
            pixel = min_inclusive(read_subset_area) + vindex(idx, size(read_subset_area))
            if expected != actual
                error("Pixel ", pixel, " of texture (idx ", idx, " in subset ", subset, ") ",
                        "should be ", expected, " but was ", actual)
            end
        end
    end

    TEX_SIZE_3D = v3u(3, 4, 2)
    tex_1D_r32f = Texture(
        SimpleFormat(
            FormatTypes.float,
            SimpleFormatComponents.R,
            SimpleFormatBitDepths.B32
        ),
        TEX_SIZE_3D.x
    )
    tex_3D_r32f = Texture(
        SimpleFormat(
            FormatTypes.float,
            SimpleFormatComponents.R,
            SimpleFormatBitDepths.B32
        ),
        TEX_SIZE_3D
    )
    tex_cube_r32f = Texture_cube(
        SimpleFormat(
            FormatTypes.float,
            SimpleFormatComponents.R,
            SimpleFormatBitDepths.B32
        ),
        TEX_SIZE_3D.x
    )
    tex_1D_rg16in = Texture(
        SimpleFormat(
            FormatTypes.normalized_int,
            SimpleFormatComponents.RG,
            SimpleFormatBitDepths.B16
        ),
        TEX_SIZE_3D.x
    )
    tex_3D_rg16in = Texture(
        SimpleFormat(
            FormatTypes.normalized_int,
            SimpleFormatComponents.RG,
            SimpleFormatBitDepths.B16
        ),
        TEX_SIZE_3D
    )
    tex_3D_rg16in = Texture_cube(
        SimpleFormat(
            FormatTypes.normalized_int,
            SimpleFormatComponents.RG,
            SimpleFormatBitDepths.B16
        ),
        TEX_SIZE_3D.x
    )
    tex_1D_rgba8u = Texture(
        SimpleFormat(
            FormatTypes.uint,
            SimpleFormatComponents.RGBA,
            SimpleFormatBitDepths.B8
        ),
        TEX_SIZE_3D.x
    )
    tex_3D_rgba8u = Texture(
        SimpleFormat(
            FormatTypes.uint,
            SimpleFormatComponents.RGBA,
            SimpleFormatBitDepths.B8
        ),
        TEX_SIZE_3D
    )
    tex_cube_rgba8u = Texture_cube(
        SimpleFormat(
            FormatTypes.uint,
            SimpleFormatComponents.RGBA,
            SimpleFormatBitDepths.B8
        ),
        TEX_SIZE_3D.x
    )

    #TODO: Do Set-then-Get tests with `try_getting_texture()`

    #TODO: Do Clear tests with `try_clearing_texture()`

    #TODO: After several clears to different subsets, get and check a texture's full pixels.
end