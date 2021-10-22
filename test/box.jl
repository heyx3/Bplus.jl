# Test constructors/equality.
@bp_test_no_allocations(Box(v2i), Box(v2f(), v2u()))
@bp_test_no_allocations(Box_minmax(Vec(3, 4), Vec(4, 10)),
                        Box2D{Int}(Vec(3, 4), Vec(2, 7)))
@bp_test_no_allocations(Box_minsize(Vec(3, 4), Vec(4, 10)),
                        Box2D{Int}(Vec(3, 4), Vec(4, 10)))
@bp_test_no_allocations(Box_maxsize(Vec(3, 4), Vec(2, 3)),
                        Box(Vec(2, 2), Vec(2, 3)))
@bp_test_no_allocations(Box_bounding(Vec(2, 3)),
                        Box(Vec(2, 3), Vec(1, 1)))
@bp_test_no_allocations(Box_bounding(Vec(5, 9), Vec(0, 0), Vec(120, -5)),
                        Box(Vec(0, -5), Vec(121, 15)))
@bp_test_no_allocations(Box_bounding(( Vec(5, 9), Vec(0, 0), Vec(120, -5) )),
                        Box(Vec(0, -5), Vec(121, 15)))

#TODO: The other unit tests