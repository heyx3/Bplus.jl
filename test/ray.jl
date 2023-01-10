# Ray-Box
@bp_test_no_allocations(
    intersect(Box_minmax(-one(v3f), one(v3f)),
              Ray(v3f(), v3f(1, 0, 0))),
    (1.0, )
)
@bp_test_no_allocations(
    intersect(Box_minmax(-one(v3f), one(v3f)),
              Ray(v3f(-2, 0, 0), v3f(1, 0, 0))),
    (1.0, 3.0)
)
@bp_test_no_allocations(
    intersect(Box_minmax(v2f(5.6, -20.103),
                         v2f(10.6, -35.21)),
              Ray(v2f(4.6, -27.0), vnorm(v2f(-1, 1)))),
    ()
)

# Ray-Sphere
@bp_test_no_allocations(
    intersect(Sphere3(v3f(), 1.0),
              Ray(v3f(), v3f(1, 0, 0))),
    (1.0, )
)
@bp_test_no_allocations(
    intersect(Sphere3(v3f(), 1.0),
              Ray(v3f(-2, 0, 0), v3f(1, 0, 0))),
    (1.0, 3.0)
)
@bp_test_no_allocations(
    intersect(Sphere2(v2f(5.6, -20.103), 3.4765),
              Ray(v2f(), vnorm(v2f(-1, 1)))),
    ()
)