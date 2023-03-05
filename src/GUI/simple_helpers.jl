##  Blocks of temporary GUI state  ##

"Nests some GUI code within a window. Extra arguments get passed directly into `CImGui.Begin()`."
function gui_window(to_do, args...; kw_args...)
    output = CImGui.Begin(args..., kw_args...)
    try
        to_do()
        return output
    finally
        CImGui.End()
    end
end

"
Executes some GUI code with a different item width.

See: https://pixtur.github.io/mkdocs-for-imgui/site/api-imgui/ImGui--Dear-ImGui-end-user/#PushItemWidth

\">0.0f: width in pixels, <0.0f align xx pixels to the right of window
  (so -1.0f always align width to the right side). 0.0f = default to ~⅔ of windows width.\"
"
function gui_with_item_width(to_do, width::Real)
    CImGui.PushItemWidth(Float32(width))
    try
        return to_do()
    finally
        CImGui.PopItemWidth()
    end
end

"
Executes some GUI code without allowing the user to tab to different widgets
  (so tabs get inserted into text editors).
"
function gui_with_unescaped_tabbing(to_do)
    CImGui.PushAllowKeyboardFocus(false)
    try
        return to_do()
    finally
        CImGui.PopAllowKeyboardFocus()
    end
end

"Executes some GUI within a fold (what Dear ImGUI calls a 'tree node')."
function gui_within_fold(to_do, label)
    is_visible::Bool = CImGui.TreeNode(label)
    if is_visible
        try
            return to_do()
        finally
            CImGui.TreePop()
        end
    end

    return nothing
end

"
Groups widgets together for placement within larger layouts
    (such as a vertical group within a horizontal line).
"
function gui_within_group(to_do)
    CImGui.BeginGroup()
    try
        return to_do()
    finally
        CImGui.EndGroup()
    end
end

export gui_window, gui_with_item_width, gui_with_unescaped_tabbing,
       gui_within_fold, gui_within_group


##  Custom editors  ##

"Edits a vector with spherical coordinates; returns its new value."
function gui_spherical_vector( label, vec::v3f
                               ;
                               stays_normalized::Bool = false,
                               fallback_yaw::Ref{Float32} = Ref(zero(Float32))
                             )::v3f
    radius = vlength(vec)
    vec /= radius

    yawpitch = Float32.((atan(vec.y, vec.x), acos(vec.z)))
    # Keep the editor stable when it reaches a straight-up or straight-down vector.
    if yawpitch[2] in (Float32(-π), Float32(π))
        yawpitch = (fallback_yaw[], yawpitch[2])
    else
        fallback_yaw[] = yawpitch[1]
    end

    if stays_normalized
        # Remove floating-point error in the previous length calculation.
        radius = one(Float32)
    else
        # Provide an editor for the radius.
        @c CImGui.InputFloat("$label Length", &radius)
        CImGui.SameLine()
    end
    @c CImGui.SliderFloat2(label, &yawpitch, -π, π)

    # Convert back to cartesian coordinates.
    pitch_sincos = sincos(yawpitch[2])
    vec_2d = v2f(sincos(yawpitch[1])).yx * pitch_sincos[1]
    vec_z = pitch_sincos[2]
    return radius * vappend(vec_2d, vec_z)
end

export gui_spherical_vector