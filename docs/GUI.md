Provides a wrapper around the wonderful Dear ImGUI library (exposed in Julia as *CImGui.jl), plus many related helpers.

Note that you can freely convert between B+ vectors (`Vec{N, T}`) and CImGui vectors (`ImVec2`, `ImVec4`, `ImColor`).

## GUI service

The Dear ImGUI integration is provided as a [B+ Context service](GL#services), meaning it is a graphics-thread-singleton that you control with the following functions:

* `service_GUI_init()` to start the service
  * `service_GUI_exists()` to check if the service is already running
* `service_GUI_shutdown()` to manually kill the service (you don't have to do this if the program is closing)
* `service_GUI_start_frame()` to set up before any GUI calls are made for the current frame
* `service_GUI_end_frame()` to finish up and render the GUI to the currently-bound target, presumably the screen.
* `service_GUI_rebuild_fonts()` to update the GUI library after you've added custom fonts.
* `service_GUI()` to get the current service instance

If you want to pass a texture (or texture view) to be drawn in the GUI, you must wrap it in a call to `gui_tex_handle()`. Do not just pass the OpenGL handle for the resource.

Note that the underlying library uses static variables, and does not manage multiple independent contexts, so you cannot run this service from more than one GL Context at a time.

## Text Editor

Text editing through a C library is tricky, due to the use of C-style strings, the need to integrate clipboard, and also the need to dynamically resize the string as the user writes larger and larger text.`BplusApp.GUI.GuiText` handles all of this for you.

* Create an instance with the initial string value, and optionally configuring some of its fields.
  * For example, `GuiText("ab\ncdef\ngh", is_multiline=true)`
* Display the text widget with `gui_text!(my_text)`.
* Get the current value with `string(my_text)`.
* Change the text value with `update!(my_text, new_string)`.

## Scoped Helper Functions

Most of Dear ImGUI's state is static variables, which you configure as you draw things.
Often, it is useful to *temporarily* set some state, run GUI code, then undo your changes.
B+ offers many helper functions which manage this, by taking your GUI code as a lambda.
You should use Julia's `do` block syntax to pass the lambda, for example:

````
gui_with_indentation() do
    # Your indented GUI code here
end
````

The following functions are availble (lambda parameter is omitted for brevity):

* `gui_window(args...; kw_args...)::Optional` nests your code inside a new window.
  * All arguments are passed through to the `CImGui.Begin()` call.
  * Returns the output of your code block, or `nothing` if the UI was culled
* `gui_with_item_width(width::Real)` changes the width of widgets.
* `gui_with_indentation(indent::Optional{Real} = nothing)` indents some GUI code.
* `gui_with_padding(padding...)` sets the padding used within a window.
  * You can pass x and y values, or a tuple of X/Y values.
* `gui_with_clip_rect(rect::Box2Df, intersect_with_current_rect::Bool, draw_list = nothing)` sets the clip rectangle.
* `gui_with_font(font_or_idx::Union{Ptr, Int})` switches to one of the fonts you've already loaded into Dear ImGUI.
* `gui_with_unescaped_tabbing()` disables the ability to switch between widgets with Tab.
  * Useful if you want Tab to be recognized in a text editor.
* `gui_with_nested_id(values::Union{AbstractString, Ptr, Integer}...)` pushes new data onto Dear ImGUI's ID stack. Dear ImGUI widgets must be uniquely identified by their label plus the state of the ID stack at the time they are called, so this helps you distinguish between different widgets that have the same label.
* `gui_within_fold(label)` nests some GUI within a collapsible region.
  * Dear ImGUI calls this a "tree node".
* `gui_with_style_color(index::CImGui.ImGuiCol_, color::Union{Integer, CImGui.ImVec4})` configures a specific color within Dear ImGUI's style settings.
* `gui_within_group()` allows widgets to be referred to as an entire group. For example, you can make a vertical layout section within a horizontal layout section by calling `CImGui.SameLine()` after the entire vertical group.
* `gui_within_child_window(size, flags=0)::Optional` nests a GUI within a sub-window. Returns the output of your code block, or `nothing` if the window is culled.

## Other Helper Functions

* `gui_spherical_vector(label, direction::v3f; settings...)::v3f` edits a vector using spherical coordinates (pitch and yaw).
* `gui_next_window_space(uv_space::Box2Df)` sets the size of the next GUI window in percentages of this program's window size.