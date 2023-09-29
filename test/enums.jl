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

# Test bitflags.
@bp_bitflag D a b c
@test_enum D 1 2 4
@bp_test_no_allocations(Int(D.a | D.b), 3)
@bp_test_no_allocations(contains(D.a, D.a), true)
@bp_test_no_allocations(contains(D.a, D.b), false)
@bp_test_no_allocations(contains(D.a, D.c), false)
@bp_test_no_allocations((D.a | D.b) - D.b, D.a)
@bp_test_no_allocations(D.ALL, D.a | D.b | D.c)
@bp_test_no_allocations(((D.a | D.b) & D.b), D.b)
@bp_test_no_allocations(D.a <= D.a, true)
@bp_test_no_allocations(D.a <= D.b, false)
@bp_test_no_allocations(D.a <= (D.a | D.c), true)
@bp_test_no_allocations((D.a | D.c) >= D.c, true)
# Test other bitflag features.
@bp_bitflag E::UInt8 z=0 a b c
@bp_test_no_allocations(Int.((E.z, E.a, E.b, E.c)), (0, 1, 2, 4))
@bp_test_no_allocations(E.ALL, E.a | E.b | E.c)
# Test aggregates.
@bp_bitflag F a b c @ab(a|b) @ac(a|c) @bc(b|c) @abc(ab|c)
@bp_test_no_allocations(F.ab, F.a | F.b)
@bp_test_no_allocations(F.ac, F.a | F.c)
@bp_test_no_allocations(F.bc, F.b | F.c)
@bp_test_no_allocations(F.abc, F.ALL)

# Test the ability of enums to reference dependencies:
module M
    const MyInt = Int8
    export MyInt
    using Bplus.Utilities

    macro make_custom_enum(name, args...)
        return Utilities.generate_enum(name, quote using ..M end, args, false)
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