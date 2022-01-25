# Test @ano_enum:
@bp_test_no_allocations(@ano_enum(ABC, DEF, GHI),
                        Union{Val{:ABC}, Val{:DEF}, Val{:GHI}})


# Define some enums with the same value names.
# Check that they work as expected, and that
#    the new enums' values don't overwrite the old.

macro test_enum(E::Symbol, a_val, b_val, c_val)
    return quote
        @bp_test_no_allocations($E.a isa $(Symbol(:E_, E)), true)
        @bp_test_no_allocations($E.b isa $(Symbol(:E_, E)), true)
        @bp_test_no_allocations($E.c isa $(Symbol(:E_, E)), true)

        @bp_test_no_allocations($E.instances(), ($E.a, $E.b, $E.c))

        @bp_test_no_allocations($E.from($a_val), $E.a)
        @bp_test_no_allocations($E.from($b_val), $E.b)
        @bp_test_no_allocations($E.from($c_val), $E.c)

        @bp_test_no_allocations($E.from_index(1), $E.a)
        @bp_test_no_allocations($E.from_index(2), $E.b)
        @bp_test_no_allocations($E.from_index(3), $E.c)

        @bp_test_no_allocations($E.to_index($E.a), 1)
        @bp_test_no_allocations($E.to_index($E.b), 2)
        @bp_test_no_allocations($E.to_index($E.c), 3)

        @bp_check($E.from("a") == $E.a, $E, " from string")
        @bp_check($E.from("b") == $E.b, $E, " from string")
        @bp_check($E.from("c") == $E.c, $E, " from string")
    end
end

@bp_enum A a b c
@test_enum A 0 1 2

@bp_enum B a b=20 c=-10
@test_enum B 0 20 -10
# Test the other enum again, to make sure there weren't any name collisions.
@test_enum A 0 1 2

@bp_enum C::UInt8 a b=255 c=1
@test_enum C 0 255 1
# Test the other enum again, to make sure there weren't any name collisions.
@test_enum A 0 1 2
@test_enum B 0 20 -10


# Test the ability of enums to reference dependencies:
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