# Test constructors/equality.
@bp_test_no_allocations(Box{2, Int32}(), Box(v2f(), v2u()))
@bp_test_no_allocations(Box(1:20), Box(1, 20))
@bp_test_no_allocations(Box(1:Vec(21, 43)), Box(Vec(1, 1), Vec(21, 43)))
@bp_test_no_allocations(Box(2, Vec(5, 6, 7)),
                        Box(Vec(2, 2, 2), Vec(5, 6, 7)))
@bp_test_no_allocations(Box((min=Vec(3, 4), max=Vec(4, 10))),
                        Box2D{Int}(Vec(3, 4), Vec(2, 7)))
@bp_test_no_allocations(Box((min=Vec(3, 4), size=Vec(4, 10))),
                        Box2D{Int}(Vec(3, 4), Vec(4, 10)))
@bp_test_no_allocations(Box((max=Vec(3, 4), size=Vec(2, 3))),
                        Box(Vec(2, 2), Vec(2, 3)))
@bp_test_no_allocations(Box((center=Vec(2, 2), size=Vec(4, 4))),
                        Box(Vec(0, 0), Vec(4, 4)))
@bp_test_no_allocations(boundary(Vec(2, 3)),
                        Box(Vec(2, 3), Vec(1, 1)))
@bp_test_no_allocations(boundary(Vec(5, 9), Vec(0, 0), Vec(120, -5)),
                        Box(Vec(0, -5), Vec(121, 15)))
@bp_test_no_allocations(boundary(Box(1:20)),
                        Box(1:20))
@bp_test_no_allocations(boundary(Vec(-1, 30), Vec(21, -10), Box(Vec(1, 1):Vec(20, 20))),
                        Box((min=Vec(-1, -10), max=Vec(21, 30))))
@bp_test_no_allocations(boundary(Union{Int, v3f}[ v3f(1, -5, 10), 0, v3f(1.5, 50, -3) ]),
                        Box((min=v3f(0, -5, -3), max=v3f(1.5, 50, 10))))

# Test conversions.
@bp_test_no_allocations(convert(Box2Du, Box(v2f(1, 2), v2f(4, 3))) isa Box2Du,
                        true)

@bp_test_no_allocations(is_empty(Box2Di()), true)
@bp_test_no_allocations(is_empty(Box((min=Vec(1, 1), max=Vec(10, 1)))), false)

@bp_test_no_allocations(shape_dimensions(Box2Di()), 2)
@bp_test_no_allocations(shape_dimensions(Box{4, Float32}()), 4)

@bp_test_no_allocations(shape_ntype(Box{4, Float32}()), Float32)
@bp_test_no_allocations(shape_ntype(Box{4, UInt32}()), UInt32)

@bp_test_no_allocations(max_inclusive(Box((min=1, max=3))), 3)
@bp_test_no_allocations(max_inclusive(Box((min=Vec(3, 4, 5), max=Vec(100, 200, 3000)))),
                        Vec(100, 200, 3000))

@bp_test_no_allocations(max_exclusive(Box((min=1, max=3))), 4)
@bp_test_no_allocations(max_exclusive(Box((min=Vec(3, 4, 5), max=Vec(100, 200, 3000)))),
                        Vec(101, 201, 3001))

@bp_test_no_allocations(volume(Box((min=Vec(-30, 200, 1), size=Vec(4, 5, 600)))),
                        4 * 5 * 600)

@bp_test_no_allocations(is_touching(Box(3:20), 3), true)
@bp_test_no_allocations(is_touching(Box(3:20), 20), true)
@bp_test_no_allocations(is_touching(Box(3:20), 17), true)
@bp_test_no_allocations(is_touching(Box(3:20), 2), false)
@bp_test_no_allocations(is_touching(Box(3:20), 21), false)
@bp_test_no_allocations(is_touching(Box(3:20), -9999), false)

@bp_test_no_allocations(is_inside(Box(3:20), Vec(3)), false)
@bp_test_no_allocations(is_inside(Box(3:20), Vec(20)), false)
@bp_test_no_allocations(is_inside(Box(3:20), Vec(17)), true)
@bp_test_no_allocations(is_inside(Box(3:20), Vec(2)), false)
@bp_test_no_allocations(is_inside(Box(3:20), Vec(21)), false)
@bp_test_no_allocations(is_inside(Box(3:20), Vec(-9999)), false)

@bp_test_no_allocations(intersect(Box(Vec(1, 1):Vec(40, 40)),
                                  Box(Vec(10, -5):Vec(23, 100))),
                        Box(Vec(10, 1):Vec(23, 40)))

#TODO: closest_point, reshape

# Interval tests.
@bp_test_no_allocations(typeof(Interval(1:10)), Interval{Int})
const TEST_INTERVAL = Interval((min=1.5, size=10))
@bp_test_no_allocations(typeof(TEST_INTERVAL), Interval{Float64})
@bp_test_no_allocations(min_inclusive(TEST_INTERVAL), 1.5)
@bp_test_no_allocations(max_exclusive(TEST_INTERVAL), 1.5 + 10)
@bp_test_no_allocations(volume(TEST_INTERVAL), 10.0)
@bp_test_no_allocations(bounds(TEST_INTERVAL), TEST_INTERVAL.box)
@bp_test_no_allocations(closest_point(TEST_INTERVAL, 2.2), 2.2)
@bp_test_no_allocations(closest_point(TEST_INTERVAL, -2.2), 1.5)
@bp_test_no_allocations(is_touching(TEST_INTERVAL, 8.25), true)
@bp_test_no_allocations(is_touching(TEST_INTERVAL, 18.25), false)