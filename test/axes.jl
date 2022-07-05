# Create a test axis, and try it out.
# The generic parameter means this axis isn't serializable,
#    which should generate a warning.
axis_warning = @capture_err(
    @bp_axis unsigned Test{I<:Integer, T} begin
        i::I = convert(I, 2)
        t::Optional{T}
        state::I

        Test{I}(::Type{T}, i::I = convert(I, 4)) where {I<:Integer, T} = Test{I, T}(nothing, zero(I), i=i)

        UPDATE(b) = (b.state += 1)
        function RAW(b)
            if exists(b.t)
                return 0
            else
                return ((b.state % b.i) == 0) ? 1 : 0
            end
        end
    end
)
@assert contains(axis_warning, "Warn")

bp_gl_context(v2i(200, 200), "Axis input test") do context::Context
    # Construct the test axis with the custom constructor.
    test_axis = Axis_Test{UInt}(String) # Should trigger on every 4th tick
                                        #    after the string is set to null
    test_axis.t = "disabled!!"
    i::Int = 0
    while true
        GLFW.SwapBuffers(context.window)
        GLFW.PollEvents()

        i += 1
        Bplus.Input.axis_update(test_axis, context.window)
        if Bplus.Input.axis_value(test_axis) > 0
            break
        end
        # Re-enable the counter at tick 6, then it should run forward
        #    to the next multiple of 4 (a.k.a. 8).
        if i==6
            test_axis.t = nothing
        end
    end
    @assert i==8 "Axis_Test logic: $i"
end

# Test the axis serialization.
(() -> begin
    # Note that Axis_Test has type parameters, and so
    #    is not deserializable except directly.
    axes = [ Axis_Key(GLFW.KEY_C),
             Axis_Sum([
                 Axis_Mouse(button=GLFW.MOUSE_BUTTON_MIDDLE),
                 Axis_Key2(GLFW.KEY_J, AggregateKeys.shift),
                 Axis_Manual(current_raw=0.5) # The '0.5' shouldn't get serialized
             ])
           ]
    function do_big_test(axes, is_deserialized_version)
        @assert axes[1] isa Axis_Key
        @assert axes[1].key == GLFW.KEY_C
        @assert axes[2] isa Axis_Sum
        @assert length(axes[2].children) == 3
        @assert axes[2].children[1] isa Axis_Mouse
        @assert axes[2].children[1].button == GLFW.MOUSE_BUTTON_MIDDLE
        @assert axes[2].children[2] isa Axis_Key2
        @assert axes[2].children[2].key_positive == GLFW.KEY_J
        @assert axes[2].children[2].key_negative == AggregateKeys.shift
        @assert axes[2].children[3] isa Axis_Manual
        @assert isapprox(axes[2].children[3].current_raw,
                         (is_deserialized_version ? 0.0 : 0.5), # Shouldn't have been serialized
                         atol=0.01)
    end

    do_big_test(axes, false)
    axes_str = JSON3.write(axes)
    axes2 = JSON3.read(axes_str, Vector{AbstractAxis})
    do_big_test(axes2, true)

    # Test direct deserialization.
    test_axis1 = Axis_Test{UInt8, Symbol}(:hey_there, zero(UInt8))
    test_axis2 = JSON3.read(JSON3.write(test_axis1),
                              Axis_Test{UInt8, Symbol})
    @assert test_axis2 isa Axis_Test
    @assert test_axis2.t === :hey_there
    @assert test_axis2.i === UInt8(2)
    @assert test_axis2.state === UInt8(0)
    @assert test_axis2.type == :Test
end)()