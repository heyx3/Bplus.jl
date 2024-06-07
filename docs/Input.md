# Input

Provides a greatly-simplified window into GLFW input, as a [GL Context Service](docs/GL.md#Services). Input bindings are defined in a data-driven way that is easily serialized through the `StructTypes` package.

Make sure to update this service every frame (with `service_Input_update()`); this is already handled for you if you build your game logic within [`@game_loop`](docs/Helpers.md#Game-Loop).

You can reset the service's state with `service_Input_reset()`.

## Buttons

Buttons are binary inputs. A single button can come from more than one source, in which case they're OR-ed together (e.x. you could bind "fire" to right-click, Enter, and Joystick1->Button4).

Create a button with `create_button(name::AbstractString, inputs::ButtonInput...)`. Refer to the `ButtonInput` class for more info on how to configure your buttons. A button can come from keyboard keys, mouse buttons, or joystick buttons.

Get the current value of a button with `get_button(name::AbstractString)::Bool`.

## Axes

Axes are continuous inputs, represented with `Float32`. Unlike buttons, they can only come from one source.

Create an axis with `create_axis(name::AbstractString, input::AxisInput)`. Refer to the `AxisInput` class for more info on how to configure your axis. An axis can come from mouse position, scroll wheel, joystick axes, or a list of `ButtonAsAxis`.

Get the current value of an axis with `get_axis(name::AbstractString)::Float32`.