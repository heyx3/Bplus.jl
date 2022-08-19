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

#TODO: Next test should be pos * pos

#TODO: Many 1D tests for more varieties of math