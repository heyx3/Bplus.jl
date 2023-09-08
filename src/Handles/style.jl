@kwdef mutable struct HandleStyle
    translation_line_thickness::Float32 = 3
    translation_line_arrow_size::Float32 = 6

    rotation_line_thickness::Float32 = 2
    rotation_outer_line_thickness::Float32 = 3

    scale_line_thickness::Float32 = 3
    scale_line_circle_size::Float32 = 6

    hatched_axis_line_thickness::Float32 = 6

    center_circle_size::Float32 = 6

    colors::NTuple{length(HandleColorKey.instances()), vRGBAf} = ntuple(Val(length(HandleColorKey.instances()))) do i
        (i == Int(HandleColorKey.dir_x)) &&                 vRGBAf(0.666, 0.000, 0.000, 1.000)
        (i == Int(HandleColorKey.dir_y)) &&                 vRGBAf(0.000, 0.666, 0.000, 1.000)
        (i == Int(HandleColorKey.dir_z)) &&                 vRGBAf(0.000, 0.000, 0.666, 1.000)
        (i == Int(HandleColorKey.plane_x)) &&               vRGBAf(0.666, 0.000, 0.000, 0.380)
        (i == Int(HandleColorKey.plane_y)) &&               vRGBAf(0.000, 0.666, 0.000, 0.380)
        (i == Int(HandleColorKey.plane_z)) &&               vRGBAf(0.000, 0.000, 0.666, 0.380)
        (i == Int(HandleColorKey.selection)) &&             vRGBAf(1.000, 0.500, 0.062, 0.541)
        (i == Int(HandleColorKey.inactive)) &&              vRGBAf(0.600, 0.600, 0.600, 0.600)
        (i == Int(HandleColorKey.translation_line)) &&      vRGBAf(0.666, 0.666, 0.666, 0.666)
        (i == Int(HandleColorKey.scale_line)) &&            vRGBAf(0.250, 0.250, 0.250, 1.000)
        (i == Int(HandleColorKey.rotation_using_border)) && vRGBAf(1.000, 0.500, 0.062, 1.000)
        (i == Int(HandleColorKey.rotation_using_fill)) &&   vRGBAf(1.000, 0.500, 0.062, 0.500)
        (i == Int(HandleColorKey.hatched_axis_lines)) &&    vRGBAf(0.000, 0.000, 0.000, 0.500)
        (i == Int(HandleColorKey.text)) &&                  vRGBAf(1.000, 1.000, 1.000, 1.000)
        (i == Int(HandleColorKey.text_shadow)) &&           vRGBAf(0.000, 0.000, 0.000, 1.000)
    end
end