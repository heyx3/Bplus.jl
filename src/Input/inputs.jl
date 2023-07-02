"Computes an input's, next value, and returns the updated version of it"
function update_input end


##  Buttons  ##

@bp_enum(ButtonModes,
    down, up,
    just_pressed, just_released
)
struct ButtonInput
    id::ButtonID
    mode::E_ButtonModes

    value::Bool
    prev_raw::Bool
end
ButtonInput(id::ButtonID, mode::E_ButtonModes = ButtonModes.down) = ButtonInput(id, mode, false, false)

function raw_input(id::ButtonID, wnd::GLFW.Window)::Bool
    if id isa GLFW.Key
        return GLFW.GetKey(wnd, id)
    elseif id isa GLFW.MouseButton
        return GLFW.GetMouseButton(wnd, id)
    elseif id isa Tuple{GLFW.Joystick, JoystickButtonID}
        buttons = GLFW.GetJoystickButtons(id[1])
        return exists(buttons) && (id[2].i <= length(buttons)) && buttons[id[2].i]
    else
        error("Unimplemented: ", typeof(id))
    end
end
function update_input(input::ButtonInput, wnd::GLFW.Window)::ButtonInput
    new_raw = raw_input(input.id, wnd)
    @set! input.value = if input.mode == ButtonModes.down
                            new_raw
                        elseif input.mode == ButtonModes.up
                            !new_raw
                        elseif input.mode == ButtonModes.just_pressed
                            new_raw && !input.prev_raw
                        elseif input.mode == ButtonModes.just_released
                            !new_raw && input.prev_raw
                        else
                            error("Unimplemented: ", input.mode)
                        end

    @set! input.prev_raw = new_raw
    return input
end

export ButtonModes, E_ButtonModes, ButtonInput


##  Axes  ##

@bp_enum(AxisModes,
    # The raw value.
    value,
    # The difference between the value this frame and its value last frame.
    delta
)
struct AxisInput
    id::AxisID
    mode::E_AxisModes

    value::Float32
    prev_raw::Float32

    value_scale::Float32
end
AxisInput(id::AxisID, mode::E_AxisModes = AxisModes.value
          ;
          initial_value = 0,
          initial_raw = initial_value,
          value_scale = 1
         ) = AxisInput(id, mode, initial_value, initial_raw, value_scale)

function raw_input(id::AxisID, wnd::GLFW.Window, current_scroll::v2f)::Float32
    if id isa E_MouseAxes
        if id == MouseAxes.x
            return GLFW.GetCursorPos(wnd).x
        elseif id == MouseAxes.y
            return GLFW.GetCursorPos(wnd).y
        elseif id == MouseAxes.scroll_x
            return current_scroll.x
        elseif id == MouseAxes.scroll_y
            return current_scroll.y
        else
            error("Unimplemented: MouseAxes.", id)
        end
    elseif id isa Tuple{GLFW.Joystick, JoystickAxisID}
        let axes = GLFW.GetJoystickAxes(id[1])
            if exists(axes) && (id[2].i <= length(axes))
                return @f32(axes[id[2].i])
            else
                return @f32(0)
            end
        end
    elseif id isa ButtonAsAxis
        button_value = raw_input(id.id, wnd)
        return button_value ? id.pressed : id.released
    elseif id isa Vector{ButtonAsAxis}
        total_value::Float32 = 0
        for button::ButtonAsAxis in id
            button_value::Bool = raw_input(button.id, wnd)
            delta = (button_value ? button.pressed : button.released)
            total_value += delta
        end
        return total_value
    else
        error("Unimplemented: ", typeof(id))
    end
end
function update_input(input::AxisInput, wnd::GLFW.Window,
                      current_scroll::v2f)::AxisInput
    new_raw = raw_input(input.id, wnd, current_scroll)
    @set! input.value = input.value_scale *
                          if input.mode == AxisModes.value
                              new_raw
                          elseif input.mode == AxisModes.delta
                              new_raw - input.prev_raw
                          else
                              error("Unimplemented: ", typeof(input.mode))
                          end
    @set! input.prev_raw = new_raw

    return input
end

export AxisModes, E_AxisModes, AxisInput