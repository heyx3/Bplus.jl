The module `Bplus.Math`.

## Functions

* `lerp(a, b, t)`, `smoothstep([a, b, ]t)`, and `smootherstep([a, b, ]t)` implement the common game interpolation functions. `smoothstep` and `smootherstep` optionally take just a `t` value (implicitly using `a=0` and `b=1`).
* `inv_lerp(a, b, x)` performs the inverse of `lerp`: `lerp(a, b, inv_lerp(a, b, x) == x`.
* `fract(f)` gets the fractional part of `f`. Negative values wrap around; for example `fract(-0.1) == 0.9` within floating-point error.
* `saturate(x)` is equal to `clamp(x, 0, 1)`.
* `square(x)` is equal to `x*x`.
* `typemin_finite(T)` and `typemax_finite(T)` are an alternative to Julia's `typemin(T)` and `typemax(T)` that returns finite values for float types.
* `round_up_to_multiple(v, multiple)` rounds an integer (or `Vec{<:Integer}`) up to the next multiple of some other integer (or `Vec{<:Integer}`). For example, `round_up_to_multiple(7, 5) == 10`.
* `solve_quadratic(a, b, c)` returns either `nothing` or the two solutions to the quadratic equation `ax^2 + bx + c = 0`.

## Vectors, Matrices, Quaternions

These objects are described in separate documents.

* [`Vectors`](Vec.md)
* [`Quaternions`](Quat.md)
* [`Matrices`](Matrix.md)

## Rays

`Ray{N, F<:AbstractFloat}` defines an `N`-dimensional ray. You can construct it with a start position and direction vector.

Aliases exist for dimensionality: `Ray2D{F}`, `Ray3D{F}`, `Ray4D{F}`.

Aliases also exist assuming Float3: `Ray2`, `Ray3`, `Ray4`.

* `ray_at(ray, t)` gets the point on the ray at the given distance from the origin (proportional to the ray direction's magnitude).
* `closest_point(ray, pos; min_t=0, max_t=typemax_finite(F))::F` gets the point along the ray (as its `t` scalar value) which is closest to the given position. By default, limits the output to be >= 0 like a true ray, but you can change the allowed range of `t` values.

Ray intersection calculations can be found in the [Shapes](#Shapes) section of this document.

## Box and Interval

Contiguous spaces can be represented by `Box{N, T}`, and contiguous number ranges represented by `Interval{T}`. All `Box` functions are also implemented by `Interval`, using scalar data instead of vector data.

`Box` is part of the `AbstractShape` system (see the [Shapes](#Shapes) section), but this section focuses on the more general utility of `Box` (and `Interval`).

Like `Vec`, boxes have many aliases based on type and dimensionality. For example, `Box2Du`, `Box4Df`, `BoxT`. Intervals have aliases for common types: `IntervalI`, `IntervalU`, `IntervalF`, and `IntervalD`.

You can construct a box with any two of `min`, `max` (inclusive), `size`, and `center`. For example, `Box2Df(min=v2f(0, 0), max=v2f(1.5, 2.2))`. You can also construct one using `boundary(elements...)`, which computes the bounding box of several points and boxes (and other `AbstractShape` types).

You can get the properties of a box with the following getters:
  * `min_inclusive(box)`
  * `max_exclusive(box)`
  * `max_inclusive(box)` (only really useful for integer-type boxes)
  * `min_exclusive(box)` (only really useful for integer-type boxes)
  * `size(box)` (a new overload of the built-in function `Base.size()`)
  * `center(box)`

Some other box/interval functions (again, not including the `AbstractShape` interface) are:
  * `is_empty(box)::Bool` returns whether the box's volume is <= 0
  * `is_inside(box, point)::Bool` returns whether the point is inside the box, not touching its edges. This is only really useful for integer-type boxes.
  * `contains(outer_box, inner_box)::Bool` checks whether the first box totally contains the second.
  * `Base.intersect(boxes::Box...)::Box` gets the intersection of one or more boxes. You can call `is_empty(result)` to check if there is no intersection.
  * `Base.reshape(box, new_dimensionality::Integer; ...)` removes or adds dimensions to the box. Added dimensions have a specific min and size (specified in named parameters).

Boxes can be serialized and deserialized. Deserialization can come from any pair of constuctor parameters: `min`, `max`, `size`, `center`. For example, you can deserialize a Box2Df from this JSON string using the JSON3 package: `"{ "min": [ 1, 2 ], "size": [5, 5] }"`.

## Shapes

`AbstractShape{N, F}` is some N-dimensional shape using numbers of type `F`. Its interface is as follows:

* `volume(s)::F`
* `bounds(s)::Box{N, F}` (Box is a specific type of `AbstractShape`, described below)
* `center(s)::Vec{N, F}`
* `is_touching(s, p::Vec{N, F})::Bool`
* `closest_point(s, p::Vec{N, F})::Vec{N, F}`
* `intersections(s, r::Ray{N, F}; ...)::UpTo{M, F}`
* `collides(a::AbstractShape{N, F}, b::AbstractShape{N, F})::Bool`
  * **Important note**: mostly unimplemented as of time of writing

**Important note**: shapes are still under development, so significant chunks of it are untested or unimplemented, and the API is a work-in-progress.

### Ray intersections

The interface for ray intersections is a bit wonky (and, as mentioned above, still under development).

By default, `intersections(shape, ray)` returns `UpTo{N, F}` intersections. If you also want the surface normal at the closest intersection, pass a third parameter `Val(true)`. In this case, it instead returns a `Tuple{UpTo{N, F}, Vec3{F}}`.

### Box

Box, [as mentioned above](#Box-And-Interval), is a particularly useful shape. For more info on the rest of its interface, unrelated to `AbstractShape`, see the linked section. Note that `Interval` does *not* implement `AbstractShape`.

## Contiguous

`Contiguous{T}` is an alias for any nested container of `T` that appears to be contiguous in memory. For example, a `Vector{v2i}` is both a `Contiguous{Int32}` and a `Contiguou{v2i}`.

The number of `T` in a `Contiguous{T}` is found with `contiguous_length(data, T)`.

Contiguous data can be turned into a `Ref` for interop using `contiguous_ref(values, T, idx=1)`. It can also be turned directly into a pointer with `coniguous_ptr(values, T, idx=1)`.

## Random Generation

* `rand_in_sphere(u, v, r)` generates a uniform-random `Vec3` within the unit sphere, given three uniform-random values.
  * To generate the three values from an RNG, call `rand_in_sphere([rng][, F = Float32])`.
* `perlin(pos::Union{Real, Vec})` generates N-dimensional Perlin noise.
  * This function is highly customizable; refer to its doc-string.