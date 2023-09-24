"Checks for OpenGL error messages, and prints less-severe messages"
function check_gl_logs(context::String)
    logs = pull_gl_logs()
    for log in logs
        msg = sprint(show, log)
        if log.severity in (DebugEventSeverities.high, DebugEventSeverities.medium)
            @error "$context. $msg"
        elseif log.severity == DebugEventSeverities.low
            @warn "$context. $msg"
        elseif log.severity == DebugEventSeverities.none
            @info "$context. $msg"
        else
            @error "Message, UNEXPECTED SEVERITY $(log.severity): $msg"
        end
    end
end

# Run a sequence of compute operations on a buffer acting like a 2D RG texture.
bp_gl_context( v2i(800, 500), "Compute tests",
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
    #  3. Add a "work group uv" value.
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

    # We're going to execute the above compute shaders, and a CPU equivalent,
    #    then check that they line up.
    GROUP_SIZE = v2u(8, 8)
    RESOLUTION = GROUP_SIZE * v2u(5, 8)
    zero_based_idcs = 0 : convert(v2u, RESOLUTION - 1)
    array = fill(zero(v2f), (RESOLUTION.data...))
    buf = Buffer(false, fill(-one(v2f), (prod(RESOLUTION), )))
    check_gl_logs("After creating compute Buffer")
    function run_tests(context_msg...)
        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT)
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
            error(context_msg...,
                  ". Compute shader output has a max error of ", biggest_error, "! ",
                  "There are ", count(e -> e>=MAX_ERROR, total_difference), " / ",
                    prod(RESOLUTION), " pixels which have this much error.",
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
    dispatch_compute_threads(compute1, vappend(vsize(array), 1))
        check_gl_logs("After running compute1")
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)
    #set_storage_block(1)
    #    check_gl_logs("After clearing compute1 SSBO slot")
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
        dispatch_compute_threads(compute2, vappend(vsize(array), 1))
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)
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
    dispatch_compute_threads(compute3, vappend(vsize(array), 1))
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)
    set_storage_block(2)
    check_gl_logs("After running compute3")
    run_tests("After compute3")
end

#TODO: Run a sequence of compute operations on an actual 2D texture.