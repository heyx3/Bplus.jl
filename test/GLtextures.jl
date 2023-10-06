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
                                 expected_flattened::AbstractVector
                                 ;
                                 atol=0,
                                 single_component::E_PixelIOChannels=PixelIOChannels.red)
        subset_area::Box = Bplus.GL.get_subset_range(subset, tex.size)
        pixels_buffer = Array{eltype(expected_flattened)}(undef, size(subset_area)...)
        get_tex_pixels(tex, pixels_buffer, subset, single_component=single_component)

        actual_flattened = reshape(pixels_buffer, (length(pixels_buffer), ))
        for (idx, expected, actual) in zip(1:typemax(Int), expected_flattened, actual_flattened)
            if !isapprox(expected, actual, atol=atol, rtol=0, nans=true)
                pixel = min_inclusive(subset_area) + vindex(idx, size(subset_area)) - 1
                error("Pixel mismatch when reading texture!\n",
                        "\tTexture:\n",
                          "\t\tFormat: ", tex.format, "\n",
                          "\t\tType: ", tex.type, "\n",
                          "\t\tFullSize: ", tex.size, "\n",
                        "\tSubset:\n",
                          "\t\tMin: ", min_inclusive(subset_area), "\n",
                          "\t\tMax: ", max_inclusive(subset_area), "\n",
                          "\t\tSize: ", size(subset_area), "\n",
                        "\tPixel: ", pixel, "\n",
                          "\t\tArray index: ", idx, "\n",
                        "\tExpected: ", expected, "\n",
                        "\tActual  : ", actual, "\n",
                        "\tDelta to Expected: ", expected-actual)
            end
        end
    end
    function try_setting_texture(tex::Texture, subset::TexSubset,
                                 data::Array,
                                 expected_data::Array = data
                                 ;
                                 atol=0,
                                 single_component::E_PixelIOChannels=PixelIOChannels.red)
        set_tex_color(tex, data, subset, single_component=single_component)
        try_getting_texture(
            tex,
            subset,
            reshape(expected_data, prod(size(expected_data))),
            atol=atol,
            single_component=single_component
        )
    end

    # Pixel data converters that match the OpenGL standard:
    println("#TODO: Move these into B+ proper")
    ogl_pixel_convert(T::Type{<:AbstractFloat}, u::Unsigned) = convert(T, u / typemax(u))
    ogl_pixel_convert(T::Type{<:AbstractFloat}, i::Signed) =
        #NOTE: The correct function according to the OpenGL spec should be this:
        #        max(-one(T), convert(T, i / typemax(i)))
        #      However, in practice that doesn't seem to work.
        #      Is the driver violating spec here?
        #      Or is the read back into float resulting in a different number than was passed in originally?
        lerp(-one(T), one(T),
             inv_lerp(convert(T, typemin(i)),
                      convert(T, typemax(i)),
                      i))
    ogl_pixel_convert(T::Type{<:Unsigned}, f::AbstractFloat) = round(T, saturate(f) * typemax(T))
    ogl_pixel_convert(T::Type{<:Signed}, f::AbstractFloat) = begin
        f = clamp(f, -one(typeof(f)), one(typeof(f)))

        fi = f * typemax(T)
        fi = clamp(fi,
                   convert(typeof(f), typemin(T)),
                   convert(typeof(f), typemax(T)))

        return round(T, fi)
    end
    ogl_pixel_convert(T::Type{Vec{N, F}}, v::Vec{N}) where {N, F} = map(f -> ogl_pixel_convert(F, f), v)

    TEX_SIZE_3D = v3u(3, 4, 5)
    TEX_SIZE_CUBE = TEX_SIZE_3D.x
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
        TEX_SIZE_CUBE
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
    tex_cube_rg16in = Texture_cube(
        SimpleFormat(
            FormatTypes.normalized_int,
            SimpleFormatComponents.RG,
            SimpleFormatBitDepths.B16
        ),
        TEX_SIZE_CUBE
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
        TEX_SIZE_CUBE
    )
    tex_1D_d16u = Texture(
        DepthStencilFormats.depth_16u,
        TEX_SIZE_3D.x
    )
    tex_cube_d16u = Texture_cube(
        DepthStencilFormats.depth_16u,
        TEX_SIZE_CUBE
    )
    tex_1D_d32f = Texture(
        DepthStencilFormats.depth_32f,
        TEX_SIZE_3D.x
    )
    tex_cube_d32f = Texture_cube(
        DepthStencilFormats.depth_32f,
        TEX_SIZE_CUBE
    )
    tex_1D_s8u = Texture(
        DepthStencilFormats.stencil_8,
        TEX_SIZE_3D.x
    )
    tex_cube_s8u = Texture_cube(
        DepthStencilFormats.stencil_8,
        TEX_SIZE_CUBE
    )
    tex_1D_d32fs8 = Texture(
        DepthStencilFormats.depth32f_stencil8,
        TEX_SIZE_3D.x,
        depth_stencil_sampling = DepthStencilSources.depth
    )
    tex_cube_d32fs8 = Texture_cube(
        DepthStencilFormats.depth32f_stencil8,
        TEX_SIZE_CUBE,
        depth_stencil_sampling = DepthStencilSources.stencil
    )
    check_gl_logs("After creating textures")

    #IMPORTANT NOTE: IN Julia multidimensional array literals, the X and Y axes are flipped.
    # So the array [ 1 2 3 (line break) 4 5 6 ] has two elements along X and 3 along Y.
    # Z slices are separated with `;;;`.

    # R32F textures:
    #  1D:
    #    Write and read as Float32:
    try_setting_texture(
        tex_1D_r32f,
        TexSubset(Box1Du(
            min=Vec(1),
            max=Vec(3)
        )),
        Float32[ 2.4, 5.5, 6.6 ]
    )
    #    Write as UInt8, read as Float32:
    try_setting_texture(
        tex_1D_r32f,
        TexSubset(Box1Du(
            min=Vec(1),
            max=Vec(3)
        )),
        let raw = UInt8[ 128, 255, 17 ]
          (
            raw,
            ogl_pixel_convert.(Ref(Float32), raw)
          )
        end...
    )
    #    Write as Int8, read as Float32:
    try_setting_texture(
        tex_1D_r32f,
        TexSubset(Box1Du(
            min=Vec(1),
            max=Vec(3)
        )),
        let raw = Int8[ -126, 32, -17 ]
          (
            raw,
            ogl_pixel_convert.(Ref(Float32), raw)
          )
        end...,
        atol=0.000001
    )
    #    Write and read as Float32:
    try_setting_texture(
        tex_1D_r32f,
        TexSubset(Box1Du(
            min=Vec(2),
            max=Vec(2)
        )),
        Float32[ 3.3 ]
    )
    #  3D:
    #    Write and read as Float32:
    try_setting_texture(
        tex_3D_r32f,
        TexSubset(Box(
            min=v3u(2, 1, 4),
            max=v3u(2, 2, 5)
        )),
        Float32[
            1.1
            1.2
            ;;;
            1.3
            1.4
        ]
    )
    #    Write and read as Float32:
    try_setting_texture(
        tex_3D_r32f,
        TexSubset(Box(
            min=one(v3u),
            max=TEX_SIZE_3D
        )),
        Float32[
            111 121 131 141
            211 221 231 241
            311 321 331 341
            ;;;
            112 122 132 142
            212 222 232 242
            312 322 332 342
            ;;;
            113 123 133 143
            213 223 233 243
            313 323 333 343
            ;;;
            114 124 134 144
            214 224 234 244
            314 324 334 344
            ;;;
            115 125 135 145
            215 225 235 245
            315 325 335 345
        ]
    )
    #    Write as UInt16, read as Float32:
    try_setting_texture(
        tex_3D_r32f,
        TexSubset(Box(
            min=one(v3u),
            max=TEX_SIZE_3D
        )),
        let raws = UInt16[
                11100 12100 13100 14100
                21100 22100 23100 24100
                31100 32100 33100 34100
                ;;;
                11200 12200 13200 14200
                21200 22200 23200 24200
                31200 32200 33200 34200
                ;;;
                11300 12300 13300 14300
                21300 22300 23300 24300
                31300 32300 33300 34300
                ;;;
                11400 12400 13400 14400
                21400 22400 23400 24400
                31400 32400 33400 34400
                ;;;
                11500 12500 13500 14500
                21500 22500 23500 24500
                31500 32500 33500 34500
            ]
          (
            raws,
            ogl_pixel_convert.(Ref(Float32), raws)
          )
        end...
    )
    #    Write as Int16, read as Float32:
    try_setting_texture(
        tex_3D_r32f,
        TexSubset(Box(
            min=one(v3u),
            max=TEX_SIZE_3D
        )),
        let raws = Int16[
                1110 1210 1310 1410
                2110 2210 2310 2410
                -3110 -3210 -3310 -3410
                ;;;
                1120 1220 1320 1420
                -2120 -2220 -2320 -2420
                3120 3220 3320 3420
                ;;;
                -1130 -1230 -1330 -1430
                2130 2230 2330 2430
                3130 3230 3330 3430
                ;;;
                1140 1240 1340 1440
                -2140 -2240 2340 2440
                -3140 -3240 3340 3440
                ;;;
                1150 1250 -1350 -1450
                2150 2250 -2350 -2450
                3150 3250 3350 3450
            ]
          (
            raws,
            ogl_pixel_convert.(Ref(Float32), raws)
          )
        end...,
        atol=0.0000001
    )
    #  Cubemap:
    #    Write and read as Float32:
    try_setting_texture(
        tex_cube_r32f,
        TexSubset(Box(
            min=v3u(1, 2, 4),
            max=v3u(1, 3, 5)
        )),
        Float32[
            124 134
            ;;;
            125 135
        ]
    )
    #    Write and read as Float32:
    try_setting_texture(
        tex_cube_r32f,
        TexSubset(Box(
            min=v3u(1, 2, 4),
            max=v3u(1, 3, 5)
        )),
        Float32[
            124 134
            ;;;
            125 135
        ]
    )
    # RG16inorm textures:
    #  1D:
    #     Write and read Vec{2, Int16}:
    try_setting_texture(
        tex_1D_rg16in,
        TexSubset(Box(
            min=v1u(1),
            max=v1u(3)
        )),
        Vec{2, Int16}[ Vec(120, -34),
                       Vec(27, -80),
                       Vec(-125, 100) ]
    )
    #     Write Vec{2, Int16}, read back as v2f:
    try_setting_texture(
        tex_1D_rg16in,
        TexSubset(Box(
            min=v1u(1),
            max=v1u(3)
        )),
        let raws = Vec{2, Int16}[ Vec(-5, 50), Vec(27, -2), Vec(-125, 12) ]
          (
              raws,
              ogl_pixel_convert.(Ref(v2f), raws)
          )
        end...,
        atol=0.0000001
    )
    #     Write and read as v2f:
    try_setting_texture(
        tex_1D_rg16in,
        TexSubset(Box(
            min=v1u(1),
            max=v1u(3)
        )),
        let raws = [ v2f(-0.5, 0.7),
                     v2f(0.1, 0.9),
                     v2f(0.852, -0.99) ]
          (
            raws,
            ogl_pixel_convert.(Ref(v2f),
                               ogl_pixel_convert.(Ref(Vec{2, Int16}), raws))
          )
        end...,
        atol=0.00005
    )
    #  3D:
    #    Write and read as v2f:
    # try_setting_texture(
    #     tex_3D_rg16in,
    #     TexSubset(Box(
    #         min=v3u(1, 2, 3),
    #         max=v3u(2, 3, 4),
    #     )),
    #     let raws = [
    #                    v2f(1.111, 2.111) v2f(1.121, 2.121)
    #                    v2f(1.211, 2.211) v2f(1.221, 2.221)
    #                    ;;;
    #                    v2f(1.112, 2.112) v2f(1.122, 2.122)
    #                    v2f(1.212, 2.212) v2f(1.222, 2.222)
    #                ]
    #         (raws, raws)
    #     end...,
    #     atol=0.00005
    # )
    # #  Cubemap:
    # #    Write and read as v2f:
    # try_setting_texture(
    #     tex_cube_rg16in,
    #     TexSubset(Box(
    #         min=v3u(1, 2, 3),
    #         max=v3u(2, 3, 4),
    #     )),
    #     let raws = [
    #                    v2f(1.111, 2.111) v2f(1.121, 2.121)
    #                    v2f(1.211, 2.211) v2f(1.221, 2.221)
    #                    ;;;
    #                    v2f(1.112, 2.112) v2f(1.122, 2.122)
    #                    v2f(1.212, 2.212) v2f(1.222, 2.222)
    #                ]
    #         (raws, raws)
    #     end...,
    #     atol=0.00005
    # )
    # #    Write Green as Float32, then read RG values as v2f:
    # try_setting_texture(
    #     tex_cube_rg16in,
    #     TexSubset(Box(
    #         min=v3u(1, 2, 3),
    #         max=v3u(2, 3, 4)
    #     )),
    #     Float32[
    #         1 2
    #         3 4
    #         ;;;
    #         5 6
    #         7 8
    #     ],
    #     v2f[
    #         v2f(1.111, 1) v2f(1.121, 2)
    #         v2f(1.211, 3) v2f(1.221, 4)
    #         ;;;
    #         v2f(1.112, 5) v2f(1.122, 6)
    #         v2f(1.212, 7) v2f(1.222, 8)
    #     ],
    #     atol=0.00005,
    #     single_component=PixelIOChannels.green
    # )
    #TODO: Try setting Green in RG[BA] textures
    #TODO: Try setting Blue in RGB[A] textures
    #TODO: Try setting other mip levels
    #TODO: Finish with the other textures

    #TODO: Do Clear tests with `try_clearing_texture()`

    #TODO: After several clears to different subsets, get and check a texture's full pixels.

    #TODO: Test recalculation of mips after setting the top mip (OpenGL mip calc always happens using the top mip's data)
end