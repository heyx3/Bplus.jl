# Ray-Box
@bp_test_no_allocations(
    intersections(Box((min=-one(v3f), max=one(v3f))),
                  Ray(v3f(), v3f(1, 0, 0))),
    (1.0, )
)
@bp_test_no_allocations(
    intersections(Box((min=-one(v3f), max=one(v3f))),
                  Ray(v3f(-2, 0, 0), v3f(1, 0, 0))),
    (1.0, 3.0)
)
@bp_test_no_allocations(
    intersections(Box((min=v2f(5.6, -20.103),
                       max=v2f(10.6, -35.21))),
                  Ray(v2f(4.6, -27.0), vnorm(v2f(-1, 1)))),
    ()
)

# Ray-Sphere
@bp_test_no_allocations(
    intersections(Sphere3D{Float32}(v3f(), 1.0),
                  Ray(v3f(), v3f(1, 0, 0))),
    (1.0, )
)
@bp_test_no_allocations(
    intersections(Sphere3D{Float32}(v3f(), 1),
                  Ray(v3f(-2, 0, 0), v3f(1, 0, 0))),
    (1.0, 3.0)
)
@bp_test_no_allocations(
    intersections(Sphere2D{Float32}(v2f(5.6, -20.103), 3.4765),
                  Ray(v2f(), vnorm(v2f(-1, 1)))),
    ()
)

println("#TODO: Do raycasts within Unity, then translate those cases into unit tests")