# Simulate an entity maneuvering in different ways.
# Note that this example comes from @component's doc-string.

#TODO: Test SUPER() calls.

@component Position begin
    value::v3f
end
@component MaxSpeed begin
    value::Float32
end


# "Some kind of animated motion. Only one maneuver can run at a time."
@component Maneuver {abstract} {entitySingleton} {require: Position} begin
    pos_component::Position
    duration::Float32
    progress_normalized::Float32

    function CONSTRUCT(duration_seconds)
        this.progress_normalized = 0
        this.duration = convert(Float32, duration_seconds)
        this.pos_component = get_component(entity, Position)
    end
    DEFAULT() = StrafingManeuver(6.0, vnorm(v3f(1, -2, 4)))

    @promise finish_maneuver(last_time_step::Float32)
    @configurable should_stop()::Bool = false

    function TICK()
        this.progress_normalized += world.delta_seconds / this.duration
    end
    function FINISH_TICK()
        if this.should_stop()
            remove_component(entity, this)
        elseif this.progress_normalized >= 1
            overshoot_seconds = (this.progress_normalized - @f32(1)) * this.duration
            this.finish_maneuver(-overshoot_seconds)
            remove_component(entity, this)
        end
    end
end

"
A Maneuver that moves the entity in a specific direction at a constant speed.
Only one maneuver can run at a time.
"
const DUMMY = 4
@component StrafingManeuver <: Maneuver {require: MaxSpeed} begin
    speed_component::MaxSpeed
    dir::v3f
    force_stop::Bool
    function CONSTRUCT(duration_seconds, dir)
        SUPER(duration_seconds)
        this.speed_component = get_component(entity, MaxSpeed)
        this.dir = vnorm(convert(v3f, dir))
        this.force_stop = false
    end

    TICK() = strafe(this)
    finish_maneuver(last_time_step::Float32) = strafe(this, last_time_step)

    should_stop() = this.force_stop

end
function strafe(s::StrafingManeuver, time_step=nothing)
    if isnothing(time_step)
        time_step = s.world.delta_seconds
    end
    s.pos_component.value += s.speed_component.value * time_step * s.dir
end


#############################

world = World()
entity = add_entity(world)

pos = add_component(entity, Position, v3f(2, 3, 4))
speed = add_component(entity, MaxSpeed, 8)
@bp_check(pos.value === v3f(2, 3, 4))
@bp_check(speed.value == 8)

maneuver = add_component(entity, Maneuver)
@bp_check(maneuver isa StrafingManeuver, maneuver,
          world.component_lookup, "\n\n",
            world.entity_lookup, "\n\n",
            world.component_counts)
@bp_check(has_component(entity, StrafingManeuver))
@bp_check(maneuver.duration === @f32(6.0), maneuver)
@bp_check(maneuver.progress_normalized == 0, maneuver)
@bp_check(maneuver.pos_component == pos, maneuver)
@bp_check(maneuver.speed_component == speed, maneuver)
@bp_check(isapprox(maneuver.dir, vnorm(v3f(1, -2, 4)), atol=0.0001),
          maneuver)
@bp_check(maneuver.force_stop == false, maneuver)

tick_world(world, @f32(2))
@bp_check(has_component(entity, StrafingManeuver))
@bp_check(isapprox(pos.value, v3f(2, 3, 4) + (@f32(2) * @f32(8) * vnorm(v3f(1, -2, 4))),
                   atol=0.0001),
          pos.value, " vs ", v3f(2, 3, 4) + (@f32(2) * @f32(8) * vnorm(v3f(1, -2, 4))))

tick_world(world, @f32(5))
@bp_check(!has_component(entity, StrafingManeuver))
@bp_check(isapprox(pos.value, v3f(2, 3, 4) + (@f32(6) * @f32(8) * vnorm(v3f(1, -2, 4))),
                   atol=0.0001),
          pos.value, " vs ", v3f(2, 3, 4) + (@f32(6) * @f32(8) * vnorm(v3f(1, -2, 4))))

# Check the ability to use a @configurable to quit the maneuver early.
maneuver = add_component(entity, Maneuver)
tick_world(world, @f32(2))
@bp_check(has_component(entity, StrafingManeuver))
maneuver.force_stop = true
tick_world(world, @f32(0.0001))
@bp_check(!has_component(entity, StrafingManeuver))