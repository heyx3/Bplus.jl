"
Prepares to calculate many values for a field.
The return value will be passed into `get_field()`.
"
prepare_field(f::AbstractField) = nothing


"
Gets a field's value at a specific position.

If the field implements `prepare_field()`, then that prepared data may get passed in.
Otherwise, the third parameter will be `nothing`.
"
function get_field(f::AbstractField{NIn, NOut, F}, pos::Vec{NIn, F}, prepared_data)::Vec{NOut, F} where {NIn, NOut, F}
    error("get_field() not implemented for (", typeof(f), ", ",
          typeof(pos), ", ", typeof(prepared_data), ")")
end
@inline get_field(f, pos) = get_field(f, pos, prepare_field(f))


"
The type of a field's gradient (i.e. per-component derivative).

For example, a texture-like field (2D input, 3D float output) has a `Vec{2, v3f}` gradient --
    along each of the two physical axes, the rate of change of each color channel.
"
@inline GradientType(::Type{<:AbstractField{NIn, NOut, F}}, T::Type = F) where {NIn, NOut, F} = Vec{NIn, Vec{NOut, T}}
@inline GradientType(f::AbstractField) = GradientType(typeof(f))
@inline GradientType(f::AbstractField, T::Type) = GradientType(typeof(f), T)

"
Gets a field's derivative at a specific position, per-component.

Defaults to using finite-differences, a numerical approach.

If the field implements `prepare_field()`, then that prepared data may get passed in.
Otherwise, the third parameter will be `nothing`.
"
function get_field_gradient( f::AbstractField{NIn, NOut, F},
                             pos::Vec{NIn, F},
                             prepared_data::TData = prepare_field(f)
                           )::GradientType(f) where {NIn, NOut, F, TData}
    Gradient = GradientType(f)
    EPSILON::F = field_gradient_epsilon(f, pos, prepared_data)
    center_value = get_field(f, pos, prepared_data)

    min_side = Gradient((ntuple(Val(NIn)) do i::Int
        get_field(f, @set(pos[i] -= EPSILON), prepared_data)
    end)...)
    max_side = Gradient((ntuple(Val(NIn)) do i::Int
        get_field(f, @set(pos[i] += EPSILON), prepared_data)
    end)...)

    pick_sides::GradientType(f, Bool) = min_side < max_side
    rise = vselect(min_side - center_value,
                   max_side - center_value,
                   pick_sides)
    run = vselect(one(Gradient), -one(Gradient), pick_sides)
    return rise / run
end
"Picks an epsilon value for computing the gradient of a field with finite-differences."
field_gradient_epsilon(f::AbstractField{NIn, NOut, F}, pos::Vec{NIn, F}, prepared_data) where {NIn, NOut, F} = field_gradient_epsilon(F)
field_gradient_epsilon(::Type{Float16}) = Float16(0.05)
field_gradient_epsilon(::Type{Float32}) = Float32(0.0001)
field_gradient_epsilon(::Type{Float64}) = Float64(0.0000001)
# Fallback for unhandled types (e.x. fixed-point)
field_gradient_epsilon(T::Type) = convert(T, 0.0001)


export prepare_field, get_field, get_field_gradient, field_gradient_epsilon