"
Prepares to calculate many values for a field, across a discrete grid of the given size.
The return value may be passed into `get_field()`.
"
prepare_field(f::AbstractField{NIn}, discrete_size::Vec{NIn, Int}) where {NIn} = nothing


"
Gets a field's value at a specific position.

If the field implements `prepare_field()`, then that prepared data may get passed in.
However, it should still implement the two-parameter overload as well.

If the field doesn't implement `prepare_field()`,
    then it should specify `::Nothing = nothing` for the third parameter.
"
function get_field(f::AbstractField{NIn, NOut, F}, pos::Vec{NIn, F}, prepared_data)::Vec{NOut, F} where {NIn, NOut, F}
    error("get_field() not implemented for (", typeof(f), ", ",
          typeof(pos), ", ", typeof(prepared_data), ")")
end


"
The type of a field's gradient (i.e. per-component derivative).

For example, a texture-like field (2D input, 3D float output) has a `Vec{2, v3f}` gradient --
    along each of the two physical axes, the rate of change of each color channel.
"
@inline GradientType(::AbstractField{NIn, NOut, F}, T::Type = F) where {NIn, NOut, F} = Vec{NIn, Vec{NOut, T}}

"
Gets a field's derivative at a specific position, per-component.

Defaults to using finite-differences, a numerical approach.

If the field implements `prepare_field()`, then that prepared data may get passed in.
However, it should still implement the two-parameter overload as well.

If the field doesn't implement `prepare_field()`,
    then it should specify `::Nothing = nothing` for the third parameter.
"
function get_field_gradient( f::AbstractField{NIn, NOut, F},
                             pos::Vec{NIn, F},
                             prepared_data::TData = nothing
                           )::GradientType(f) where {NIn, NOut, F, TData}
    Gradient = GradientType(f)
    EPSILON = convert(F, 0.0001)
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


export prepare_field, get_field, get_field_gradient