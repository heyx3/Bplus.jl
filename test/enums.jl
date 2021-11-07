# Test @ano_enum
@bp_test_no_allocations(@ano_enum(ABC, DEF, GHI),
                        Union{Val{:ABC}, Val{:DEF}, Val{:GHI}})

# Define some enums with the same value names.
# Check that they work as expected, and that
#    the new enums' values don't overwrite the old.

@bp_enum A a b c
@bp_test_no_allocations(A.a, A.from(0))
@bp_test_no_allocations(A.b, A.from(1))
@bp_test_no_allocations(A.c, A.from(2))
@assert(A.from("a") == A.a, "A from string")
@assert(A.from("b") == A.b, "A from string")
@assert(A.from("c") == A.c, "A from string")
@bp_test_no_allocations(length(A.instances()), 3)
@bp_test_no_allocations(A.a in A.instances(), true)
@bp_test_no_allocations(A.b in A.instances(), true)
@bp_test_no_allocations(A.c in A.instances(), true)
@bp_test_no_allocations(A.a isa E_A, true)
@bp_test_no_allocations(A.b isa E_A, true)
@bp_test_no_allocations(A.c isa E_A, true)

@bp_enum B a b=20 c=-10
@bp_test_no_allocations(B.a, B.from(0))
@bp_test_no_allocations(B.b, B.from(20))
@bp_test_no_allocations(B.c, B.from(-10))
@assert(B.from("a") == B.a, "B from string")
@assert(B.from("b") == B.b, "B from string")
@assert(B.from("c") == B.c, "B from string")
@bp_test_no_allocations(length(B.instances()), 3)
@bp_test_no_allocations(B.a in B.instances(), true)
@bp_test_no_allocations(B.b in B.instances(), true)
@bp_test_no_allocations(B.c in B.instances(), true)
@bp_test_no_allocations(B.a isa E_B, true)
@bp_test_no_allocations(B.b isa E_B, true)
@bp_test_no_allocations(B.c isa E_B, true)
# Test the other enum again, to make sure there wasn't a name collision.
@bp_test_no_allocations(A.a, A.from(0))
@bp_test_no_allocations(A.b, A.from(1))
@bp_test_no_allocations(A.c, A.from(2))
@bp_test_no_allocations(length(A.instances()), 3)
@bp_test_no_allocations(A.a in A.instances(), true)
@bp_test_no_allocations(A.b in A.instances(), true)
@bp_test_no_allocations(A.c in A.instances(), true)
@assert(A.from("a") == A.a, "A from string")
@assert(A.from("b") == A.b, "A from string")
@assert(A.from("c") == A.c, "A from string")
@bp_test_no_allocations(A.a isa E_A, true)
@bp_test_no_allocations(A.b isa E_A, true)
@bp_test_no_allocations(A.c isa E_A, true)

@bp_enum C::UInt8 a b=255 c=1
@bp_test_no_allocations(C.a, C.from(0))
@bp_test_no_allocations(C.b, C.from(255))
@bp_test_no_allocations(C.c, C.from(1))
@assert(C.from("a") == C.a, "C from string")
@assert(C.from("b") == C.b, "C from string")
@assert(C.from("c") == C.c, "C from string")
@bp_test_no_allocations(length(C.instances()), 3)
@bp_test_no_allocations(C.a in C.instances(), true)
@bp_test_no_allocations(C.b in C.instances(), true)
@bp_test_no_allocations(C.c in C.instances(), true)
@bp_test_no_allocations(C.a isa E_C, true)
@bp_test_no_allocations(C.b isa E_C, true)
@bp_test_no_allocations(C.c isa E_C, true)
# Test the other enums again, to make sure there wasn't a name collision.
@bp_test_no_allocations(A.a, A.from(0))
@bp_test_no_allocations(A.b, A.from(1))
@bp_test_no_allocations(A.c, A.from(2))
@bp_test_no_allocations(B.a, B.from(0))
@bp_test_no_allocations(B.b, B.from(20))
@bp_test_no_allocations(B.c, B.from(-10))
@assert(A.from("a") == A.a, "A from string")
@assert(A.from("b") == A.b, "A from string")
@assert(A.from("c") == A.c, "A from string")
@assert(B.from("a") == B.a, "B from string")
@assert(B.from("b") == B.b, "B from string")
@assert(B.from("c") == B.c, "B from string")
@bp_test_no_allocations(length(A.instances()), 3)
@bp_test_no_allocations(A.a in A.instances(), true)
@bp_test_no_allocations(A.b in A.instances(), true)
@bp_test_no_allocations(A.c in A.instances(), true)
@bp_test_no_allocations(length(B.instances()), 3)
@bp_test_no_allocations(B.a in B.instances(), true)
@bp_test_no_allocations(B.b in B.instances(), true)
@bp_test_no_allocations(B.c in B.instances(), true)
@bp_test_no_allocations(A.a isa E_A, true)
@bp_test_no_allocations(A.b isa E_A, true)
@bp_test_no_allocations(A.c isa E_A, true)
@bp_test_no_allocations(B.a isa E_B, true)
@bp_test_no_allocations(B.b isa E_B, true)
@bp_test_no_allocations(B.c isa E_B, true)

# Test the ability of enums to reference dependencies.
module M
    const MyInt = Int8
    export MyInt
    using Bplus.Utilities

    macro make_custom_enum(name, args...)
        return Utilities.generate_enum(name, quote using ..M end, args)
    end
    @make_custom_enum(D::MyInt,
        a=5,
        b=1,
        c=-1,
        d=0
    )
end
@bp_test_no_allocations(M.D.a, M.D.from(5))
@bp_test_no_allocations(M.D.b, M.D.from(1))
@bp_test_no_allocations(M.D.c, M.D.from(-1))
@bp_test_no_allocations(M.D.d, M.D.from(0))