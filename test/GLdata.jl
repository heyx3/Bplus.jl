using Bplus.GL

# Test get_mip_size()
@bp_test_no_allocations(get_mip_size(32, 1), 32)
@bp_test_no_allocations(get_mip_size(32, 1), 32)
@bp_test_no_allocations(get_mip_size(32, 2), 16)
@bp_test_no_allocations(get_mip_size(32, 4), 4)
@bp_test_no_allocations(get_mip_size(32, 6), 1)
@bp_test_no_allocations(get_mip_size(v2i(32, 32), 1), v2i(32, 32))
@bp_test_no_allocations(get_mip_size(v2i(32, 32), 4), v2i(4, 4))
@bp_test_no_allocations(get_mip_size(v2i(32, 4), 4), v2i(4, 1))

# Test get_n_mips()
@bp_test_no_allocations(get_n_mips(1), 1)
@bp_test_no_allocations(get_n_mips(3), 2)
@bp_test_no_allocations(get_n_mips(4), 3)
@bp_test_no_allocations(get_n_mips(4), 3)
@bp_test_no_allocations(get_n_mips(v2u(129, 40)), 8)

# Test pixel buffer type data.
@bp_test_no_allocations(get_component_type(Int8), Int8)
@bp_test_no_allocations(get_component_type(PixelBufferD{3, Vec{2, Float32}}), Float32)
@bp_test_no_allocations(get_component_count(Int8), 1)
@bp_test_no_allocations(get_component_count(PixelBufferD{3, Vec{2, Float32}}), 2)

@bp_test_no_allocations(TexSampler{2}(
                            wrapping = Vec(WrapModes.clamp, WrapModes.repeat),
                            pixel_filter = PixelFilters.smooth,
                            depth_comparison_mode = ValueTests.less_than_or_equal
                        ),
                        TexSampler{2}(
                            wrapping = Vec(WrapModes.clamp, WrapModes.repeat),
                            pixel_filter = PixelFilters.smooth,
                            depth_comparison_mode = ValueTests.less_than_or_equal
                        ))
@bp_check(JSON3.read(JSON3.write(TexSampler{2}(wrapping = WrapModes.mirrored_repeat)), TexSampler{2}) ==
             TexSampler{2}(wrapping = WrapModes.mirrored_repeat))
@bp_check(JSON3.read(JSON3.write(TexSampler{2}(wrapping = Vec(WrapModes.repeat, WrapModes.mirrored_repeat))), TexSampler{2}) !=
             TexSampler{2}(wrapping = WrapModes.repeat))
@bp_check(JSON3.read("{ \"pixel_filter\":\"rough\" }", TexSampler{2}) ==
             TexSampler{2}(pixel_filter = PixelFilters.rough))

println("#TODO: Test packed depth/stencil primitive types.")