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

#TODO: Many 1D tests for more varieties of math

#TODO: Test automatic promotion of 1D inputs to higher-D inputs

#TODO: test AppendField

#TODO: test that Lerp(), Smoothstep(), and Smootherstep() avoid heap allocations when running get_field() and get_field_gradient()

# Test the DSL.
const FIELD_DSL_POS = v2f(3, -5)
# Each test is a tuple of (DSL, expected_type, expected_value).
const FIELD_DSL_TESTS = Tuple{Any, Type{<:AbstractField{2}}, VecT{Float32}}[
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
    )
    #TODO: More tests
]
# Because the types aren't known at compile-time, we can't test for no-allocation here.
# However, earlier tests that don't use the DSL should catch those problems already.
for (dsl, expected_type, expected_value) in FIELD_DSL_TESTS
    context = DslContext(length(FIELD_DSL_POS), -1, Float32) # Note that 'NOut' isn't useful here,
                                                             #   as it's generally deduced from the DSL.
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