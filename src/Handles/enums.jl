@bp_bitflag(HandleType::UInt16,
    translate_x, translate_y, translate_z,
    rotate_x, rotate_y, rotate_z, rotate_screen,
    scale_x, scale_y, scale_z,
    bounds,
    scaleu_x, scaleu_y, scaleu_z,

    translate = translate_x | translate_y | translate_z,
    rotate = rotate_x | rotate_y | rotate_z,
    scale = scale_x | scale_y | scale_z,
    scaleu = scaleu_x | scaleu_y | scaleu_z,
    universal = translate | rotate | scaleu
)
export HandleType, E_HandleType

@bp_enum(HandleSpace,
    s_local,
    s_world
)
export HandleSpace, E_HandleSpace

@bp_enum(HandleColorKey,
    dir_x, dir_y, dir_z,
    plane_x, plane_y, plane_z,
    selection, inactive,
    translation_line, scale_line,
    rotation_using_border, rotation_using_fill,
    hatched_axis_lines,
    text, text_shadow
)
export HandleColorKey, E_HandleColorKey