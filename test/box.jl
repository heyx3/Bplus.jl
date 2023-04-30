# Test constructors/equality.
@bp_test_no_allocations(Box(v2i), Box(v2f(), v2u()))
@bp_test_no_allocations(Box(1:20), Box(1, 20))
@bp_test_no_allocations(Box(1:Vec(21, 43)), Box(Vec(1, 1), Vec(21, 43)))
@bp_test_no_allocations(Box(2, Vec(5, 6, 7)),
                        Box(Vec(2, 2, 2), Vec(5, 6, 7)))
@bp_test_no_allocations(Box_minmax(Vec(3, 4), Vec(4, 10)),
                        Box2D{Int}(Vec(3, 4), Vec(2, 7)))
@bp_test_no_allocations(Box_minsize(Vec(3, 4), Vec(4, 10)),
                        Box2D{Int}(Vec(3, 4), Vec(4, 10)))
@bp_test_no_allocations(Box_maxsize(Vec(3, 4), Vec(2, 3)),
                        Box(Vec(2, 2), Vec(2, 3)))
@bp_test_no_allocations(Box_centersize(Vec(2, 2), Vec(4, 4)),
                        Box(Vec(0, 0), Vec(4, 4)))
@bp_test_no_allocations(Box_bounding(Vec(2, 3)),
                        Box(Vec(2, 3), Vec(1, 1)))
@bp_test_no_allocations(Box_bounding(Vec(5, 9), Vec(0, 0), Vec(120, -5)),
                        Box(Vec(0, -5), Vec(121, 15)))
@bp_test_no_allocations(Box_bounding(Box(1:20)),
                        Box(1:20))
@bp_test_no_allocations(Box_bounding(Box(1:20), Vec(-1, 30)),
                        Box_minmax(Vec(-1, 1), Vec(20, 30)))
@bp_test_no_allocations(Box_bounding(Vec(-1, 30), Box(1:20)),
                        Box_minmax(Vec(-1, 1), Vec(20, 30)))
@bp_test_no_allocations(Box_bounding(Vec(-1, 30), Box(1:20), Vec(21, -10)),
                        Box_minmax(Vec(-1, -10), Vec(21, 30)))
@bp_test_no_allocations(Box_bounding(Box(1:20), Vec(-1, 30), Vec(21, -10)),
                        Box_minmax(Vec(-1, -10), Vec(21, 30)))
@bp_test_no_allocations(Box_bounding(Vec(-1, 30), Vec(21, -10), Box(1:20)),
                        Box_minmax(Vec(-1, -10), Vec(21, 30)))
@bp_test_no_allocations(Box_bounding(Union{Int, v3f}[ v3f(1, -5, 10), 0, v3f(1.5, 50, -3) ]),
                        Box_minmax(v3f(0, -5, -3), v3f(1.5, 50, 10)))

# Test conversions.
@bp_test_no_allocations(convert(Box{v2u}, Box(v2f(1, 2), v2f(4, 3))) isa Box{v2u},
                        true)

#TODO: Test the use of aliases.

@bp_test_no_allocations(is_empty(Box(v2i)), true)
@bp_test_no_allocations(is_empty(Box_minmax(Vec(1, 1), Vec(10, 1))), false)

@bp_test_no_allocations(get_dimensions(Box(v2i)), 2)
@bp_test_no_allocations(get_dimensions(Box(v4f)), 4)
@bp_test_no_allocations(get_dimensions(Box(Float64)), 1)

@bp_test_no_allocations(get_number_type(Box(v4f)), Float32)
@bp_test_no_allocations(get_number_type(Box(v4u)), UInt32)
@bp_test_no_allocations(get_number_type(Box(Int8)), Int8)

@bp_test_no_allocations(max_inclusive(Box_minmax(1, 3)), 3)
@bp_test_no_allocations(max_inclusive(Box_minmax(Vec(3, 4, 5), Vec(100, 200, 3000))),
                        Vec(100, 200, 3000))

@bp_test_no_allocations(max_exclusive(Box_minmax(1, 3)), 4)
@bp_test_no_allocations(max_exclusive(Box_minmax(Vec(3, 4, 5), Vec(100, 200, 3000))),
                        Vec(101, 201, 3001))

@bp_test_no_allocations(volume(Box_minsize(Vec(-30, 200, 1), Vec(4, 5, 600))),
                        4 * 5 * 600)

@bp_test_no_allocations(is_touching(Box(3:20), 3), true)
@bp_test_no_allocations(is_touching(Box(3:20), 20), true)
@bp_test_no_allocations(is_touching(Box(3:20), 17), true)
@bp_test_no_allocations(is_touching(Box(3:20), 2), false)
@bp_test_no_allocations(is_touching(Box(3:20), 21), false)
@bp_test_no_allocations(is_touching(Box(3:20), -9999), false)

@bp_test_no_allocations(is_inside(Box(3:20), 3), false)
@bp_test_no_allocations(is_inside(Box(3:20), 20), false)
@bp_test_no_allocations(is_inside(Box(3:20), 17), true)
@bp_test_no_allocations(is_inside(Box(3:20), 2), false)
@bp_test_no_allocations(is_inside(Box(3:20), 21), false)
@bp_test_no_allocations(is_inside(Box(3:20), -9999), false)

@bp_test_no_allocations(intersect(Box(Vec(1, 1):Vec(40, 40)),
                                  Box(Vec(10, -5):Vec(23, 100))),
                        Box(Vec(10, 1):Vec(23, 40)))

#TODO: Math.closest_point, reshape