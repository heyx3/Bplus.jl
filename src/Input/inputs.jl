"Computes an input's, next value, and returns the updated version of it"
function update_input end


##  Buttons  ##

@bp_enum(ButtonModes,
    Down, Up,
    JustPressed, JustReleased
)
struct ButtonInput
    id::ButtonID
    mode::E_ButtonModes

    value::Bool
    prev_raw::Bool
end
ButtonInput(id::ButtonID, mode::E_ButtonModes) = ButtonInput(id, mode, false, false)

function update_input(input::ButtonInput, wnd::GLFW.Window)::ButtonInput
    new_raw = if input.id isa GLFW.Key
                  !iszero(GLFW.GetKey(wnd, input.id))
              elseif input.id isa GLFW.MouseButton
                  !iszero(GLFW.GetMouseButton(wnd, input.id))
              elseif input.id isa Tuple{GLFW.Joystick, JoystickButtonID}
                  let buttons = GLFW.GetJoystickButtons(input.id[1])
                      exists(buttons) && (input.id[2] <= length(buttons)) &&
                        !iszero(buttons[input.id[2]])
                  end
              else
                  error("Unimplemented: ", typeof(input.id))
              end

    @set! input.value = if input.mode == ButtonModes.Down
                            new_raw
                        elseif input.mode == ButtonModes.Up
                            !new_raw
                        elseif input.mode == ButtonModes.JustPressed
                            new_raw && !input.prev_raw
                        elseif input.mode == ButtonModes.JustReleased
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
end
AxisInput(id::AxisID, mode::E_ButtonModes,
          initial_value = 0,
          initial_raw = initial_value) = AxisInput(id, mode, initial_value, initial_raw)

function update_input(input::AxisInput, wnd::GLFW.Window,
                      currentScroll::v2f)::AxisInput
    new_raw = if input.id isa E_MouseAxes
                  if input.id == MouseAxes.x
                      GLFW.GetCursorPos(wnd).x
                  elseif input.id == MouseAxes.y
                      GLFW.GetCursorPos(wnd).y
                  elseif input.id == MouseAxes.scroll_x
                      currentScroll.x
                  elseif input.id == MouseAxes.scroll_y
                      currentScroll.y
                  else
                      error("Unimplemented: MouseAxes.", input.id)
                  end
              elseif input.id isa Tuple{GLFW.Joystick, GLFW.JoystickAxisID}
                  let axes = GLFW.GetJoystickAxes(input.id[1])
                      if exists(axes) && (input.id[2] <= length(axes))
                          @f32(axes[input.id[2]])
                      else
                          @f32(0)
                      end
                  end
              else
                  error("Unimplemented: ", typeof(input.id))
              end
    @set! input.value = if input.mode == AxisModes.value
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