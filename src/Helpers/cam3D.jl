"A simple 3D camera, useful for 3D scene viewers."
Base.@kwdef struct Cam3D{F<:AbstractFloat}
    pos::Vec3{F} = zero(Vec3{F})

    forward::Vec3{F} = get_horz_vector(2, F)
    up::Vec3{F} = get_up_vector(F)

    clip_range::Interval{F} = Interval(min=convert(F, 0.01),
                                       max=convert(F, 1000))
    fov_degrees::F = convert(F, 90)
    aspect_width_over_height::F = one(F)
end

"Gets the vector for this camera's right-ward direction."
cam_rightward(cam::Cam3D{F}) where {F} = cam_basis(cam).right

@inline cam_basis(cam::Cam3D)::VBasis = vbasis(cam.forward, cam.up)

"Computes the view matrix for the given camera."
function cam_view_mat(cam::Cam3D{F})::Mat{4, 4, F} where {F}
    return m4_look_at(cam.pos, cam.pos + cam.forward, cam.up)
end
"Computes the projection matrix for the given camera."
function cam_projection_mat(cam::Cam3D{F})::Mat{4, 4, F} where {F}
    return m4_projection(
        min_inclusive(cam.clip_range), max_exclusive(cam.clip_range),
        cam.aspect_width_over_height,
        cam.fov_degrees)
end

export Cam3D, cam_rightward, cam_basis, cam_view_mat, cam_projection_mat


#######################
##     Settings      ##
#######################

# Different ways the camera can handle its Up axis while rotating.
@bp_enum(Cam3D_UpModes,
    # Always keeps the camera's Up vector at its current value.
    # Prevent the forward vector from reaching that value.
    keep_upright,
    # Allow the camera to roll freely, but
    #    snap back to the default Up axis once rotation input stops.
    reset_on_letting_go,
    # Allow the camera to roll without limitations.
    free
)

"The movement/turning settings for the 3D camera."
Base.@kwdef struct Cam3D_Settings{F<:AbstractFloat}
    # Movement speed, in units per second:
    move_speed::F = convert(F, 30)
    # Scale in movement speed when holding the "boost" input:
    move_speed_boost_multiplier::F = convert(F, 3)
    # The change in movement speed due to the "change speed" input:
    move_speed_scale::F = convert(F, 1.05)
    move_speed_min::F = convert(F, 0.01)
    move_speed_max::F = convert(F, 9999)

    # Turn speed, in degrees per second:
    turn_speed_degrees::F = convert(F, 360)

    # How the camera handles its own Up-axis.
    up_axis_mode::E_Cam3D_UpModes = Cam3D_UpModes.keep_upright
    up_movement_is_global::Bool = true
end

export Cam3D_UpModes, E_Cam3D_UpModes, Cam3D_Settings


####################
##     Input      ##
####################

"The user inputs used to set a 3D camera"
Base.@kwdef struct Cam3D_Input{F<:AbstractFloat}
    controlling_rotation::Bool = false # Important to know when the user stops
                                       #    controlling rotation.
    yaw::F = zero(F)
    pitch::F = zero(F)

    boost::Bool = false
    forward::F = zero(F)
    right::F = zero(F)
    up::F = zero(F)

    speed_change::F = zero(F)
end

"
Updates the given camera with the given user input,
    returning its new state.
Also returns the new Settings, which may be changed.
"
function cam_update( cam::Cam3D{F},
                     settings::Cam3D_Settings{F},
                     input::Cam3D_Input{F},
                     delta_seconds::F2
                   )::Tuple{Cam3D{F}, Cam3D_Settings{F}} where {F<:AbstractFloat, F2<:Real}
    delta_seconds_F = convert(F, delta_seconds)

    basis = cam_basis(cam)

    # Handle movement.
    max_delta_pos::F = delta_seconds_F * settings.move_speed
    if input.boost
        max_delta_pos *= settings.move_speed_boost_multiplier
    end
    move_up_dir = settings.up_movement_is_global ?
                      cam.up :
                      basis.up
    @set! cam.pos += max_delta_pos *
                     ((basis.forward * input.forward) +
                      (move_up_dir * input.up) +
                      (basis.right * input.right))

    # Handle movement speed.
    if !iszero(input.speed_change)
        scale_factor::F = settings.move_speed_scale ^ input.speed_change
        @set! settings.move_speed = clamp(settings.move_speed * scale_factor,
                                          settings.move_speed_min,
                                          settings.move_speed_max)
    end

    # Handle rotation.
    if input.controlling_rotation
        max_delta_angle::F = delta_seconds_F * settings.turn_speed_degrees

        # Apply yaw:
        angle_yaw::F = deg2rad(max_delta_angle * -input.yaw)
        rot_yaw = Quaternion{F}(cam.up, angle_yaw)
        @set! cam.forward = vnorm(q_apply(rot_yaw, cam.forward))

        # Apply pitch, but remember the original orientation in case we need to clamp:
        angle_pitch::F = deg2rad(max_delta_angle * input.pitch)
        rot_pitch = Quaternion{F}(vnorm(cam_rightward(cam)), angle_pitch)
        old_forward = cam.forward
        @set! cam.forward = vnorm(q_apply(rot_pitch, cam.forward))

        # Handle the up-axis rotation based on the camera's mode.
        if settings.up_axis_mode in (Cam3D_UpModes.free, Cam3D_UpModes.reset_on_letting_go)
            @set! cam.up = vnorm(q_apply(rot_pitch, cam.up))
        elseif settings.up_axis_mode == Cam3D_UpModes.keep_upright
            # Leave the up-axis alone, and prevent forward-axis from passing through it.
            old_right = vnorm(v_rightward(old_forward, cam.up))
            old_true_forward = vnorm(v_rightward(old_right, cam.up))
            new_true_forward = vnorm(v_rightward(cam.up, cam_rightward(cam)))
            if (vdot(cam.forward, new_true_forward) < 0) ||
               isapprox(vdot(cam.forward, cam.up), zero(F))
            #begin
                @set! cam.forward = old_forward
            end
        else
            error("Unhandled case: ", settings.up_axis_mode)
        end
    else
        # If the user is no longer controlling the camera,
        #    we may want it to snap back upright.
        if settings.up_axis_mode == Cam3D_UpModes.reset_on_letting_go
            @set! cam.up = get_up_vector()
        end
    end

    return (cam, settings)
end

export Cam3D_Input, cam_update