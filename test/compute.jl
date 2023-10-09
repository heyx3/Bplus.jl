"Checks for OpenGL error messages, and prints less-severe messages"
function check_gl_logs(context::String)
    logs = pull_gl_logs()
    for log in logs
        msg = sprint(show, log)
        if log.severity in (DebugEventSeverities.high, DebugEventSeverities.medium)
            @warn "$context. $msg"
            println("Stacktrace:\n--------------------")
            display(stacktrace())
            println("-------------------\n\n")
        elseif log.severity == DebugEventSeverities.low
            @warn "$context. $msg"
        elseif log.severity == DebugEventSeverities.none
            @info "$context. $msg"
        else
            @error "Message, UNEXPECTED SEVERITY $(log.severity): $msg"
        end
    end
end

# Run sequences of compute operations on some data, and make sure it has the expected result.

bp_gl_context( v2i(800, 500), "Compute tests: SSBO",
               vsync=VsyncModes.off,
               debug_mode=true
             ) do context::Context
    check_gl_logs("Before doing anything")

    #  1. Generate color based on UV's.
    compute1 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        layout(std430) buffer Output {
            vec2 pixels[];
        };

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = gl_NumWorkGroups.xy * gl_WorkGroupSize.xy;
            vec2 uv = vec2(pixel) / vec2(totalSize);

            uint idx = pixel.x + (totalSize.x * pixel.y);
            pixels[idx] = uv;
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute1")
    @bp_check(compute1.compute_work_group_size == v3i(8, 8, 1))
    #  2. Raise color to an exponent.
    POWERS = Float32.((1.2, 0.4, 1.5, 0.9))
    compute2 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        layout(std430) buffer InOut {
            vec2 pixels[];
        };
        uniform float Power = 2.0;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = uvec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);
            uint idx = pixel.x  + (totalSize.x * pixel.y);
            pixels[idx] = pow(pixels[idx], vec2(Power));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute2")
    @bp_check(compute2.compute_work_group_size == v3i(8, 8, 1))
    #  3. Multiply by a "work group uv" value.
    compute3 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        layout(std430) buffer InOut {
            vec2 pixels[];
        };

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = uvec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);
            uint idx = pixel.x + (totalSize.x * pixel.y);

            vec2 groupUV = gl_LocalInvocationID.xy / vec2(gl_WorkGroupSize.xy);
            pixels[idx] *= groupUV;
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute3")
    @bp_check(compute3.compute_work_group_size == v3i(8, 8, 1))

    # We're going to execute the above compute shaders, and a CPU equivalent,
    #    then check that they line up.
    GROUP_SIZE = v2u(8, 8)
    RESOLUTION = GROUP_SIZE * v2u(5, 8)
    zero_based_idcs = 0 : convert(v2u, RESOLUTION - 1)
    array = fill(zero(v2f), (RESOLUTION.data...))
    buf = Buffer(false, fill(-one(v2f), (prod(RESOLUTION), )))
    check_gl_logs("After creating compute Buffer")
    function run_tests(context_msg...)
        gl_catch_up_before(MemoryActions.texture_upload_or_download)
        expected::Vector{v2f} = reshape(array, (prod(RESOLUTION), ))
        actual::Vector{v2f} = get_buffer_data(buf, v2f)
        check_gl_logs("After reading compute buffer $(context_msg...)")
        axis_difference::Vector{v2f} = abs.(expected .- actual)
        total_difference::Vector{Float32} = vlength.(axis_difference)
        MAX_ERROR = @f32(0.001)
        biggest_error = maximum(total_difference)
        if biggest_error >= MAX_ERROR
            # Grab a few representative samples.
            # Use a spacing that doesn't line up with work-group size.
            logging_pixel_poses = UInt32(0):(GROUP_SIZE+UInt32(3)):(RESOLUTION-UInt32(1))
            logging_pixel_idcs = map(logging_pixel_poses) do pos
                pos.x + (pos.y * RESOLUTION.x)
            end
            logging_global_uv = map(logging_pixel_poses) do pos
                pos / convert(v2f, RESOLUTION)
            end
            logging_powers_1 = map(logging_global_uv) do uv
                uv ^ POWERS[1]
            end
            logging_powers_2 = map(logging_powers_1) do p
                p ^ POWERS[2]
            end
            logging_powers_3 = map(logging_powers_2) do p
                p ^ POWERS[3]
            end
            logging_powers_4 = map(logging_powers_3) do p
                p ^ POWERS[4]
            end
            logging_workgroup_uv = map(logging_pixel_poses) do pos
                (pos % GROUP_SIZE) / convert(v2f, GROUP_SIZE)
            end
            logging_calculated = map(*, logging_powers_4, logging_workgroup_uv)
            logging_expected = getindex.(Ref(expected), logging_pixel_idcs .+ 1)
            logging_actual = getindex.(Ref(actual), logging_pixel_idcs .+ 1)
            data_msg = "Log: <\n"
            for (pos, idx, g_uv, p1, p2, p3, p4, w_uv, c, e, a) in zip(logging_pixel_poses,
                                                                       logging_pixel_idcs,
                                                                       logging_global_uv,
                                                                       logging_powers_1,
                                                                       logging_powers_2,
                                                                       logging_powers_3,
                                                                       logging_powers_4,
                                                                       logging_workgroup_uv,
                                                                       logging_calculated,
                                                                       logging_expected,
                                                                       logging_actual)
                data_msg *= "\tPixel (0-based):$pos {\n"
                    data_msg *= "\t\tIdx: $idx\n"
                    data_msg *= "\t\tGlobal UV: $g_uv\n"
                    data_msg *= "\t\tP1: $p1\n"
                    data_msg *= "\t\tP2: $p2\n"
                    data_msg *= "\t\tP3: $p3\n"
                    data_msg *= "\t\tP4: $p4\n"
                    data_msg *= "\t\tWorkgroup UV: $w_uv\n"
                    data_msg *= "\t\tFinal Calculated: $c\n"
                    data_msg *= "\t\tCurrent CPU: $e\n"
                    data_msg *= "\t\tCurrent GPU: $a\n"
                data_msg *= "\t}\n"
            end
            data_msg *= ">\n"

            data_msg *= "Flat run: [ "
            data_msg *= join(actual[1:11], ", ")
            data_msg *= "]"

            error("SSBO compute tests: ", context_msg..., ". Compute shader output has a max error of ", biggest_error, "! ",
                  "There are ", count(e -> e>=MAX_ERROR, total_difference), " / ",
                    prod(RESOLUTION), " pixels which have this much error. Resolution ", RESOLUTION,
                  "\nBig log is below:\n", data_msg, "\n\n")
        end
    end

    #  1:
    #   a. CPU array:
    texel = @f32(1) / vsize(array)
    map!(pixel::Vec2 -> (pixel * texel)::v2f, array, zero_based_idcs)
    #   b. GPU buffer:
    set_storage_block(buf, 1)
        check_gl_logs("After setting SSBO slot 1")
    set_storage_block(compute1, "Output", 1)
        check_gl_logs("After pointing compute1 to SSBO slot 1")
    gl_catch_up_before(MemoryActions.buffer_storage)
    dispatch_compute_threads(compute1, vappend(RESOLUTION, 1))
        check_gl_logs("After running compute1")
    set_storage_block(1)
       check_gl_logs("After clearing compute1 SSBO slot")
    check_gl_logs("After running compute1")
    run_tests("After compute1")

    #  2:
    for power in POWERS
        #   a. CPU array:
        array .^= power
        #   b. GPU buffer:
        set_storage_block(buf, 3)
        set_storage_block(compute2, "InOut", 3)
        set_uniform(compute2, "Power", power)
        gl_catch_up_before(MemoryActions.buffer_storage)
        dispatch_compute_threads(compute2, vappend(RESOLUTION, 1))
        set_storage_block(3)
        check_gl_logs("After running compute2 with $power")
        run_tests("After compute2($power)")
    end

    #  3:
    #   a. CPU array:
    map!(array, zero_based_idcs) do pixel_idx
        work_group_uv = (pixel_idx % GROUP_SIZE) / convert(v2f, GROUP_SIZE)
        return array[pixel_idx + 1] * work_group_uv
    end
    #   b. GPU buffer:
    set_storage_block(buf, 2)
    set_storage_block(compute3, "InOut", 2)
    gl_catch_up_before(MemoryActions.buffer_storage)
    dispatch_compute_threads(compute3, vappend(RESOLUTION, 1))
    set_storage_block(2)
    check_gl_logs("After running compute3")
    run_tests("After compute3")
end

# Run a sequence of compute operations on a 3D texture.
bp_gl_context( v2i(800, 500), "Compute tests: 3D layered Image View",
               vsync=VsyncModes.off,
               debug_mode=true
             ) do context::Context
    check_gl_logs("Before doing anything")

    GROUP_SIZE = v3u(8, 4, 2)
    RESOLUTION = GROUP_SIZE * v3u(5, 8, 1)
    zero_based_idcs = UInt32(0) : (RESOLUTION - UInt32(1))

    #  1. Generate color based on global and local UV's.
    cpu_compute1(idcs_zero) = let arr = fill(zero(v4f), RESOLUTION...)
        map!(arr, idcs_zero) do pixel
            global_uv = convert(v3f, pixel) / convert(v3f, RESOLUTION)
            local_uv = (pixel % GROUP_SIZE) / convert(v3f, GROUP_SIZE)
            return convert(v4f,
                lerp(-1, 1, vappend(global_uv.xz, local_uv.yz))
            )
        end
        arr
    end
    compute1 = Program("
        layout(local_size_x = 8, local_size_y = 4, local_size_z = 2) in;
        uniform layout(rgba16_snorm) writeonly image3D Output;

        void main() {
            uvec3 pixel = gl_GlobalInvocationID;
            uvec3 totalSize = gl_NumWorkGroups * gl_WorkGroupSize;
            vec3 globalUV = vec3(pixel) / vec3(totalSize);
            vec3 localUV = gl_LocalInvocationID / vec3(gl_WorkGroupSize.xyz);
                             //NOTE: Nvidia driver's shader compiler seems to crash
                             //   when I don't add '.xyz' to gl_WorkGroupSize in the denominator.

            imageStore(Output, ivec3(pixel),
                       mix(vec4(-1, -1, -1, -1),
                           vec4(1, 1, 1, 1),
                           vec4(globalUV.xz, localUV.yz)));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute1")
    @bp_check(compute1.compute_work_group_size == v3i(8, 4, 2))

    #  2. Pass through a triangle wave (sine waves are simpler but prone to numeric error).
    PERIOD = v4f(1, 2, 3, 4)
    cpu_compute2(array, idcs_zero) = map!(array, idcs_zero) do pixel
        weighted_value = array[pixel + 1] / convert(v4f, PERIOD)
        triangle_wave_t = 2 * abs(weighted_value - floor(weighted_value + 0.5))
        return convert(v4f, lerp(-1, 1, triangle_wave_t))
    end
    compute2 = Program("
        layout(local_size_x = 8, local_size_y = 4, local_size_z = 2) in;
        uniform vec4 Period;
        uniform layout(rgba16_snorm) image3D InOut;

        void main() {
            uvec3 pixel = gl_GlobalInvocationID;
            uvec3 totalSize = gl_NumWorkGroups * gl_WorkGroupSize;

            vec4 imgValue = imageLoad(InOut, ivec3(pixel));

            vec4 weightedValue = imgValue / Period;
            vec4 triangleWaveT = 2 * abs(weightedValue - floor(weightedValue + 0.5));
            vec4 outValue = mix(vec4(-1, -1, -1, -1),
                                vec4(1, 1, 1, 1),
                                triangleWaveT);
            imageStore(InOut, ivec3(pixel), outValue);
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute2")
    @bp_check(compute2.compute_work_group_size == v3i(8, 4, 2))

    #  3. Scale and fract() the data.
    SCALE = v4f(1.5, 3.8, -1.2, -4.6) #NOTE: Large values magnify the error from the 16bit_snorm format,
                                      #  so keep them small
    cpu_compute3(array, idcs_zero) = map!(array, idcs_zero) do pixel
        return fract(array[pixel + 1] * SCALE)
    end
    compute3 = Program("
        layout(local_size_x = 8, local_size_y = 4, local_size_z = 2) in;
        uniform layout(rgba16_snorm) image3D InOut;

        void main() {
            uvec3 pixel = gl_GlobalInvocationID;
            uvec3 totalSize = gl_NumWorkGroups * gl_WorkGroupSize;

            vec4 imgValue = imageLoad(InOut, ivec3(pixel));
            #define SCALE vec4( $(join(SCALE, ", ")) )
            imageStore(InOut, ivec3(pixel), fract(imgValue * SCALE));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute3")
    @bp_check(compute3.compute_work_group_size == v3i(8, 4, 2))

    # We're going to execute the above compute shaders, and a CPU equivalent,
    #    then check that they line up.
    tex = Texture(
        SimpleFormat(
            FormatTypes.normalized_int,
            SimpleFormatComponents.RGBA,
            SimpleFormatBitDepths.B16
        ),
        fill(-one(v4f), RESOLUTION...)
    )
    check_gl_logs("After creating texture")
    tex_view = get_view(tex, SimpleViewParams())
    view_activate(tex_view)
    check_gl_logs("After creating/activating texture view")
    function run_tests(context_msg...)
        gl_catch_up_before(MemoryActions.texture_upload_or_download)
        expected = array
        actual = fill(zero(v4f), RESOLUTION...)
        get_tex_color(tex, actual)
        check_gl_logs("After reading image $(context_msg...)")
        axis_difference::Array{v4f} = abs.(expected .- actual)
        total_difference::Array{Float32} = max.(axis_difference)
        MAX_ERROR = @f32(0.01) # Large error margins due to 16_Snorm format
        biggest_error = maximum(total_difference)
        if biggest_error >= MAX_ERROR
            Bplus.Math.VEC_N_DIGITS = 5

            # Grab a few representative samples.
            # Use a spacing that doesn't line up with work-group size.
            logging_pixel_poses = UInt32(0):(GROUP_SIZE+UInt32(3)):(RESOLUTION-UInt32(1))
            logging_pixel_idcs = map(p -> vindex(p+1, RESOLUTION), logging_pixel_poses)
            logging_output1 = cpu_compute1(zero_based_idcs)
                logging_output1 = getindex.(Ref(logging_output1), logging_pixel_idcs)
            logging_output2 = copy(logging_output1); cpu_compute2(logging_output2, 0:length(logging_output2)-1)
            logging_calculated = copy(logging_output2); cpu_compute3(logging_calculated, 0:length(logging_calculated)-1)
            logging_expected = getindex.(Ref(expected), logging_pixel_idcs)
            logging_actual = getindex.(Ref(actual), logging_pixel_idcs)
            data_msg = "Log: <\n"
            for (pos, o1, o2, c, e, a) in zip(logging_pixel_poses,
                                              logging_output1,
                                              logging_output2,
                                              logging_calculated,
                                              logging_expected,
                                              logging_actual
                                             )
                data_msg *= "\tPixel (0-based):$pos {\n"
                    data_msg *= "\t\tO1: $o1\n"
                    data_msg *= "\t\tO2: $o2\n"
                    data_msg *= "\t\tFinal Calculated: $c\n"
                    data_msg *= "\t\tCurrent CPU: $e\n"
                    data_msg *= "\t\tCurrent GPU: $a\n"
                data_msg *= "\t}\n"
            end
            data_msg *= ">\n"

            data_msg *= "Flat run: [ "
            data_msg *= join(actual[1:11], ", ")
            data_msg *= "]\n"

            data_msg *= "Corners: [\n"
            for corner::NTuple{3, Int} in Iterators.product((0, 1), (0, 1), (0, 1))
                corner = corner .* RESOLUTION.data
                corner = map(i -> (i==0) ? 1 : i, corner)
                idx = vindex(Vec(corner), RESOLUTION)
                data_msg *= "\t$(Vec(corner...))  :  Expected $(expected[idx])  :  Got $(actual[idx])\n"
            end
            data_msg *= "]"
            error("Layered 3D image compute tests: ",  context_msg..., ". Compute shader output has a max error of ", biggest_error, "! ",
                  "There are ", count(e -> e>=MAX_ERROR, total_difference), " / ",
                    prod(RESOLUTION), " pixels which have this much error. Resolution ", RESOLUTION,
                  "\nBig log is below:\n", data_msg, "\n\n")
        end
    end

    # Execute step 1.
    array = cpu_compute1(zero_based_idcs)
    set_uniform(compute1, "Output", tex_view)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute1, RESOLUTION)
        check_gl_logs("After running compute1")
    run_tests("After compute1")

    # Execute step 2.
    cpu_compute2(array, zero_based_idcs)
    set_uniform(compute2, "InOut", tex_view)
    set_uniform(compute2, "Period", PERIOD)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute2, RESOLUTION)
        check_gl_logs("After running compute2")
    run_tests("After compute2")

    # Execute step 3.
    cpu_compute3(array, zero_based_idcs)
    set_uniform(compute3, "InOut", tex_view)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute3, RESOLUTION)
        check_gl_logs("After running compute3")
    run_tests("After compute3")
end

# Run a sequence of compute operations on one face of a cubemap texture.
bp_gl_context( v2i(800, 500), "Compute tests: 3D non-layered Image View",
               vsync=VsyncModes.off,
               debug_mode=true
             ) do context::Context
    check_gl_logs("Before doing anything")

    GROUP_SIZE = 8
    GROUP_SIZE_2D = Vec(GROUP_SIZE, GROUP_SIZE)
    RESOLUTION = GROUP_SIZE * 4
    RESOLUTION_2D = Vec(RESOLUTION, RESOLUTION)
    zero_based_idcs = UInt32(0) : vappend(RESOLUTION_2D - UInt32(1), UInt32(0))

    #  1. Generate color based on global and local UV's.
    cpu_compute1(idcs_zero) = let arr = fill(v4f(i->NaN32), RESOLUTION_2D..., 1)
        map!(arr, idcs_zero) do _pixel
            pixel = _pixel.xy
            global_uv = convert(v2f, pixel) / convert(v2f, RESOLUTION_2D)
            local_uv = (pixel % GROUP_SIZE_2D) / convert(v2f, GROUP_SIZE_2D)
            return convert(v4f,
                lerp(-1, 1, vappend(global_uv, local_uv))
            )
        end
        arr
    end
    compute1 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) writeonly image2D Output;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = gl_NumWorkGroups.xy * uvec2($GROUP_SIZE, $GROUP_SIZE);
            vec2 globalUV = vec2(pixel) / vec2($RESOLUTION, $RESOLUTION);
            vec2 localUV = gl_LocalInvocationID.xy / vec2($GROUP_SIZE, $GROUP_SIZE);
                             //NOTE: Nvidia driver's shader compiler seems to crash
                             //   when I don't add '.xy' to gl_WorkGroupSize in the denominator.

            imageStore(Output, ivec2(pixel),
                       mix(vec4(-1, -1, -1, -1),
                           vec4(1, 1, 1, 1),
                           vec4(globalUV, localUV)));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute1")
    @bp_check(compute1.compute_work_group_size == v3i(8, 8, 1))

    #  2. Pass through a triangle wave (sine waves are simpler but prone to numeric error).
    PERIOD = v4f(1.234, 4.253, 8.532, -4.4368)
    cpu_compute2(array, idcs_zero) = map!(array, idcs_zero) do _pixel
        pixel = _pixel.xy
        weighted_value = array[pixel + 1] / PERIOD
        triangle_wave_t = 2 * abs(weighted_value - floor(weighted_value + 0.5))
        return convert(v4f, lerp(-1, 1, triangle_wave_t))
    end
    compute2 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) image2D InOut;
        uniform vec4 Period;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = gl_NumWorkGroups.xy * uvec2($GROUP_SIZE, $GROUP_SIZE);

            vec4 imgValue = imageLoad(InOut, ivec2(pixel));

            vec4 weightedValue = imgValue / Period;
            vec4 triangleWaveT = 2 * abs(weightedValue - floor(weightedValue + 0.5));
            vec4 outValue = mix(vec4(-1, -1, -1, -1),
                                vec4(1, 1, 1, 1),
                                triangleWaveT);
            imageStore(InOut, ivec2(pixel), outValue);
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute2")
    @bp_check(compute2.compute_work_group_size == v3i(8, 8, 1))

    #  3. Scale and fract() the data.
    SCALE = v4f(13.5, 31.8, -16.2, -48.6)
    cpu_compute3(array, idcs_zero) = map!(array, idcs_zero) do _pixel
        pixel = _pixel.xy
        return fract(array[pixel + 1] * SCALE)
    end
    compute3 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) image2D InOut;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = gl_NumWorkGroups.xy * gl_WorkGroupSize.xy;

            vec4 imgValue = imageLoad(InOut, ivec2(pixel));
            #define SCALE vec4( $(join(SCALE, ", ")) )
            imageStore(InOut, ivec2(pixel), fract(imgValue * SCALE));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute3")
    @bp_check(compute3.compute_work_group_size == v3i(8, 8, 1))

    # We're going to execute the above compute shaders, and a CPU equivalent,
    #    then check that they line up.
    tex = Texture(
        SimpleFormat(
            FormatTypes.float,
            SimpleFormatComponents.RGBA,
            SimpleFormatBitDepths.B32
        ),
        vappend(RESOLUTION_2D, 256)
    )
    check_gl_logs("After creating texture")
    Z_SLICE = 3
    tex_view = get_view(tex, SimpleViewParams(
        layer = Z_SLICE
    ))
    view_activate(tex_view)
    check_gl_logs("After creating/activating texture view")
    function run_tests(context_msg...)
        view_deactivate(tex_view)
        gl_catch_up_before(MemoryActions.texture_upload_or_download)
        expected::Array{v4f} = array
        actual::Array{v4f} = fill(v4f(i->NaN32), RESOLUTION_2D..., 1)
        get_tex_color(tex, actual, TexSubset(Box(
            min = v3u(1, 1, Z_SLICE),
            size = v3u(tex.size.xy..., 1)
        )))
        check_gl_logs("After reading image $(context_msg...)")
        axis_difference::Array{v4f} = abs.(expected .- actual)
        total_difference::Array{Float32} = max.(axis_difference)
        MAX_ERROR = @f32(0.001)
        biggest_error = maximum(total_difference)
        if biggest_error >= MAX_ERROR
            Bplus.Math.VEC_N_DIGITS = 5

            # Grab a few representative samples.
            # Use a spacing that doesn't line up with work-group size.
            logging_pixel_poses = v2u(0, 0, Z_SLICE):vappend(GROUP_SIZE_2D+UInt32(3), UInt32(1)):vappend(RESOLUTION_2D-UInt32(1), Z_SLICE)
            logging_pixel_idcs = map(p -> vindex(p+1, vappend(RESOLUTION_2D, 1)), logging_pixel_poses)
            logging_output1 = cpu_compute1(zero_based_idcs)
                logging_output1 = getindex.(Ref(logging_output1), logging_pixel_idcs)
            logging_output2 = copy(logging_output1); cpu_compute2(logging_output2, 0:length(logging_output2)-1)
            logging_calculated = copy(logging_output2); cpu_compute3(logging_calculated, 0:length(logging_calculated)-1)
            logging_expected = getindex.(Ref(expected), logging_pixel_idcs)
            logging_actual = getindex.(Ref(actual), logging_pixel_idcs)
            data_msg = "Log: <\n"
            for (pos, o1, o2, c, e, a) in zip(logging_pixel_poses,
                                              logging_output1,
                                              logging_output2,
                                              logging_calculated,
                                              logging_expected,
                                              logging_actual
                                             )
                data_msg *= "\tPixel (0-based):$pos {\n"
                    data_msg *= "\t\tO1: $o1\n"
                    data_msg *= "\t\tO2: $o2\n"
                    data_msg *= "\t\tFinal Calculated: $c\n"
                    data_msg *= "\t\tCurrent CPU: $e\n"
                    data_msg *= "\t\tCurrent GPU: $a\n"
                data_msg *= "\t}\n"
            end
            data_msg *= ">\n"

            data_msg *= "Flat run: [ "
            data_msg *= join(actual[1:11], ", ")
            data_msg *= "]\n"

            data_msg *= "Corners: [\n"
            for corner::NTuple{2, Int} in Iterators.product((0, 1), (0, 1))
                corner = corner .* RESOLUTION_2D.data
                corner = map(i -> (i==0) ? 1 : i, corner)
                idx = vindex(Vec(corner), RESOLUTION_2D)
                data_msg *= "\t$(Vec(corner...))  :  Expected $(expected[idx])  :  Got $(actual[idx])\n"
            end
            data_msg *= "]\n"

            error("Non-layered 3D compute tests: ", context_msg..., ". Compute shader output has a max error of ", biggest_error, "! ",
                  "There are ", count(e -> e>=MAX_ERROR, total_difference), " / ",
                    prod(RESOLUTION_2D), " pixels which have this much error. Resolution ", RESOLUTION_2D,
                  "\nBig log is below:\n", data_msg, "\n\n")
        else
            view_activate(tex_view)
        end
    end

    # Execute step 1.
    array = cpu_compute1(zero_based_idcs)
    set_uniform(compute1, "Output", tex_view)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute1, vappend(RESOLUTION_2D, 1))
        check_gl_logs("After running compute1")
    run_tests("After compute1")

    # Execute step 2.
    cpu_compute2(array, zero_based_idcs)
    set_uniform(compute2, "InOut", tex_view)
    set_uniform(compute2, "Period", PERIOD)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute2, vappend(RESOLUTION_2D, 1))
        check_gl_logs("After running compute2")
    run_tests("After compute2")

    # Execute step 3.
    cpu_compute3(array, zero_based_idcs)
    set_uniform(compute3, "InOut", tex_view)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute3, vappend(RESOLUTION_2D, 1))
        check_gl_logs("After running compute3")
    run_tests("After compute3")
end

# Run a sequence of compute operations on all faces of a cubemap texture.
bp_gl_context( v2i(800, 500), "Compute tests: Cubemap layered Image View",
               vsync=VsyncModes.off,
               debug_mode=true
             ) do context::Context
    check_gl_logs("Before doing anything")

    GROUP_SIZE = 8
    GROUP_SIZE_3D = Vec(GROUP_SIZE, GROUP_SIZE, 1)
    RESOLUTION = GROUP_SIZE * 4
    RESOLUTION_2D = Vec(RESOLUTION, RESOLUTION)
    RESOLUTION_3D = Vec(RESOLUTION, RESOLUTION, 6)
    zero_based_idcs = UInt32(0) : (RESOLUTION_3D - UInt32(1))

    #  1. Generate color based on global and local UV's.
    cpu_compute1(idcs_zero) = let arr = fill(zero(v4f), RESOLUTION_3D...)
        map!(arr, idcs_zero) do pixel
            global_uv = convert(v3f, pixel) / convert(v3f, RESOLUTION_3D)
            local_uv = (pixel % GROUP_SIZE_3D) / convert(v3f, GROUP_SIZE_3D)
            return convert(v4f,
                lerp(-1, 1, vappend(global_uv.xz, local_uv.yz))
            )
        end
        arr
    end
    compute1 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) writeonly imageCube Output;

        void main() {
            uvec3 pixel = gl_GlobalInvocationID.xyz;
            uvec3 totalSize = gl_NumWorkGroups.xyz * gl_WorkGroupSize.xyz;
            vec3 globalUV = vec3(pixel) / vec3(totalSize);
            vec3 localUV = gl_LocalInvocationID.xyz / vec3(gl_WorkGroupSize.xyz);
                             //NOTE: Nvidia driver's shader compiler seems to crash
                             //   when I don't add '.xyz' to gl_WorkGroupSize in the denominator.

            imageStore(Output, ivec3(pixel),
                       mix(vec4(-1, -1, -1, -1),
                           vec4(1, 1, 1, 1),
                           vec4(globalUV.xz, localUV.yz)));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute1")
    @bp_check(compute1.compute_work_group_size == GROUP_SIZE_3D)

    #  2. Pass through a triangle wave (sine waves are simpler but prone to numeric error).
    PERIOD = v4f(2.2, 3.8, 5.8, 9.2)
    cpu_compute2(array, idcs_zero) = map!(array, idcs_zero) do pixel
        weighted_value = array[pixel + 1] / convert(v4f, PERIOD)
        triangle_wave_t = 2 * abs(weighted_value - floor(weighted_value + 0.5))
        return convert(v4f, lerp(-1, 1, triangle_wave_t))
    end
    compute2 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) imageCube InOut;
        uniform vec4 Period;

        void main() {
            uvec3 pixel = gl_GlobalInvocationID.xyz;
            uvec3 totalSize = gl_NumWorkGroups.xyz * gl_WorkGroupSize.xyz;

            vec4 imgValue = imageLoad(InOut, ivec3(pixel));

            vec4 weightedValue = imgValue / Period;
            vec4 triangleWaveT = 2 * abs(weightedValue - floor(weightedValue + 0.5));
            vec4 outValue = mix(vec4(-1, -1, -1, -1),
                                vec4(1, 1, 1, 1),
                                triangleWaveT);
            imageStore(InOut, ivec3(pixel), outValue);
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute2")
    @bp_check(compute2.compute_work_group_size == GROUP_SIZE_3D)

    #  3. Scale and fract() the data.
    SCALE = v4f(13.5, 31.8, -16.2, -48.6)
    cpu_compute3(array, idcs_zero) = map!(array, idcs_zero) do pixel
        return fract(array[pixel + 1] * SCALE)
    end
    compute3 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) imageCube InOut;

        void main() {
            uvec3 pixel = gl_GlobalInvocationID.xyz;
            uvec3 totalSize = gl_NumWorkGroups.xyz * gl_WorkGroupSize.xyz;

            vec4 imgValue = imageLoad(InOut, ivec3(pixel));
            #define SCALE vec4( $(join(SCALE, ", ")) )
            imageStore(InOut, ivec3(pixel), fract(imgValue * SCALE));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute3")
    @bp_check(compute3.compute_work_group_size == GROUP_SIZE_3D)

    # We're going to execute the above compute shaders, and a CPU equivalent,
    #    then check that they line up.
    tex = Texture_cube(
        SimpleFormat(
            FormatTypes.float,
            SimpleFormatComponents.RGBA,
            SimpleFormatBitDepths.B32
        ),
        RESOLUTION
    )
    check_gl_logs("After creating texture")
    tex_view = get_view(tex, SimpleViewParams())
    view_activate(tex_view)
    check_gl_logs("After creating/activating texture view")
    function run_tests(context_msg...)
        view_deactivate(tex_view)
        gl_catch_up_before(MemoryActions.texture_upload_or_download)
        expected = array
        actual = fill(v4f(i->NaN32), RESOLUTION_3D...)
        get_tex_color(tex, actual)
        check_gl_logs("After reading image $(context_msg...)")
        axis_difference::Array{v4f} = abs.(expected .- actual)
        total_difference::Array{Float32} = max.(axis_difference)
        MAX_ERROR = @f32(0.001)
        biggest_error = maximum(total_difference)
        if biggest_error >= MAX_ERROR
            Bplus.Math.VEC_N_DIGITS = 5

            # Grab a few representative samples.
            # Use a spacing that doesn't line up with work-group size.
            logging_pixel_poses = UInt32(0):(GROUP_SIZE_3D+v3u(3, 3, 0)):(RESOLUTION_3D-UInt32(1))
            logging_pixel_idcs = map(p -> vindex(p+1, RESOLUTION_3D), logging_pixel_poses)
            logging_output1 = cpu_compute1(zero_based_idcs)
                logging_output1 = getindex.(Ref(logging_output1), logging_pixel_idcs)
            logging_output2 = copy(logging_output1); cpu_compute2(logging_output2, 0:length(logging_output2)-1)
            logging_calculated = copy(logging_output2); cpu_compute3(logging_calculated, 0:length(logging_calculated)-1)
            logging_expected = getindex.(Ref(expected), logging_pixel_idcs)
            logging_actual = getindex.(Ref(actual), logging_pixel_idcs)
            data_msg = "Log: <\n"
            for (pos, o1, o2, c, e, a) in zip(logging_pixel_poses,
                                              logging_output1,
                                              logging_output2,
                                              logging_calculated,
                                              logging_expected,
                                              logging_actual
                                             )
                data_msg *= "\tPixel (0-based):$pos {\n"
                    data_msg *= "\t\tO1: $o1\n"
                    data_msg *= "\t\tO2: $o2\n"
                    data_msg *= "\t\tFinal Calculated: $c\n"
                    data_msg *= "\t\tCurrent CPU: $e\n"
                    data_msg *= "\t\tCurrent GPU: $a\n"
                data_msg *= "\t}\n"
            end
            data_msg *= ">\n"

            data_msg *= "Flat run: [ "
            data_msg *= join(actual[1:11], ", ")
            data_msg *= "]\n"

            data_msg *= "Corners: [\n"
            for corner::NTuple{3, Int} in Iterators.product((0, 1), (0, 1), (0, 1))
                corner = corner .* RESOLUTION_3D.data
                corner = map(i -> (i==0) ? 1 : i, corner)
                idx = vindex(Vec(corner), RESOLUTION_3D)
                data_msg *= "\t$(Vec(corner...))  :  Expected $(expected[idx])  :  Got $(actual[idx])\n"
            end
            data_msg *= "]\n"

            error("Layered Cubemap image view compute tests: ", context_msg...,
                    ". Compute shader output has a max error of ", biggest_error, "! ",
                  "There are ", count(e -> e>=MAX_ERROR, total_difference), " / ",
                    prod(RESOLUTION_3D), " pixels which have this much error. Resolution ", RESOLUTION_2D,
                  "\nBig log is below:\n", data_msg, "\n\n")
        else
            view_activate(tex_view)
        end
    end

    # Execute step 1.
    array = cpu_compute1(zero_based_idcs)
    set_uniform(compute1, "Output", tex_view)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute1, RESOLUTION_3D)
        check_gl_logs("After running compute1")
    run_tests("After compute1")

    # Execute step 2.
    cpu_compute2(array, zero_based_idcs)
    set_uniform(compute2, "InOut", tex_view)
    set_uniform(compute2, "Period", PERIOD)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute2, RESOLUTION_3D)
        check_gl_logs("After running compute2")
    run_tests("After compute2")

    # Execute step 3.
    cpu_compute3(array, zero_based_idcs)
    set_uniform(compute3, "InOut", tex_view)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    dispatch_compute_threads(compute3, RESOLUTION_3D)
        check_gl_logs("After running compute3")
    run_tests("After compute3")
end

# Run a sequence of compute operations on one face of a cubemap texture.
bp_gl_context( v2i(800, 500), "Compute tests: Cubemap non-layered Image View",
               vsync=VsyncModes.off,
               debug_mode=true
             ) do context::Context
    check_gl_logs("Before doing anything")

    GROUP_SIZE = 8
    GROUP_SIZE_2D = Vec(GROUP_SIZE, GROUP_SIZE)
    RESOLUTION = GROUP_SIZE * 4
    RESOLUTION_2D = Vec(RESOLUTION, RESOLUTION)
    zero_based_idcs = UInt32(0) : (RESOLUTION_2D - UInt32(1))

    #  1. Generate color based on global and local UV's.
    cpu_compute1(idcs_zero) = let arr = fill(v4f(i->NaN32), RESOLUTION_2D...)
        map!(arr, idcs_zero) do pixel
            global_uv = convert(v2f, pixel) / convert(v2f, RESOLUTION_2D)
            local_uv = (pixel % GROUP_SIZE_2D) / convert(v2f, GROUP_SIZE_2D)
            return convert(v4f,
                lerp(v4f(-23, -6, 10, 13),
                     v4f(6, 22, -4.54645, -8.4568),
                     vappend(global_uv, local_uv))
            )
        end
        arr
    end
    compute1 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) writeonly image2D Output;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = gl_NumWorkGroups.xy * uvec2($GROUP_SIZE, $GROUP_SIZE);
            vec2 globalUV = vec2(pixel) / vec2($RESOLUTION, $RESOLUTION);
            vec2 localUV = gl_LocalInvocationID.xy / vec2($GROUP_SIZE, $GROUP_SIZE);
                             //NOTE: Nvidia driver's shader compiler seems to crash
                             //   when I don't add '.xy' to gl_WorkGroupSize in the denominator.

            imageStore(Output, ivec2(pixel),
                       mix(vec4(-23, -6, 10, 13),
                           vec4(6, 22, -4.54645, -8.4568),
                           vec4(globalUV, localUV)));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute1")
    @bp_check(compute1.compute_work_group_size == v3i(8, 8, 1))

    #  2. Pass through a triangle wave (sine waves are simpler but prone to numeric error).
    PERIOD = v4f(1.234, 4.253, 8.532, -4.4368)
    cpu_compute2(array, idcs_zero) = map!(array, idcs_zero) do pixel
        weighted_value = array[pixel + 1] / PERIOD
        triangle_wave_t = 2 * abs(weighted_value - floor(weighted_value + 0.5))
        return convert(v4f, lerp(-1, 1, triangle_wave_t))
    end
    compute2 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) image2D InOut;
        uniform vec4 Period;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = gl_NumWorkGroups.xy * uvec2($GROUP_SIZE, $GROUP_SIZE);

            vec4 imgValue = imageLoad(InOut, ivec2(pixel));

            vec4 weightedValue = imgValue / Period;
            vec4 triangleWaveT = 2 * abs(weightedValue - floor(weightedValue + 0.5));
            vec4 outValue = mix(vec4(-1, -1, -1, -1),
                                vec4(1, 1, 1, 1),
                                triangleWaveT);
            imageStore(InOut, ivec2(pixel), outValue);
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute2")
    @bp_check(compute2.compute_work_group_size == v3i(8, 8, 1))

    #  3. Scale and fract() the data.
    SCALE = v4f(13.5, 31.8, -16.2, -48.6)
    cpu_compute3(array, idcs_zero) = map!(array, idcs_zero) do pixel
        return fract(array[pixel + 1] * SCALE)
    end
    compute3 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        uniform layout(rgba32f) image2D InOut;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uvec2 totalSize = gl_NumWorkGroups.xy * gl_WorkGroupSize.xy;

            vec4 imgValue = imageLoad(InOut, ivec2(pixel));
            #define SCALE vec4( $(join(SCALE, ", ")) )
            imageStore(InOut, ivec2(pixel), fract(imgValue * SCALE));
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute3")
    @bp_check(compute3.compute_work_group_size == v3i(8, 8, 1))

    # We're going to execute the above compute shaders, and a CPU equivalent,
    #    then check that they line up.
    tex = Texture_cube(
        SimpleFormat(
            FormatTypes.float,
            SimpleFormatComponents.RGBA,
            SimpleFormatBitDepths.B32
        ),
        RESOLUTION
    )
    check_gl_logs("After creating texture")
    CUBE_FACE = 3
    tex_view = get_view(tex, SimpleViewParams(
        layer = CUBE_FACE
    ))
    view_activate(tex_view)
    check_gl_logs("After creating/activating texture view")
    function run_tests(context_msg...)
        view_deactivate(tex_view)
        gl_catch_up_before(MemoryActions.texture_upload_or_download)
        expected = array
        actual = fill(v4f(i->NaN32), RESOLUTION_2D..., 1)
        get_tex_color(tex, actual, TexSubset(Box(
            min=v3u(1, 1, CUBE_FACE),
            size=v3u(RESOLUTION_2D..., 1)
        )))
        check_gl_logs("After reading image $(context_msg...)")
        axis_difference::Array{v4f} = abs.(expected .- actual)
        total_difference::Array{Float32} = max.(axis_difference)
        MAX_ERROR = @f32(0.001)
        biggest_error = maximum(total_difference)
        if biggest_error >= MAX_ERROR
            Bplus.Math.VEC_N_DIGITS = 5

            # Grab a few representative samples.
            # Use a spacing that doesn't line up with work-group size.
            logging_pixel_poses = UInt32(0):(GROUP_SIZE_2D+UInt32(3)):(RESOLUTION_2D-UInt32(1))
            logging_pixel_idcs = map(p -> vindex(p+1, RESOLUTION_2D), logging_pixel_poses)
            logging_output1 = cpu_compute1(zero_based_idcs)
                logging_output1 = getindex.(Ref(logging_output1), logging_pixel_idcs)
            logging_output2 = copy(logging_output1); cpu_compute2(logging_output2, 0:length(logging_output2)-1)
            logging_calculated = copy(logging_output2); cpu_compute3(logging_calculated, 0:length(logging_calculated)-1)
            logging_expected = getindex.(Ref(expected), logging_pixel_idcs)
            logging_actual = getindex.(Ref(actual), logging_pixel_idcs)
            data_msg = "Log: <\n"
            for (pos, o1, o2, c, e, a) in zip(logging_pixel_poses,
                                              logging_output1,
                                              logging_output2,
                                              logging_calculated,
                                              logging_expected,
                                              logging_actual
                                             )
                data_msg *= "\tPixel (0-based):$pos {\n"
                    data_msg *= "\t\tO1: $o1\n"
                    data_msg *= "\t\tO2: $o2\n"
                    data_msg *= "\t\tFinal Calculated: $c\n"
                    data_msg *= "\t\tCurrent CPU: $e\n"
                    data_msg *= "\t\tCurrent GPU: $a\n"
                data_msg *= "\t}\n"
            end
            data_msg *= ">\n"

            data_msg *= "Flat run: [ "
            data_msg *= join(actual[1:11], ", ")
            data_msg *= "]\n"

            data_msg *= "Corners: [\n"
            for corner::NTuple{2, Int} in Iterators.product((0, 1), (0, 1))
                corner = corner .* RESOLUTION_2D.data
                corner = map(i -> (i==0) ? 1 : i, corner)
                idx = vindex(Vec(corner), RESOLUTION_2D)
                data_msg *= "\t$(Vec(corner...))  :  Expected $(expected[idx])  :  Got $(actual[idx])\n"
            end
            data_msg *= "]\n"

            error("Non-layered Cubemap compute tests: ", context_msg..., ". Compute shader output has a max error of ", biggest_error, "! ",
                  "There are ", count(e -> e>=MAX_ERROR, total_difference), " / ",
                    prod(RESOLUTION_2D), " pixels which have this much error. Resolution ", RESOLUTION_2D,
                  "\nBig log is below:\n", data_msg, "\n\n")
        else
            view_activate(tex_view)
        end
    end

    # Execute step 1.
    array = cpu_compute1(zero_based_idcs)
    gl_catch_up_before(MemoryActions.texture_simple_views)
    set_uniform(compute1, "Output", tex_view)
    dispatch_compute_threads(compute1, vappend(RESOLUTION_2D, 1))
        check_gl_logs("After running compute1")
    run_tests("After compute1")

    # Execute step 2.
    cpu_compute2(array, zero_based_idcs)
    gl_catch_up_before(MemoryActions.USE_TEXTURES) # Gets around a potential driver bug. See B+ issue #111
    set_uniform(compute2, "InOut", tex_view)
    set_uniform(compute2, "Period", PERIOD)
    dispatch_compute_threads(compute2, vappend(RESOLUTION_2D, 1))
        check_gl_logs("After running compute2")
    run_tests("After compute2")

    # Execute step 3.
    cpu_compute3(array, zero_based_idcs)
    gl_catch_up_before(MemoryActions.USE_TEXTURES)
    set_uniform(compute3, "InOut", tex_view)
    dispatch_compute_threads(compute3, vappend(RESOLUTION_2D, 1))
        check_gl_logs("After running compute3")
    run_tests("After compute3")
end