"Checks for OpenGL error messages, and prints less-severe messages"
function check_gl_logs(context::String)
    logs = pull_gl_logs()
    for log in logs
        if log.severity in (DebugEventSeverities.high, DebugEventSeverities.medium)
            @error "While $context. $(sprint(show, log))"
        elseif log.severity == DebugEventSeverities.low
            @warn "While $context. $(sprint(show, log))"
        elseif log.severity == DebugEventSeverities.none
            @info "While $context. $(sprint(show, log))"
        else
            error("Unhandled case: ", log.severity)
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
            uvec2 totalSize = uvec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);
            vec2 uv = pixel / vec2(totalSize);

            uint idx = pixel.x  + (gl_WorkGroupSize.x * pixel.y);
            pixels[idx] = uv;
        }
    "; flexible_mode=false)
    println("Blocks: ")
    display(compute1.uniform_blocks)
    check_gl_logs("After compiling compute1")
    #  2. Raise color to an exponent.
    compute2 = Program("
        layout(local_size_x = 8, local_size_y = 8) in;
        layout(std430) buffer InOut {
            vec2 pixels[];
        };
        uniform float Power = 2.0;

        void main() {
            uvec2 pixel = gl_GlobalInvocationID.xy;
            uint idx = pixel.x  + (gl_WorkGroupSize.x * pixel.y);
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
            uint idx = pixel.x  + (gl_WorkGroupSize.x * pixel.y);

            vec2 groupUV = gl_LocalInvocationID.xy / vec2(gl_WorkGroupSize.xy);
            pixels[idx] *= groupUV;
        }
    "; flexible_mode=false)
    check_gl_logs("After compiling compute3")

    # Execute the above compute shaders, and a CPU equivalent,
    #    then check that they line up.
    GROUP_SIZE = v2u(8, 8)
    RESOLUTION = GROUP_SIZE * v2u(5, 8)
    zero_based_idcs = 0 : (RESOLUTION - 1)
    array = fill(zero(v2f), (RESOLUTION.data...))
    buf = Buffer(sizeof(v2f) * prod(RESOLUTION), false)
    check_gl_logs("After creating compute Buffer")
    #  1:
    #   a. CPU array:
    texel = @f32(1) / vsize(array)
    map!(pixel::Vec2 -> (pixel * texel)::v2f, array, zero_based_idcs)
    #   b. GPU buffer:
    set_storage_block(buf, 1)
    set_storage_block(compute1, "Output", 1)
    dispatch_compute_threads(compute1, vappend(vsize(array), 1))
    set_storage_block(1)
    check_gl_logs("After running compute1")
    #  2:
    for power in Float32.(1.2, 0.4, 1.5, 0.9)
        #   a. CPU array:
        array .^= power
        #   b. GPU buffer:
        set_storage_block(buf, 3)
        set_storage_block(compute2, "InOut", 3)
        set_uniform(compute2, "Power", power)
        dispatch_compute_threads(compute2, vappend(vsize(array), 1))
        set_storage_block(3)
        check_gl_logs("After running compute2 with $power")
    end
    #  3:
    #   a. CPU array:
    map!(array, zero_based_idcs) do pixel_idx
        work_group_uv = (pixel_idx ÷ GROUP_SIZE) / v2f(GROUP_SIZE)
        return array[pixel_idx] * work_group_uv
    end
    #   b. GPU buffer:
    set_storage_block(buf, 2)
    set_storage_block(compute3, "InOut", 2)
    dispatch_compute_threads(compute3, vappend(vsize(array), 1))
    set_storage_block(2)
    check_gl_logs("After running compute3")

    # Finally, check their relative values.
    expected::Vector{v2f} = reshape(array, (prod(RESOLUTION), ))
    actual::Vector{v2f} = get_buffer_data(buf, v2f)
    check_gl_logs("After reading compute buffer")
    axis_difference::Vector{v2f} = abs.(expected .- actual)
    total_difference::Vector{Float32} = vlength.(axis_difference)
    MAX_ERROR = @f32(0.001)
    biggest_error = max(total_difference)
    if biggest_error >= MAX_ERROR
        error("Compute shader output has a max error of ", biggest_error, "! ",
              "There are ", count(e -> e>=MAX_ERROR, total_difference), " / ",
                prod(RESOLUTION), " pixels which have this much error.")
    end
end

#TODO: Run a sequence of compute operations on an actual 2D texture.