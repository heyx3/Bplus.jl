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
const field8 = MultiplyField(field7, SwizzleField(field7, :yx))
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