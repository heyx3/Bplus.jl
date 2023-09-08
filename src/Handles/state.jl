@kwdef mutable struct State
    style::HandleStyle = HandleStyle()
    draw_list::Ptr{CImGui.ImDrawList} = C_NULL

    space::E_HandleSpace = HandleSpace.s_local

    mat_view::fmat4 = m_identityf(4, 4)
    mat_projection::fmat4 = m_identityf(4, 4)
    mat_view_projection::fmat4 = m_identityf(4, 4)
    mat_model::fmat4 = m_identityf(4, 4)
    mat_model_local::fmat4 = m_identityf(4, 4)
    mat_model_inverse::fmat4 = m_identityf(4, 4)
    mat_model_source::fmat4 = m_identityf(4, 4)
    mat_model_source_inverse::fmat4 = m_identityf(4, 4)
    mat_model_view_projection::fmat4 = m_identityf(4, 4)
    mat_model_view_projection_local::fmat4 = m_identityf(4, 4)

    model_scale_origin::v4f = one(v4f)
    camera_pos::v4f = zero(v4f)
    camera_forward::v4f = zero(v4f)
    camera_right::v4f = zero(v4f)
    camera_up::v4f = zero(v4f)
    ray_origin::v4f = zero(v4f)
    ray_vector::v4f = zero(v4f)

    radius_square_center::Float32 = 0
    screen_square_center::v2f = zero(v2f)
    screen_square_area::Box2Df = Box2Df(min=zero(v2f), size=zero(v2f))

    screen_factor::Float32 = 0
    relative_origin::v4f = zero(v4f)

    is_using::Bool = false
    is_enabled::Bool = false
    mouse_over::Bool = false
    projection_matrix_reversed::Bool = false

    # Translation:
    translation_plan::v4f = zero(v4f)
    translation_plan_origin::v4f = zero(v4f)
    matrix_origin::v4f = zero(v4f)
    translation_last_delta::v4f = zero(v4f)

    # Rotation:
    rotation_vector_source::v4f = zero(v4f)
    rotation_angle::Float32 = 0
    rotation_angle_origin::Float32 = 0

    # Scale
    scale::v4f = zero(v4f)
    scale_value_origin::v4f = zero(v4f)
    scale_last::v4f = zero(v4f)
    saved_mouse_pos_x::Float32 = 0

    # Save axis factor when using gizmo
    below_axis_limit::v3b = zero(v3b)
    below_plane_limit::v3b = zero(v3b)
    axis_factor::v3f = zero(v3f)

    axis_limit::Float32 = 0.0025
    plane_limit::Float32 = 0.02

    # Bounds stretching
    bounds_pivot::v4f = zero(v4f)
    bounds_anchor::v4f = zero(v4f)
    bounds_plan::v4f = zero(v4f)
    bounds_local_pivot::v4f = zero(v4f)
    bounds_best_axis::Int32 = 0
    bounds_axis::v2i = zero(v2i)
    is_using_bounds::Bool = false
    bounds_matrix::fmat4 = m_identityf(4, 4)

    current_operation::Int

    rect::Box2Df = Box2Df(min=zero(v2f), size=zero(v2f))
    rect_aspect_ratio::Float32 = 1

    is_ortho::Bool = false

    actual_id::Int32 = -1
    editing_id::Int32 = -1
    operation::E_HandleType = HandleType.ALL

    allow_axis_flip::Bool = true
    gizmo_size_clip_space::Float32 = 0.1
end

const STATE = State()