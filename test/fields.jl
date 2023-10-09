const field1 = ConstantField{2}(v3f(5, 2, 1))
@bp_check(field1 isa ConstantField{2, 3, Float32})
@bp_test_no_allocations(get_field(field1, zero(v2f)),
                        v3f(5, 2, 1))
@bp_test_no_allocations(get_field(field1, one(v2f)),
                        v3f(5, 2, 1))

const field2 = ConstantField{2}(v3f(-1, 3, -10))
@bp_check(field2 isa ConstantField{2, 3, Float32})
@bp_test_no_allocations(get_field(field2, zero(v2f)),
                        v3f(-1, 3, -10))
@bp_test_no_allocations(get_field(field2, one(v2f)),
                        v3f(-1, 3, -10))

const field3 = AddField(field1, field2)
@bp_check(field3 isa AddField{2, 3, Float32})
@bp_test_no_allocations(get_field(field3, zero(v2f)),
                        v3f(4, 5, -9))
@bp_test_no_allocations(get_field(field3, one(v2f)),
                        v3f(4, 5, -9))

# Check that the gradient of the above fields is 0 everywhere.
for const_fields in (field1, field2, field3)
    @bp_test_no_allocations(get_field_gradient(field3, zero(v2f)),
                            zero(Vec{2, v3f}))
    @bp_test_no_allocations(get_field_gradient(field3, one(v2f)),
                            zero(Vec{2, v3f}))
end

const field4 = PosField{2, Float32}()
for pos in (zero(v2f), one(v2f), v2f(3, -5))
    @bp_test_no_allocations(get_field(field4, pos), pos)
    @bp_test_no_allocations(get_field_gradient(field4, pos),
                            Vec(v2f(1, 0), v2f(0, 1)))
end

const field5 = MultiplyField(
    field4,
    ConstantField{2}(v2f(3, -5))
)
for pos in (zero(v2f), one(v2f), v2f(2, -4))
    @bp_test_no_allocations(get_field(field5, pos),
                            pos * v2f(3, -5))
    @bp_test_no_allocations(get_field_gradient(field5, pos),
                            Vec(v2f(3, 0), v2f(0, -5)))
end

# Test swizzling.
const field6 = SwizzleField(PosField{2, Float32}(), :xxyyx)
for pos in (zero(v2f), one(v2f), v2f(2, -4))
    @bp_test_no_allocations(get_field(field6, pos),
                            pos.xxyyx)
    @bp_test_no_allocations(get_field_gradient(field6, pos),
                            Vec(Vec{5, Float32}(1, 1, 0, 0, 1),
                                Vec{5, Float32}(0, 0, 1, 1, 0)))
end

# Multiply pos.xy * pos.yx.
# f(x, y) = x * y
# f' = (y, x)
const field7 = PosField{2, Float32}()
const field8 = MultiplyField(field7, SwizzleField(field7, 2, 1))
for pos in (zero(v2f), one(v2f), v2f(2, -4), v2f(-4, 2))
    @bp_test_no_allocations(get_field(field8, pos),
                            pos.xy * pos.yx)
    @bp_test_no_allocations(get_field_gradient(field8, pos),
                            Vec(pos.yx, pos.yx))
end

# Try GradientFields with the input cosine(position).
# The gradient of cos(pos) should be (-sin(pos.x), -sin(pos.y))
const field9 = GradientField(CosField(PosField{2, Float32}()), 1)
const field10 = GradientField(CosField(PosField{2, Float32}()), 2)
const field11 = GradientField(CosField(PosField{2, Float32}()),
                              ConstantField{2}(v2f(2, 3)))
for pos in (zero(v2f), one(v2f), v2f(2, -4), v2f(-40, 2000))
    # Round the result, to avoid floating-point error.
    rounded_value(val::Vec) = map(x -> Int(round(x * 10000)), val)
    rounded_field(field::AbstractField) = rounded_value(get_field(field, pos))
    @bp_test_no_allocations(rounded_value(get_field(field9, pos)),
                            rounded_value(v2f(-sin(pos.x), 0)))
    @bp_test_no_allocations(rounded_value(get_field(field10, pos)),
                            rounded_value(v2f(0, -sin(pos.y))))
    @bp_test_no_allocations(rounded_value(get_field(field11, pos)),
                            rounded_value(map(x -> -sin(x), pos) * v2f(2, 3)))
end

# Test ConversionField.
const field12 = ConversionField(PosField{2, Float32}(), Float64)
for pos in (zero(v2f), one(v2f), v2f(2, -4), v2f(-40, 2000))
    pos64 = convert(Vec2{Float64}, pos)
    @bp_test_no_allocations(get_field(field12, pos64), pos64)
end

# Test AppendField.
const field13 = AppendField(PosField{2, Float32}(), ConstantField{2}(Vec(@f32 2)))
for pos in (zero(v2f), one(v2f), v2f(2, -4), v2f(-40, 2000))
    @bp_test_no_allocations(get_field(field13, pos), vappend(pos, @f32 2))
end

# Test math ops.
const field14 = AddField(
    SubtractField(
        ConstantField{2}(Vec(@f32 3)),
        PosField{2, Float32}()
    ),
    DivideField(
        MultiplyField(
            PowField(
                PosField{2, Float32}(),
                ConstantField{2}(Vec(@f32 3))
            ),
            SqrtField(AbsField(PosField{2, Float32}()))
        ),
        ConstantField{2}(Vec(@f32 4.5))
    ),
    MinField(
        TanField(PosField{2, Float32}()),
        CosField(PosField{2, Float32}())
    ),
    MaxField(
        SinField(PosField{2, Float32}()),
        ModField(PosField{2, Float32}(),
                 ClampField(MultiplyField(PosField{2, Float32}(),
                                          PosField{2, Float32}()),
                            ConstantField{2}(Vec(@f32 4.5)),
                            ConstantField{2}(Vec(@f32 8.5))))
    )
    #TODO: Step, Lerp, Smoothstep, Smootherstep
)
field14_expected(pos) = +(
    (@f32(3) - pos),
    ((map(x -> sqrt(abs(x)), pos) *
      map(x -> x^@f32(3), pos)) /
      @f32(4.5)),
    min(map(tan, pos),
        map(cos, pos)),
    max(map(sin, pos),
        map(x -> x % clamp(x*x, @f32(4.5), @f32(8.5)),
            pos))
)
# Generate many random test cases, plus a few important ones
const FIELD14_TEST_POSES = vcat(
    [ zero(v2f), one(v2f) ],
    map(1:100) do i
        prng = PRNG(i, 2497843)
        lerp(@f32(-20), @f32(20),
             v2f(rand(prng, Float32),
                 rand(prng, Float32)))
    end
)
for pos in FIELD14_TEST_POSES
    @bp_test_no_allocations(get_field(field14, pos),
                            field14_expected(pos),
                            # Print the binary patttern of the float values,
                            #    in case there is a strange source of floating-point error
                            join(map(x -> sprint(io -> show(io, Binary(x))), pos), ','))
end

# Test negation.
const field15 = SubtractField(PosField{2, Float32}())
@bp_test_no_allocations(get_field(field15, v2f(3.5, -4.5)),
                        -v2f(3.5, -4.5))

# Test vector ops.
const field16 = AppendField(
    CrossProductField(
        NormalizeField(PosField{3, Float32}()),
        AddField(PosField{3, Float32}(),
                 ConstantField{3}(v3f(5, 0, 0))),
    ),
    DotProductField(PosField{3, Float32}(),
                    ConstantField{3}(v3f(1.5, 2.5, -3.5))),
    LengthField(PosField{3, Float32}()),
    DistanceField(PosField{3, Float32}(),
                  ConstantField{3}(v3f(-10.101, 11.12, 0.5)))
)
field16_expected(pos) = vappend(
    vnorm(pos) × (pos + v3f(5, 0, 0)),
    pos ⋅ v3f(1.5, 2.5, -3.5),
    vlength(pos),
    vdist(pos, v3f(-10.101, 11.12, 0.5))
)
# Generate many random test cases, plus a few important ones
const FIELD16_TEST_POSES = vcat(
    [ one(v3f), v3f(1, 0, 0), v3f(0, 1, 0), v3f(0, 0, 1) ],
    map(1:100) do i
        prng = PRNG(i, 2497843)
        lerp(@f32(-20), @f32(20),
             v3f(rand(prng, Float32),
                 rand(prng, Float32),
                 rand(prng, Float32)))
    end
)
for pos in FIELD16_TEST_POSES
    @bp_test_no_allocations(get_field(field16, pos),
                            field16_expected(pos),
                            # Print the binary patttern of the float values,
                            #    in case there is a strange source of floating-point error
                            join(map(x -> sprint(io -> show(io, Binary(x))), pos), ','))
end


# Test TextureField.
const TEXTURE_FIELD_TESTS = Tuple{TextureField, Vector{<:Tuple{<:Union{Vec, Real}, Union{Vec, Real}}}}[
    (
        # 1D, clamp, nearest
        TextureField(
            v3f[
                v3f(0, 1, 2),
                v3f(3, 4, 5),
                v3f(6, 7, 8),
                v3f(9, 10, 11)
            ],
            wrapping = GL.WrapModes.clamp,
            sampling = SampleModes.nearest
        ),
        [
            (@f32(0), v3f(0, 1, 2)),
            (@f32(-20), v3f(0, 1, 2)),
            (@f32(-0.32), v3f(0, 1, 2)),
            (@f32(0.2), v3f(0, 1, 2)),
            (@f32(0.1), v3f(0, 1, 2)),

            (@f32(1), v3f(9, 10, 11)),
            (@f32(1), v3f(9, 10, 11)),
            (@f32(1.2), v3f(9, 10, 11)),
            (@f32(10.2), v3f(9, 10, 11)),
            (@f32(100), v3f(9, 10, 11)),

            (@f32(0.34), v3f(3, 4, 5)),
            (@f32(0.495), v3f(3, 4, 5)),

            (@f32(0.667), v3f(6, 7, 8)),
            (@f32(0.702), v3f(6, 7, 8)),

            (@f32(0.755), v3f(9, 10, 11))
        ]
    ),
    # 1D, clamp, linear
    (
        TextureField(
            v3f[
                v3f(0, 1, 2),
                v3f(3, 4, 5),
                v3f(6, 7, 8),
                v3f(9, 10, 11)
            ],
            wrapping = GL.WrapModes.clamp,
            sampling = SampleModes.linear
        ),
        [
            # Past the min pixel:
            (@f32(0), v3f(0, 1, 2)),
            (@f32(-20), v3f(0, 1, 2)),
            (@f32(-0.32), v3f(0, 1, 2)),

            # Past the max pixel:
            (@f32(1), v3f(9, 10, 11)),
            (@f32(1), v3f(9, 10, 11)),
            (@f32(1.2), v3f(9, 10, 11)),
            (@f32(10.2), v3f(9, 10, 11)),
            (@f32(100), v3f(9, 10, 11)),

            # Near the min/max pixels:
            (@f32(0.1), v3f(0, 1, 2)),
            (@f32(0.89), v3f(9, 10, 11)),

            # Actual interpolation:
            (@f32(0.15),
             lerp(v3f(0, 1, 2),
                  v3f(3, 4, 5),
                  inv_lerp(@f32(0.125), @f32(0.375),
                           @f32(0.15)))),
            (@f32(0.34),
             lerp(v3f(0, 1, 2),
                  v3f(3, 4, 5),
                  inv_lerp(@f32(0.125), @f32(0.375),
                           @f32(0.34)))),
            (@f32(0.401),
             lerp(v3f(3, 4, 5),
                  v3f(6, 7, 8),
                  inv_lerp(@f32(0.375), @f32(0.625),
                           @f32(0.401)))),
            (@f32(0.595),
             lerp(v3f(3, 4, 5),
                  v3f(6, 7, 8),
                  inv_lerp(@f32(0.375), @f32(0.625),
                           @f32(0.595)))),
            (@f32(0.700),
             lerp(v3f(6, 7, 8),
                  v3f(9, 10, 11),
                  inv_lerp(@f32(0.625), @f32(0.875),
                           @f32(0.700)))),
            (@f32(0.800),
             lerp(v3f(6, 7, 8),
                  v3f(9, 10, 11),
                  inv_lerp(@f32(0.625), @f32(0.875),
                           @f32(0.800))))
        ]
    )
    #TODO: Wrap and mirror sampling
    #TODO: Multi-dimensional textures
]
for (field, tests) in TEXTURE_FIELD_TESTS
    for (test_pos, expected_output) in tests
        if test_pos isa Real
            test_pos = Vec(test_pos)
        end
        actual_output = get_field(field, test_pos)

        # If using 'nearest' sampling, then do exact equality.
        # Otherwise, use approximate equality.
        matches::Bool = if texture_field_sampling(field) == SampleModes.nearest
                            expected_output == actual_output
                        elseif texture_field_sampling(field) in (SampleModes.linear, )
                            isapprox(expected_output, actual_output, atol=0.0001)
                        else
                            error("Unhandled case: ", texture_field_sampling(field))
                        end
        @bp_check(matches,
                  "Sampling size ", join(size(field.pixels), "x"), " ",
                     length(size(field.pixels)), "D texture at ",
                     test_pos, " with ", texture_field_sampling(field), " ",
                     texture_field_wrapping(field), " sampling.",
                     "\n\tExpected: ", expected_output, "\n\tActual: ", actual_output)
    end
end

#TODO: Come up with a meaningful test for "noise" fields
#TODO: Test automatic promotion of 1D inputs to higher-D inputs
#TODO: Test that Lerp(), Smoothstep(), and Smootherstep() avoid heap allocations when running get_field() and get_field_gradient()

# Test the DSL.
const FIELD_DSL_POS = v2f(3, -5)
#TODO: Add a FIELD_DSL_STATE, and test TextureField
# Each test is a tuple of (DSL, expected_type, expected_value).
const FIELD_DSL_TESTS = Tuple{Any, Type, VecT{Float32}}[
    (:( 4 ), ConstantField{2, 1, Float32}, Vec(@f32(4))),
    (:( { 2, 3, 4, 5, 6 }), AppendField{2, 5, Float32}, Vec{5, Float32}(2, 3, 4, 5, 6)),
    (:( 3 * 5 ), MultiplyField{2, 1, Float32}, Vec{1, Float32}(@f32(3) * @f32(5))),
    (:( 3 * { 1, 2, 3 } ), MultiplyField{2, 3, Float32}, v3f(3, 6, 9)),
    (:( pos ), PosField{2, Float32}, FIELD_DSL_POS),
    (:( pos + 1.5 ), AddField{2, 2, Float32}, FIELD_DSL_POS + @f32(1.5)),
    (:( pos + { 1.5, -3.5 }), AddField{2, 2, Float32}, FIELD_DSL_POS + v2f(1.5, -3.5)),
    (:( sin(pos) ), SinField{2, 2, Float32}, map(sin, FIELD_DSL_POS)),
    (:( let x = 4
          x * { 4, 5, 6 }
        end ),
      MultiplyField{2, 3, Float32},
      4 * v3f(4, 5, 6)
    ),
    (:( let x = 4,
            y = 5
          let x = 0
              x * y
          end + (x * y)
        end
      ),
      AddField{2, 1, Float32},
      Vec(@f32(4) * @f32(5))
    ),
    ( :( vdot({ 1, 2, 3 }, {4, 5, 6}) ),
      DotProductField{2, 3, Float32},
      Vec(vdot(v3f(1, 2, 3), v3f(4, 5, 6)))
    ),
    ( :( { 1, 2, 3 } ⋅ {4, 5, 6} ),
      DotProductField{2, 3, Float32},
      Vec(vdot(v3f(1, 2, 3), v3f(4, 5, 6)))
    ),
    ( :( vcross({ 1, 2, 3 }, {4, -5, -6}) ),
      CrossProductField{2, Float32},
      vcross(v3f(1, 2, 3), v3f(4, -5, -6))
    ),
    ( :( { 1, 2, 3 } × {4, -5, -6} ),
      CrossProductField{2, Float32},
      vcross(v3f(1, 2, 3), v3f(4, -5, -6))
    )
    #TODO: vlength_sqr, vlength, vdist_sqr, vdist, vnorm
]
# Because the types aren't known at compile-time, we can't test for no-allocation here.
# However, earlier tests that don't use the DSL should catch those problems already.
for (dsl, expected_type, expected_value) in FIELD_DSL_TESTS
    context = DslContext(length(FIELD_DSL_POS), Float32)
    dsl_field = field_from_dsl(dsl, context)
    @bp_check(dsl_field isa expected_type,
              "Expected ", expected_type,
                " from parsing the DSL \"", dsl, "\", but got ", typeof(dsl_field))

    actual_value = get_field(dsl_field, FIELD_DSL_POS)
    @bp_check(actual_value == expected_value,
              "Expected ", expected_value,
                " from the DSL \"", dsl, "\", but got ", actual_value)

    # Check that converting the field back to DSL works, by re-parsing and re-evaluating it.
    actual_dsl = dsl_from_field(dsl_field)
    dsl_field = try
        field_from_dsl(actual_dsl, context)
    catch e
        @error(
            """Error converting dsl to field; "$actual_dsl"

            Original DSL: "$dsl"

            """,
            exception=(e, catch_backtrace())
        )
        error()
    end
    actual_value = get_field(dsl_field, FIELD_DSL_POS)
    @bp_check(actual_value == expected_value,
              "Converting into DSL and back yielded a different result!\n",
                 "\tOriginal DSL: ", dsl,
                 "\n\tGenerated DSL: ", actual_dsl,
                 "\n\tExpected ", expected_value,
                   ", got ", actual_value)

end

# Test @field, using FIELD_DSL_POS and the below DslState.
const FIELD_MACRO_STATE = DslState(
    Dict{Symbol, Stack{AbstractField}}(
        :texture1 => let stck = Stack{AbstractField}()
            push!(stck, TextureField(v3f[ v3f(0, 1, 2),
                                          v3f(3, 4, 5),
                                          v3f(6, 7, 8)
                                        ],
                                    wrapping = GL.WrapModes.clamp,
                                    sampling = SampleModes.linear))
            stck
        end
    ),
    Dict{Symbol, Array}(
        :array1 => v2f[
            v2f(-1, -2),
            v2f(-3, -4),
            v2f(-5, -6)
        ]
    )
)
# Each test is a tuple of (DSL, expected_type, expected_value).
#TODO: Use the FIELD_MACRO_STATE in some tests
const FIELD_MACRO_TESTS = Tuple{Expr, Type, VecT{Float32}}[
    (:( @field 2 Float32 4 ), ConstantField{2, 1, Float32}, Vec(@f32(4))),
    (:( @field 2 Float32 { 2, 3, 4, 5, 6 }), AppendField{2, 5, Float32}, Vec{5, Float32}(2, 3, 4, 5, 6)),
    (:( @field 2 Float32 3 * 5 ), MultiplyField{2, 1, Float32}, Vec{1, Float32}(@f32(3) * @f32(5))),
    (:( @field 2 Float32 3 * { 1, 2, 3 } ), MultiplyField{2, 3, Float32}, v3f(3, 6, 9)),
    (:( @field 2 Float32 pos ), PosField{2, Float32}, FIELD_DSL_POS),
    (:( @field 2 Float32 pos + 1.5 ), AddField{2, 2, Float32}, FIELD_DSL_POS + @f32(1.5)),
    (:( @field 2 Float32 pos + { 1.5, -3.5 }), AddField{2, 2, Float32}, FIELD_DSL_POS + v2f(1.5, -3.5)),
    (:( @field 2 Float32 sin(pos) ), SinField{2, 2, Float32}, map(sin, FIELD_DSL_POS)),
    #TODO: Try sampling from 'array1'.
    (:( @field 2 Float32 let x = 4
          x * { 4, 5, 6 }
        end ),
      MultiplyField{2, 3, Float32},
      4 * v3f(4, 5, 6)
    ),
    (:( @field 2 Float32 let x = 4,
                             y = 5
          let x = 0
            x * y
          end + (x * y)
        end
      ),
      AddField{2, 1, Float32},
      Vec(@f32(4) * @f32(5))
    )
]
for (macro_expr, expected_type, expected_value) in FIELD_MACRO_TESTS
    local evaluated_field::AbstractField
    try
        evaluated_field = eval(macro_expr)
    catch e
        error("Failed to evaluate macro DSL:\n  ", sprint(showerror, e, catch_backtrace()))
    end

    @bp_check(evaluated_field isa expected_type,
              "Expected a field type ", expected_type, " from the field DSL \"",
                 macro_expr, "\"\n  But got: ", typeof(evaluated_field))

    actual_value = get_field(evaluated_field, FIELD_DSL_POS)
    @bp_check(isapprox(actual_value, expected_value, atol=0.0001),
              "Expected the value ", expected_value, " from \"", macro_expr,
                "\"\n  But got: ", actual_value)
end


#TODO: Test sample_field()