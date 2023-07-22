"
Encapsulates a Dear ImGUI text editor.

Run the GUI with `gui_text!()` and get the current string with `string()`.
Update the value externally with `update!()`.
"
Base.@kwdef mutable struct GuiText
    raw_value::InteropString

    label::String = ""
    is_multiline::Bool = false
    multiline_requested_size::NTuple{2, Integer} = (0, 0)
    imgui_flags::Integer = 0x00000

    # Internal; leave this alone
    _buffer_has_been_updated = false
    _edit_callback::Base.Callable = Nothing
    _c_func_edit_callback::Optional{Base.CFunction} = nothing
    #TODO: Dear ImGUI callbacks for resizing text buffer as needed
end

GuiText(initial_value::AbstractString; kw...) = GuiText(;
    raw_value = InteropString(initial_value),
    kw...
)

"
Displays a Dear ImGUI widget for the given text editor.
Returns whether the text has been edited on this frame.
"
function gui_text!(t::GuiText)::Bool
    # Lazy-initialize the C callback for buffer resizing.
    if isnothing(t._c_func_edit_callback)
        t._edit_callback = (p::Ptr{CImGui.ImGuiInputTextCallbackData}) -> begin
            data::CImGui.ImGuiInputTextCallbackData = unsafe_load(p)
            if data.EventFlag == CImGui.ImGuiInputTextFlags_CallbackResize
                desired_size::Int = data.BufSize
                if length(t.raw_value.c_buffer) < desired_size
                    resize!(t.raw_value.c_buffer, desired_size * 2)
                end
            elseif data.EventFlag == CImGui.ImGuiInputTextFlags_CallbackCompletion
                println("Completed!")
            else
                println("Other event: ", data.EventFlag)
            end
            return Cint(0) # Does anybody know if this value has any meaning?
        end
        t._c_func_edit_callback = @cfunction($(t._edit_callback), Cint, (Ptr{CImGui.ImGuiInputTextCallbackData}, ))
    end

    changed::Bool = false
    if t.is_multiline
        changed = @c CImGui.InputTextMultiline(
            t.label,
            &t.raw_value.c_buffer[0], length(t.raw_value.c_buffer),
            t.multiline_requested_size,
            t.imgui_flags |
                CImGui.ImGuiInputTextFlags_CallbackResize |
                CImGui.ImGuiInputTextFlags_CallbackCompletion,
            t._c_func_edit_callback
        )
    else
        changed = @c CImGui.InputText(
            t.label,
            &t.raw_value.c_buffer[0], length(t.raw_value.c_buffer),
            t.imgui_flags
        )
    end

    t._buffer_has_been_updated |= changed
    return changed
end

"Gets the current value of a GUI-edited text."
function Base.string(t::GuiText)::String
    if t._buffer_has_been_updated
        update!(t.raw_value)
        t._buffer_has_been_updated = false
    end
    return t.raw_value.julia
end

function Utilities.update!(t::GuiText, s::AbstractString)
    update!(t.raw_value, s)
    t._buffer_has_been_updated = false
end

export GuiText, gui_text!