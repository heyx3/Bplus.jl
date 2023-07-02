##  Buttons  ##

const JoystickButtonID = Int

"Some kind of binary input that GLFW tracks"
const ButtonID = Union{GLFW.Key, GLFW.MouseButton, Tuple{GLFW.Joystick, JoystickButtonID}}

export JoystickButtonID, ButtonID


##  Axes  ##

@bp_enum(MouseAxes,
    x, y, scroll_x, scroll_y
)
const JoystickAxisID = Int

"Some kind of continuous input that GLFW tracks"
const AxisID = Union{E_MouseAxes, Tuple{GLFW.Joystick, JoystickAxisID}}

export MouseAxes, E_MouseAxes, JoystickAxisID, AxisID


#TODO: Include 'gamepads', a more player-friendly view of joysticks.