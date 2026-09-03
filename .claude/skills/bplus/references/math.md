# BplusCore.Math

Game math. Everything here is immutable, stack-allocatable, and designed to be allocation-free — `@bp_test_no_allocations` is used heavily on this module.

## Scalar functions

`lerp(a, b, t)`, `inv_lerp(a, b, x)`, `smoothstep([a, b, ]t)`, `smootherstep([a, b, ]t)`, `fract(f)` (negatives wrap: `fract(-0.1) ≈ 0.9`), `saturate(x)` = `clamp(x,0,1)`, `square(x)`.

`typemin_finite(T)` / `typemax_finite(T)` — like `typemin`/`typemax` but finite for floats.

`round_up_to_multiple(v, multiple)` — works on integers and `Vec{<:Integer}`.

`wraparound(a, b, x)` — wraps `x` into `[a, b)`; degrades gracefully when `a == b`.

`solve_quadratic(a, b, c)` — `nothing`, or the two roots.

---

# `Vec{N, T}`

An N-dimensional vector of T components, backed by `data::NTuple{N, T}`. Deliberately *not* an alias for `StaticArrays.SVector` — aliasing would make most of the operator overloads type piracy.

## Aliases

By dimension: `Vec2{T}`, `Vec3{T}`, `Vec4{T}`.
By component type: `VecF{N}`, `VecD{N}`, `VecI{N}`, `VecU{N}`, `VecB{N}`; `VecT{T, N}` puts the type first (`VecT{Float32} == VecF`).
By both: `v2f v3f v4f` (Float32), `v2d v3d v4d`, `v2i v3i v4i` (Int32), `v2u v3u v4u` (UInt32), `v2b v3b v4b` (Bool).
By intent: `vRGB{T}`/`vRGBA{T}`, with `vRGBu8`, `vRGBi8`, `vRGBu`, `vRGBi`, `vRGBf` and the `vRGBA*` equivalents.

## Construction

| Form | Notes |
| --- | --- |
| `v3f()` | all zeros |
| `Vec(1.5, 2)` | components promoted to a common type → `v2d` |
| `Vec{T}(components...)` / `Vec{N,T}(components...)` | casts; the 2-param form also checks the count |
| `Vec(i -> i*2, 3)` | lambda per component (1-based) → `Vec(2,4,6)` |
| `Vec(f, Val(3))`, `Vec{N,T}(f)` | compile-time-known size / inferred size + cast |
| `Vec(4, Val(0.5))`, `Vec{N}(Val(F))`, `Vec{N,T}(Val(F))` | fill with a constant |
| `Vec(data::NTuple)`, `Vec{T}(tuple)`, `Vec{N,T}(tuple)` | direct tuple, optionally cast/checked |
| `Vec{T}()`, `Vec{N,T}(::Tuple{})` | 0-dimensional; useful in recursive functions |

`vappend(...)` concatenates vectors and scalars: `vappend(Vec(1,2), 3, Vec(4,5,6)) == Vec(1,2,3,4,5,6)`.

Serializes like an array, so *StructTypes* and *JSON3* work.

## Components and swizzling

- `.x .y .z .w`, equivalently `.r .g .b .a`.
- Any combination swizzles: `Vec(55,66,77).xxzy == Vec(55,55,77,66)`.
- `v[2]` (1-based) and `v[1,1,3,2]` (tuple of indices, same as `.xxzy`).
- Swizzle constants: `0` inserts 0 (`v2f(3,10).x0y`), `1` inserts 1 (`.rgb1`), `Δ` (`\Delta`+Tab) inserts the component type's largest finite value (`vRGBu8(20,200,63).rgbΔ` → alpha 255), `∇` (`\del`+Tab) inserts the smallest.
- Ranges: `v[2:4]` works but is type-unstable; `v[Val(2:4)]` is type-stable.

## Math

Implements `zero one typemin typemax min max minmax abs round clamp floor ceil` and all arithmetic, bitwise, and comparison operators **component-wise** — with the two exceptions `==` and `!=`, which compare the whole vector and return one `Bool`. Use `vequal(a, b)` (or `map(==, a, b)`) for component-wise equality.

Vector-specific functions:

- `vdot(a, b)`, or the `⋅` operator (`\cdot`+Tab)
- `vcross(a, b)` (Vec3 only), or `×` (`\times`+Tab)
- `vdist_sqr`, `vdist`, `vlength_sqr`, `vlength`
- `vnorm(v)`, `v_is_normalized(v)`
- `vreflect(v, normal)`, `vrefract(v, normal, IoR)`
- `vbasis(forward, up)` → a `VBasis` with forward exactly as given and up as close as possible
- `vselect(a, b, t::VecB)` — component-wise pick, like a binary lerp (`lerp()` also works, marginally slower)
- `vindex(p, size)` multidimensional coord → flat index; `vindex(i::Int, size)` goes back
- `convert(v2f, Vec(3,4))` casts components, and also converts a `StaticArrays.SVector`
- `reinterpret(v2f, v2u(0xabcdef01, 0x12345678))` reinterprets bits

## Arrays and indexing

`Vec{N,T} <: AbstractVector{T}`, so most array functions work, and the ones that matter (`map`, etc.) are re-overloaded to return a `Vec` rather than a `Vector`.

**Broadcasting is the exception** — `.` operators treat `Vec` as a 1D array and produce an actual `Vector`. Avoid in hot paths.

`hcat`/`vcat`/`hvncat` are overloaded so a matrix literal of `Vec` stays a matrix of `Vec` instead of gaining a dimension.

**Index order is reversed** relative to intuition, because Julia arrays are column-major: in `arr[v]` for a 2D array, `v.x` is the row and `v.y` is the column. Use `TrueOrdering`:

```julia
a[TrueOrdering(v)] == a[reverse(v)] == a[v.z, v.y, v.x]
```

`vsize(arr, I=Int32)` gets an array's size as a vector — again in reversed order unless you pass `true_order=true`.

## Ranges

Colon syntax works with `Vec` to make multidimensional ranges:

```julia
one(v2i):vsize(a)          # every index of a 2D array
1:vsize(a)                 # mixing scalars and vectors; any dimensionality
1 : v2i(1, 2) : vsize(a)   # stepped: skip every other column
```

Intended for integer vectors. For float ranges use `Box`/`Interval` instead.

## Setfield

Vectors are immutable; "modifying" means copying. Use *Setfield*: `@set! v.x += 10` mutates the binding, `v2 = @set v1.x += 10` produces a copy. Normally optimizes down to a mutation.

**Trap:** `@set! my_class.v.x += 10` copies all of `my_class`, not just `my_class.v`.

## Coordinate system

Redefine two functions in your own project to set the engine-wide convention:

```julia
BplusCore.Math.get_up_axis() = 2        # default 3 (Z-up)
BplusCore.Math.get_right_handed() = false  # default true
```

This works because Julia inlines these trivial functions and recompiles callers when they're redefined. **Never** bind their result to a `const` — consts are not recomputed.

Functions that respect the setting: `v_rightward(forward, up)`, `get_up_vector(F=Float32)`, `get_horz_vector(i, F=Float32)`, `get_horz_axes()`, `to_3d(v, f=0)`, `get_vert(v)`.

Useful case: Dear ImGUI's 2D space is left-handed and arguably Y-up, so a GUI-first project may want to flip both.

## Printing

`BplusCore.Mat.VEC_N_DIGITS` global; `use_vec_digits()` to change temporarily; `show_vec` for a one-off.

---

# `Quaternion{F}`

Aliases `fquat` (Float32) and `dquat` (Float64). Immutable and stack-allocatable like `Vec`.

## Construction

4 components (individually, as a tuple, or as a `Vec4`); nothing (`fquat()` = identity); axis + angle in radians; a normalized start and end vector (rotation between them); a rotation matrix; a `VBasis` (needs to know what "un-rotated" means — defaults to the configured coordinate system); a start and end `VBasis`; or a sequence of quaternions in chronological order.

## Operations

- `q_apply(q, v)` rotates a `Vec`
- `a >> b` = "a then b"; `a << b` = "b then a"
- `lerp(a, b, t)` — cheap and fine for most uses
- `q_slerp(a, b, t)` — higher quality, use for animation
- `q_basis(q)` → the `VBasis` produced by the rotation; forward/up/right axes configurable by keyword
- `q_is_identity(q)`
- `q_axisangle(q)::Tuple{Vec3, F}`
- `q_mat3x3(q)`, `q_mat4x4(q)`
- `qnorm(q)`, `q_is_normalized(q)`; `*` and unary `-` behave as expected but you rarely want them directly

Printing is as axis + angle, digits controlled by `QUAT_AXIS_DIGITS` and `QUAT_ANGLE_DIGITS`; `use_quat_digits()` and `show_quat()` for temporary/one-off control.

---

# `Mat{C, R, F}`

Statically-sized matrix. Under the hood it *is* `StaticArrays.SMatrix`, but with the type parameters swapped to Column-then-Row to match OpenGL. Memory order (column-major) already agrees between the two.

`SMatrix` requires a fourth type parameter equal to `C*R`, so **declare new matrix types with the `@Mat(C, R, F)` macro** rather than writing `Mat{C,R,F}` by hand.

## Aliases

HLSL-style: prefix `f` (Float32) or `d` (Float64), suffix `[cols]x[rows]`, collapsible to one number when square. `fmat2x2`, `dmat3x4`, `fmat3`, `fmat4`. Also `Mat2{F}`, `Mat3{F}`, `Mat4{F}`.

## Operations (`m_` prefix)

- `m_apply_point(m, p)` / `m_apply_vector(m, v)` (the latter ignores translation)
- `m_apply_point_affine` / `m_apply_vector_affine` — faster; valid for affine matrices (world and view, but *not* perspective projection)
- `m_combine(a, b...)` multiplies in chronological order: "a, then b, then …"
- `m_invert(m)`, `m_transpose(m)` (or the `'` operator)
- `m_identity(C, R, F)`, `m_identityf(C, R)`, `m_identityd(C, R)`
- `m_to_mat4x4(m)`, `m_to_mat3x3(m)`
- `*` works with `Vec` and `Mat`. B+ is pre-multiplication (`M*v`); `v*M` is deliberately undefined.

## Transform constructors

`m3_translate(delta)`, `m4_translate(delta)`, `m_scale(v)` / `m_scale(f, N)`, `m3_rotateX(radians)` and friends (`m[3|4]_rotate[X|Y|Z]`), `m3_rotate(q)` / `m4_rotate(q)`, `m4_world(pos, rot, scale)`, `m4_look_at(cam_pos, target_pos, up)`, `m3_look_at(forward, up, right; [new basis])` (default target basis: +X right, +Y up, −Z forward), `m3_look_at(from::VBasis, to::VBasis)`, `m4_projection(near_clip, far_clip, aspect_width_over_height, fov_degrees)`, `m4_ortho(range::Box3)`.

Component access mirrors a Julia multidimensional array — including the reversed `Vec` indexing caveat (X is the row).

---

# `Box{N, T}` and `Interval{T}`

Contiguous spaces and number ranges. `Interval` implements every `Box` function with scalars in place of vectors. `Box` is an `AbstractShape`; `Interval` is not.

Aliases follow the `Vec` pattern: `Box2Du`, `Box4Df`, `BoxT`, `IntervalI`, `IntervalU`, `IntervalF`, `IntervalD`.

## Construction

Give any two of `min`, `max` (inclusive), `size`, `center`:

```julia
Box2Df(min=v2f(0, 0), max=v2f(1.5, 2.2))
Box(@ano_value(Int), center=Vec(2, 3), size=Vec(5, 5))  # force integer division
Box(v2i(1, 2) : v2i(5, 5))                              # from a VecRange
Box2Di()                                                # min and size both 0
boundary(elements...)                                   # bounding box of points/boxes/shapes
```

One parameter may be a scalar meaning "same for every component"; if the `Box`'s type params are already fixed, both may be. When passing `center` + `size`, integer vs float division is chosen from the parameter types unless you disambiguate with `@ano_value(Int)` / `@ano_value(Float)`. Integer division with an even size puts `center` on the max side.

## Accessors and predicates

`min_inclusive`, `max_exclusive`, `max_inclusive`, `min_exclusive` (the latter two mainly for integer boxes), `size` (a `Base.size` overload), `center`, `corners` (all 2^N of them).

`is_empty(box)` (volume ≤ 0), `is_inside(box, point)` (strictly inside, not touching — integer boxes mainly), `contains(outer, inner)`, `Base.intersect(boxes...)` (check `is_empty` on the result), `Base.reshape(box, new_dimensionality; ...)` (added dimensions take a min and size by keyword).

## Array slicing

Integer boxes and intervals slice multidimensional arrays: `myArr[Box(min=v3i(3,4,5), max=v3i(5,5,5))]` == `myArr[3:5, 4:5, 5:5]`. Get a box over an array's indices with `box_indices(a, I=Int)`.

Boxes serialize/deserialize from any constructor pair, e.g. the JSON `{ "min": [1, 2], "size": [5, 5] }` reads as a `Box2Df` (`max` is inclusive).

---

# Rays and shapes

`Ray{N, F<:AbstractFloat}`, constructed from a start position and a direction. Aliases `Ray2D{F}`/`Ray3D{F}`/`Ray4D{F}`, and `Ray2`/`Ray3`/`Ray4` for Float32.

- `ray_at(ray, t)` — the point at distance `t`, scaled by the direction's magnitude
- `closest_point(ray, pos; min_t=0, max_t=typemax_finite(F))::F` — returns the `t`, clamped to a real ray by default

`AbstractShape{N, F}` interface: `volume`, `bounds` → `Box`, `center`, `is_touching(s, p)`, `closest_point(s, p)`, `intersections(s, r::Ray; ...)::UpTo{M, F}`, `collides(a, b)`.

`intersections(shape, ray)` returns `UpTo{N,F}`. Pass `Val(true)` as a third argument to also get the surface normal at the closest hit, changing the return to `Tuple{UpTo{N,F}, Vec3{F}}`.

**Status:** shapes are a work in progress — much is untested or unimplemented (`collides` especially), and the API may change. `Box` is the solid, fully-supported member.

---

# Curves

`Curve{T}` — an animated value of type `T` over a time range, optionally with Perlin noise for organic variation, optionally looping.

Built from `CurveKey{T}` keyframes: a timestamp, a value, and the slope carrying it to the *next* key (so the last key's slope only matters when looping). A key can also be given as a single `Vec` where X is the time and YZ… the value, when the value is scalar or `Vec`.

Construct from a `Vector` of keys or from keys as arguments. Evaluate with `curve_eval(curve, t, perlin_seeds_tuple=(0x1,); looping::Bool=false)`. After mutating a curve, call `curve_sanitize!(c)` to restore chronological order.

## Slopes (`CurveSlope` union)

- `CurveLinearSlope`
- `CurveExponentialSlope` — `t = pow(t, exponent)` before interpolating
- `CurveBezierSlope(a, b)` — Bezier control points at each end; e.g. `(-2, 2)` meanders away from the target first, then overshoots and doubles back

Slopes only remap the interpolant `t`, never the value. Two consequences: they're agnostic to what data is being interpolated, and the output can never leave the range spanned by the keyframes, no matter how strong the noise.

## Perlin variation

Configured by `CurvePerlin` at construction: a scale in `t` units (scale 2 → gradients at t=0,2,4,…), plus fade-in/fade-out windows with a duration and exponent. Pass the *same* seed tuple on every `curve_eval` call — that's what makes the variation smooth rather than random per-sample. On a looping curve the window applies only near the start, and the noise itself does not loop.

## Making your own type curve-able

- `curve_coord_lerp(a::T, b::T, t::Float32)::T` — defaults to `lerp()`, and to `q_slerp` for quaternions
- `curve_print(io::IO, x::T)` — defaults to `print`

A Dear ImGUI editor for `Curve{T}` is planned, not implemented.

---

# Random generation and noise

- `rand_in_sphere(u, v, r)` — uniform point in the unit sphere from three uniform values; `rand_in_sphere([rng][, F=Float32])` draws them for you.
- `perlin(pos::Union{Real, Vec})` — N-dimensional Perlin noise. Highly configurable; read the doc-string.

See `references/utilities.md` for `PRNG` and `ConstPRNG`, the RNGs you actually want for procedural generation.

---

# `Contiguous{T}`

An alias matching any nested container of `T` that is contiguous in memory. A `Vector{v2i}` is both a `Contiguous{Int32}` and a `Contiguous{v2i}` — which is exactly why `set_uniforms` has an overload taking an explicit element type.

`contiguous_length(data, T)`, `contiguous_ref(values, T, idx=1)`, `contiguous_ptr(values, T, idx=1)`.
