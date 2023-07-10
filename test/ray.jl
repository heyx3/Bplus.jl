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

# Ray-Plane
const PL = Plane{3, Float32}(v3f(), v3f(0, 0, 1))
@bp_test_no_allocations(
    intersections(PL, Ray(v3f(0, 0, 1), vnorm(v3f(1, 1, 1)))),
    ()
)
const RY = Ray(v3f(0, 0, 1), vnorm(v3f(1, 1, -1)))
const INTERS = intersections(PL, RY)
@bp_check(length(INTERS) == 1, "No intersection between ", PL, " and ", RY)
@bp_check(isapprox(INTERS[1], sqrt(3), atol=0.001),
          "Intersection point is ", INTERS[1], ", not ", sqrt(3))


println("#TODO: Do raycasts within Unity, then translate those cases into unit tests")