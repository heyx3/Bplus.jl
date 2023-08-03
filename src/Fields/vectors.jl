##   Dot product   ##

struct DotProductField{ NIn, NParamOut, F,
                        T1<:AbstractField{NIn, NParamOut, F},
                        T2<:AbstractField{NIn, NParamOut, F}
                      } <: AbstractField{NIn, 1, F}
    field1::T1
    field2::T2
end
export DotProductField

prepare_field(d::DotProductField) = prepare_field.((d.field1, d.field2))
function get_field( d::DotProductField{NIn, NParamOut, F},
                    pos::Vec{NIn, F},
                    prepared_data::Tuple
                  )::Vec{1, F} where {NIn, NParamOut, F}
    values::NTuple{2, Vec{NParamOut, F}} = get_field.((d.field1, d.field2),
                                                      Ref(pos),
                                                      prepared_data)
    return Vec(vdot(values...))
end
function get_field_gradient( d::DotProductField{NIn, NParamOut, F},
                             pos::Vec{NIn, F},
                             prepared_data::Tuple
                           )::Vec{NIn, Vec{1, F}} where {NIn, NParamOut, F}
    #=
            Given input position p0 = {x0, y0, z0},
                    field1 = { x1(p0), y1(p0), z1(p0) },
                    field2 = { x2(p0), y2(p0), z2(p0) }
               field1 ⋅ field2 = (x1 * x2) + (y1 * y2) + (z1 * z2)

        Use product rule for derivative:
        d/dt field1⋅field2  = d/dt (x1 * x2) + d/dt (y1 * y2) + d/dt (z1 * z2)
                            =  (dx1/dt * x2) + (x1 * dx2/dt) +
                               (dy1/dt * y2) + (y1 * dy2/dt) +
                               (dz1/dt * z2) + (z1 * dz2/dt)
                            = (dField1/dt ⋅ field2) + (field1 ⋅ dField2/dt)

        Δ = { d/x0 field1 ⋅ field2,  d/y0 field1 ⋅ field2,  d/z0 field1 ⋅ field2 }
          = {
                ((dField1/dx0) ⋅ field2) + (field1 ⋅ (dField2/dx0)),
                ((dField1/dy0) ⋅ field2) + (field1 ⋅ (dField2/dy0)),
                ((dField1/dz0) ⋅ field2) + (field1 ⋅ (dField2/dz0))
            }
    =#
    values::NTuple{2, Vec{NParamOut, F}} =
        get_field.((d.field1, d.field2), Ref(pos), prepared_data)
    gradients::NTuple{2, Vec{NIn, Vec{NParamOut, F}}} =
        get_field_gradient.((d.field1, d.field2), Ref(pos), prepared_data)
    gradient::Vec{NIn, F} = Vec(
        i -> (gradients[1][i] ⋅ values[2]) +
             (values[1] ⋅ gradients[2][i]),
        Val(NIn)
    )
    # Each derivative along an input axis is a scalar, but should be formatted as a 1D vector.
    return map(f -> Vec{1, F}(f), gradient)
end

# Dot product can be written in the DSL with 'vdot' or '⋅'.
function field_from_dsl_func(::Val{:vdot}, context::DslContext, state::DslState, args::Tuple)
    (a, b) = field_from_dsl.(args, Ref(context), Ref(state))
    return DotProductField(a, b)
end
field_from_dsl_func(::Val{:⋅}, context::DslContext, state::DslState, args::Tuple) = field_from_dsl_func(Val(:vdot), context, state, args)

dsl_from_field(d::DotProductField) = :( $(dsl_from_field(d.field1)) ⋅ $(dsl_from_field(d.field2)) )


##   Cross product   ##

struct CrossProductField{ NIn, F,
                          T1<:AbstractField{NIn, 3, F},
                          T2<:AbstractField{NIn, 3, F}
                        } <: AbstractField{NIn, 3, F}
    field1::T1
    field2::T2
end
export CrossProductField

prepare_field(c::CrossProductField) = prepare_field.((c.field1, c.field2))
function get_field( c::CrossProductField{NIn, F},
                    pos::Vec{NIn, F},
                    prepared_data::Tuple
                  )::Vec{3, F} where {NIn, F}
    return vcross(get_field(c.field1, pos, prepared_data[1]),
                  get_field(c.field2, pos, prepared_data[2]))
end
function get_field_gradient( c::CrossProductField{NIn, F},
                             pos::Vec{NIn, F},
                             prepared_data::Tuple
                           )::Vec{NIn, Vec{3, F}} where {NIn, F}
    #=
        Given input position p0 = {x0, y0, z0},
                field1 = { x1(p0), y1(p0), z1(p0) },
                field2 = { x2(p0), y2(p0), z2(p0) }
            field1 × field2 = { (y1 * z2) - (z1 * y2),
                                (z1 * x2) - (x1 * z2),
                                (x1 * y2) - (y1 * x2) }

            d/dt (field1 × field2) = { d/dt ((y1 * z2) - (z1 * y2)),
                                       d/dt ((z1 * x2) - (x1 * z2)),
                                       d/dt ((x1 * y2) - (y1 * x2)) }
                                (use product rule)
                                    = { ((dy1/dt * z2) + (y1 * dz2/dt)) -
                                        ((dz1/dt * y2) + (z1 * dy2/dt)),

                                        ((dz1/dt * x2) + (z1 * dx2/dt)) -
                                        ((dx1/dt * z2) + (x1 * dz2/dt)),

                                        ((dx1/dt * y2) + (x1 * dy2/dt)) -
                                        ((dy1/dt * x2) + (y1 * dx2/dt)) }
                                    = ((dField1/dt).yzx * field2.zxy)
                                      - ((dField1/dt).zxy * field2.yzx)
                                      + ((dField2/dt).zxy * field1.yzx)
                                      - ((dField2/dt).yzx * field1.zxy)
            Δ (field1 × field2) = { ((dField1/dx0).yzx * field2.zxy)
                                    - ((dField1/dx0).zxy * field2.yzx)
                                    + ((dField2/dx0).zxy * field1.yzx)
                                    - ((dField2/dx0).yzx * field1.zxy),
                                    ((dField1/dy0).yzx * field2.zxy)
                                    - ((dField1/dy0).zxy * field2.yzx)
                                    + ((dField2/dy0).zxy * field1.yzx)
                                    - ((dField2/dy0).yzx * field1.zxy),
                                    ((dField1/dz0).yzx * field2.zxy)
                                    - ((dField1/dz0).zxy * field2.yzx)
                                    + ((dField2/dz0).zxy * field1.yzx)
                                    - ((dField2/dz0).yzx * field1.zxy) }
    =#
    values::NTuple{2, Vec{3, F}} =
        get_field.((c.field1, c.field2), Ref(pos), prepared_data)
    gradients::NTuple{2, Vec{NIn, Vec{3, F}}} =
        get_field_gradient.((c.field1, c.field2), Ref(pos), prepared_data)
    return Vec(i -> (gradients[1][i].yzx * values[2].zxy)
                    - (gradients[1][i].zxy * values[2].yzx)
                    + (gradients[2][i].zxy * values[1].yzx)
                    - (gradients[2][i].yzx * values[1].zxy),
               Val(NIn))
end

# Cross product can be written in the DSL with 'vcross' or '×'.
function field_from_dsl_func(::Val{:vcross}, context::DslContext, state::DslState, args::Tuple)
    (a, b) = field_from_dsl.(args, Ref(context), Ref(state))
    return CrossProductField(a, b)
end
field_from_dsl_func(::Val{:×}, context::DslContext, state::DslState, args::Tuple) = field_from_dsl_func(Val(:vcross), context, state, args)

dsl_from_field(c::CrossProductField) = :( $(dsl_from_field(c.field1)) × $(dsl_from_field(c.field2)) )


##   Length-squared   ##

# Length-squared is just a vector's dot product with itself.
# Unfortunately we can't just do 'LengthSqrField(x) = DotProductField(x, x)'
#    because I doubt the compiler would be smart enough to only evaluate x once.

struct LengthSqrField{ NIn, NParamOut, F,
                       T<:AbstractField{NIn, NParamOut, F}
                     } <: AbstractField{NIn, 1, F}
    field::T
end
prepare_field(l::LengthSqrField) = prepare_field(l.field)
function get_field( l::LengthSqrField{NIn, NParamOut, F},
                    pos::Vec{NIn, F},
                    prepared_data
                  )::Vec{1, F} where {NIn, NParamOut, F}
    value = get_field(l.field, pos, prepared_data)
    return Vec{1, F}(vlength_sqr(value))
end
function get_field_gradient( l::LengthSqrField{NIn, NParamOut, F},
                             pos::Vec{NIn, F},
                             prepared_data
                           )::Vec{NIn, Vec{1, F}} where {NIn, NParamOut, F}
    value::Vec{NParamOut, F} = get_field(l.field, pos, prepared_data)
    gradient::Vec{NIn, Vec{NParamOut, F}} = get_field_gradient(l.field, pos, prepared_data)
    # Use a simplified version of the dot product gradient derived above.
    my_gradient = Vec(
        i -> convert(F, 2) * (gradient ⋅ value),
        Val(NIn)
    )
    # Each derivative along an input axis is a scalar, but should be formatted as a 1D vector.
    return map(f -> Vec{1, F}(f), my_gradient)
end


##   Simpler vector ops   ##

DistanceSqrField(a, b) = AggregateField{:DistanceSqr}(LengthSqrField(SubtractField(a, b)))
LengthField(v) = AggregateField{:Length}(SqrtField(LengthSqrField(v)))
DistanceField(a, b) = AggregateField{:Distance}(SqrtField(DistanceSqrField(a, b)))

NormalizeField(v::AbstractField{NIn, NOut, F}) where {NIn, NOut, F} = let eps = convert(F, 0.0000001),
                                                                          eps_field = ConstantField{NIn}(Vec(eps))
    AggregateField{:Normalize}(DivideField(v, MaxField(eps_field, LengthField(v))))
end

# Their representation in DSL is as function calls, with names matching the B+ functions.
for (dsl_name, key_name) in [ (:vlength_sqr, :LengthSqr),
                              (:vlength, :Length),
                              (:vdist_sqr, :DistanceSqr),
                              (:vdist, :Distance),
                              (:vnorm, :Normalize) ]
    type_name = Symbol(key_name, :Field)
    @eval begin
        field_from_dsl_func(::Val{$(QuoteNode(dsl_name))},
                            context::DslContext, state::DslState,
                            args::Tuple) =
            $type_name(field_from_dsl.(args, Ref(context), Ref(state))...)

        if $(dsl_name == :vlength_sqr)
            dsl_from_field(a::LengthSqrField) = :( vlength_sqr($(dsl_from_field(a.field))) )
        else
            dsl_from_field(a::AggregateField{$(QuoteNode(key_name))}) =
                :( $dsl_name($(dsl_from_field(a.actual_field))) )
        end

        export $type_name
    end
end