# Create a test button, and try it out.
# The generic parameter means this button isn't serializable,
#    which should generate a warning.
button_warning = @capture_err(
    @bp_button Test{I<:Integer, T} begin
        i::I = convert(I, 2)
        t::Optional{T}
        state::I

        Test{I}(::Type{T}, i::I = convert(I, 4)) where {I<:Integer, T} = Test{I, T}(nothing, zero(I), i=i)

        UPDATE(b) = (b.state += 1)
        function RAW(b)
            if exists(b.t)
                return false
            else
                return (b.state % b.i) == 0
            end
        end
    end
)
@assert contains(button_warning, "Warn")

bp_gl_context(v2i(200, 200), "Button input test") do context::Context
    # Construct the test button with the custom constructor.
    test_button = Button_Test{UInt}(String) # Should trigger on every 4th tick
                                            #    after the string is set to null
    test_button.t = "disabled!!"
    i::Int = 0
    while true
        GLFW.SwapBuffers(context.window)
        GLFW.PollEvents()

        i += 1
        Bplus.Input.button_update(test_button, context.window)
        if Bplus.Input.button_value(test_button)
            break
        end
        # Re-enable the counter at tick 6, then it should run forward
        #    to the next multiple of 4 (a.k.a. 8).
        if i==6
            test_button.t = nothing
        end
    end
    @assert i==8 "Button_Test logic: $i"
end

# Test the button serialization.
(() -> begin
    # Note that Button_Test has type parameters, and so
    #    is not deserializable except directly.
    buttons = [ Button_Key(GLFW.KEY_C),
                Button_All([
                    Button_Mouse(button=GLFW.MOUSE_BUTTON_MIDDLE),
                    Button_Key(AggregateKeys.shift),
                    Button_Manual(current_raw=true) # The 'true' shouldn't get serialized
                ])
              ]
    function do_big_test(buttons, is_deserialized_version)
        @assert buttons[1] isa Button_Key
        @assert buttons[1].key == GLFW.KEY_C
        @assert buttons[2] isa Button_All
        @assert length(buttons[2].children) == 3
        @assert buttons[2].children[1] isa Button_Mouse
        @assert buttons[2].children[1].button == GLFW.MOUSE_BUTTON_MIDDLE
        @assert buttons[2].children[2] isa Button_Key
        @assert buttons[2].children[2].key == AggregateKeys.shift
        @assert buttons[2].children[3] isa Button_Manual
        @assert buttons[2].children[3].current_raw === !is_deserialized_version # Shouldn't have been serialized
    end

    do_big_test(buttons, false)
    buttons_str = JSON3.write(buttons)
    buttons2 = JSON3.read(buttons_str, Vector{AbstractButton})
    do_big_test(buttons2, true)

    # Test direct deserialization.
    test_button1 = Button_Test{UInt8, Symbol}(:hey_there, zero(UInt8),
                                              mode=ButtonModes.just_released)
    test_button2 = JSON3.read(JSON3.write(test_button1),
                              Button_Test{UInt8, Symbol})
    @assert test_button2 isa Button_Test
    @assert test_button2.t === :hey_there
    @assert test_button2.i === UInt8(2)
    @assert test_button2.state === UInt8(0)
    @assert test_button2.type == :Test
    @assert test_button2.mode == ButtonModes.just_released
end)()