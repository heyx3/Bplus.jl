# Test that vector constructors work as expected.
@check(Vec(1, 2, 3) isa Vec{3, Int})
@check(Vec(1, 2.0, UInt8(3)) isa Vec{3, Float64})
@check(Vec{Float32}(1, -2, 3) isa Vec{3, Float32})

# Test equality.
@check(Vec(1, 2, 3) == Vec(1, 2, 3))
@check(Vec(1, 2, 3) == Vec(1, 2.0, 3))

# Test the dot product, including that it doesn't allocate.
vdot(Vec(1, 2, 3), Vec(4, 5, 6))
@check(@timed(vdot(Vec(1, 2, 3), Vec(4, 5, 6))).bytes == 0,
       "It appears vdot() is allocating")
@check(vdot(Vec(1, 2), Vec(4, 5)) === 14)
@check(Vec(1, 2) ⋅ Vec(4, 5) === vdot(Vec(1, 2), Vec(4, 5)))

# Test swizzling.
@check(Vec(1, 2, 3, 4, 5).x === 1)
@check(Vec(1, 2, 3, 4, 5).y === 2)
@check(Vec(1, 2, 3, 4, 5).z === 3)
@check(Vec(1, 2, 3, 4, 5).w === 4)
@check(Vec(1, 2, 3, 4).xyz === Vec(1, 2, 3))
@check(Vec(1, 2, 3, 4).xyz0 === Vec(1, 2, 3, 0))
@check(Vec(1, 2, 3, 4).xyz1 === Vec(1, 2, 3, 1))
@check(Vec(1, 2, 3, 4).xxyyz0101w === Vec(1, 1, 2, 2, 3, 0, 1, 0, 1, 4))
@check(Vec(1, 2, 3, 4, 5).r === 1)
@check(Vec(1, 2, 3, 4, 5).g === 2)
@check(Vec(1, 2, 3, 4, 5).b === 3)
@check(Vec(1, 2, 3, 4, 5).a === 4)