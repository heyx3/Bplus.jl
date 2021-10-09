# Define some enums with the same value names.
# Check that they work as expected, and that
#    the new enums' values don't overwrite the old.

@bp_enum A a b c
@bp_test_no_allocations(A.a, A.from(0))
@bp_test_no_allocations(A.b, A.from(1))
@bp_test_no_allocations(A.c, A.from(2))
@bp_test_no_allocations(A.a isa A_t, true)
@bp_test_no_allocations(A.b isa A_t, true)
@bp_test_no_allocations(A.c isa A_t, true)

@bp_enum B a b=20 c=-10
@bp_test_no_allocations(B.a, B.from(0))
@bp_test_no_allocations(B.b, B.from(20))
@bp_test_no_allocations(B.c, B.from(-10))
@bp_test_no_allocations(B.a isa B_t, true)
@bp_test_no_allocations(B.b isa B_t, true)
@bp_test_no_allocations(B.c isa B_t, true)
# Test the other enum again, to make sure there wasn't a name collision.
@bp_test_no_allocations(A.a, A.from(0))
@bp_test_no_allocations(A.b, A.from(1))
@bp_test_no_allocations(A.c, A.from(2))
@bp_test_no_allocations(A.a isa A_t, true)
@bp_test_no_allocations(A.b isa A_t, true)
@bp_test_no_allocations(A.c isa A_t, true)

@bp_enum C::UInt8 a b=255 c=1
@bp_test_no_allocations(C.a, C.from(0))
@bp_test_no_allocations(C.b, C.from(255))
@bp_test_no_allocations(C.c, C.from(1))
@bp_test_no_allocations(C.a isa C_t, true)
@bp_test_no_allocations(C.b isa C_t, true)
@bp_test_no_allocations(C.c isa C_t, true)
# Test the other enums again, to make sure there wasn't a name collision.
@bp_test_no_allocations(A.a, A.from(0))
@bp_test_no_allocations(A.b, A.from(1))
@bp_test_no_allocations(A.c, A.from(2))
@bp_test_no_allocations(B.a, B.from(0))
@bp_test_no_allocations(B.b, B.from(20))
@bp_test_no_allocations(B.c, B.from(-10))
@bp_test_no_allocations(A.a isa A_t, true)
@bp_test_no_allocations(A.b isa A_t, true)
@bp_test_no_allocations(A.c isa A_t, true)
@bp_test_no_allocations(B.a isa B_t, true)
@bp_test_no_allocations(B.b isa B_t, true)
@bp_test_no_allocations(B.c isa B_t, true)