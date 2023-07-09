##  Buttons  ##

struct JoystickButtonID
    i::Int
end

#TODO: AxisAsButton

"Some kind of binary input that GLFW tracks"
#NOTE: Make sure this has no overlap with `AxisID`!
const ButtonID = Union{GLFW.Key, GLFW.MouseButton, Tuple{GLFW.Joystick, JoystickButtonID}}

export JoystickButtonID, ButtonID


##  Axes  ##

@bp_enum(MouseAxes,
    x, y, scroll_x, scroll_y
)

struct JoystickAxisID
    i::Int
end

"Maps a key up/down to a value range"
struct ButtonAsAxis
    id::ButtonID
    released::Float32
    pressed::Float32
end
ButtonAsAxis(id::ButtonID) = ButtonAsAxis(id, 0, 1)
ButtonAsAxis_Reflected(id::ButtonID) = ButtonAsAxis(id, 1, 0)
ButtonAsAxis_Negative(id::ButtonID) = ButtonAsAxis(id, 0, -1)

"Some kind of continuous input that GLFW tracks"
#NOTE: Make sure this has no overlap with `ButtonID`!
const AxisID = Union{E_MouseAxes,
                     Tuple{GLFW.Joystick, JoystickAxisID},
                     ButtonAsAxis, Vector{ButtonAsAxis}}

export MouseAxes, E_MouseAxes, JoystickAxisID, AxisID,
       ButtonAsAxis, ButtonAsAxis_Reflected, ButtonAsAxis_Negative


#TODO: Include 'gamepads', a more player-friendly view of joysticks.