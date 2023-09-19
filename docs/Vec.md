Part of the [`Bplus.Math` module](Math.md). The core game math types are: `Vec{N, T}`, [`Quaternion{F}`](Quat.md), and [`Mat{C, R, F}`](Matrix.md).

# `Vec{N, T}`

An `N`-dimensional vector of `T`-type components. Unlike Julia's built-in `Vector`, this is static, immutable, and most importantly allocatable on the stack.

Under the hood, it holds a tuple `data::NTuple{N, T}`.

## Aliases

### By dimension

* `Vec2{T}`
* `Vec3{T}`
* `Vec4{T}`

### By component type

* `VecF{N}` : `Float32`
* `VecD{N}` : `Float64`
* `VecI{N}` : `Int32`
* `VecU{N}` : `UInt32`
* `VecB{N}` : `Bool`
* `VecT{T, N}` allows you to specify a vector's component type before its dimension. For example, `VecT{Float32} == VecF`.

### By dimension *and* component type

* Float32:
  * `v2f`
  * `v3f`
  * `v4f`
* Float64:
  * `v2d`
  * `v3d`
  * `v4d`
* Int32:
  * `v2i`
  * `v3i`
  * `v4i`
* UInt32:
  * `v2u`
  * `v3u`
  * `v4u`
* Bool:
  * `v2b`
  * `v3b`
  * `v4b`

### By intent

* `vRGB{T} = Vec3{T}`, for 3-component data
  * `vRGBu8` for UInt8 channels
  * `vRGBi8` for Int8 channels
  * `vRGBu` for UInt32 channels
  * `vRGBi` for Int32 channels
  * `vRGBf` for Float32 channels
* `vRGBA{T} = Vec4{T}`, for 4-component data
  * `vRGBAu8` for UInt8 channels
  * `vRGBAi8` for Int8 channels
  * `vRGBAu` for UInt32 channels
  * `vRGBAi` for Int32 channels
  * `vRGBAf` for Float32 channels

## Construction

You can construct a vector in the following ways. Julia's type system is a bit tricky, so if you're having problems then come back to this cheat-sheet.

* `Vec{N, T}()` constructs an instance of specific size and component type, using all 0's. For example, `v3f()`.
* `Vec(components...)` pass in the individual components, which are all promoted to the same type. For example, `Vec(1.5, 2.7)` creates a `v2d`.
  * `Vec{T}(components...)` casts all elements to `T`.
  * `Vec{N, T}(components...)` casts all elements to `T` and throws an error if the wrong number of components is passed. For example, `v3f(3, 4, 5)` produces a `v3f` instead of a `Vec{3, Int64}`.
* `Vec(f::Callable, n::Int)` creates a vector of size `n` using the given lambda to map each component (1 through `n`) to a value. For example, `Vec(i->i*2, 3)` is equivalent to `Vec(2, 4, 6)`.
  * `Vec(f::Callable, ::Val{N})` uses a compile-time constant for the size. For example, `Vec(i->i*2, Val(3))`. Note that this *doesn't* speed things up unless `N` is actually a compile-time constant.
  * `Vec{N, T}(f::Callable)` infers the dimensionality and casts the components to `T`. For example, `v3f(i -> i*2)` makes a `v3f(2, 4, 6)`.
* `Vec(N, ::Val{F})` : constructs a vector with `N` components, of type `typeof(F)`, all set to the value `F`. For example, `Vec(4, Val(0.5))` is equivalent to `Vec4(0.5, 0.5, 0.5, 0.5)`.
  * `Vec{N}(::Val{F})` is a variant that takes `N` as a type parameter. For example, `Vec4(::Val(0.5))`.
  * `Vec{N, T}(::Val{F})` is a variant that also casts `F` to type `T`. For example, `v2f(Val(0.5))` makes a `v2f` instead of a `v2d`.
* `Vec(data::NTuple)` : directly pass in the ntuple of data.
  * `Vec{T}(data::NTuple)` is a variant that will cast each element of the tuple to `T`.
  * `Vec{N, T}(data::NTuple)` is a variant that will cast each element, and throw an error if the tuple has the wrong number of elements.
* `Vec{T}()` : constructs a 0-D vector of component type `T`

`vappend()` lets you combine vectors and scalars into bigger vectors. For example, `vappend(Vec(1, 2), 3, Vec(4, 5, 6)) == Vec(1, 2, 3, 4, 5, 6)`.

`Vec` is serializable like an array, so it supports the *StructTypes* package and therefore packages like *JSON3*.

## Components/Swizzling

You can get a tuple of a vector's components by accessing its `data` field.

You can access the individual components of a vector in several ways:
* Naming its `x`, `y`, `z`, or `w` component, also named `r`, `g`, `b`, and `a`. For example, `Vec(1, 2, 3).x == 1`.
* Combining components in any order to "swizzle" a new vector. For example, `Vec(55, 66, 77).xxzy == Vec(55, 55, 77, 66)`.
* Getting a single component using its index (1-based, like everything else in Julia). For example, `v[2] == v.y`.
* Accessing a tuple of component indices. For example, `v[1, 1, 3, 2] == v.xxzy`.

Along with the usual components mentioned above, you can swizzle with some special characters:
* `0` gets the value 0 for a component. For example, `v2f(3, 10).x0y` turns a 2D vector into 3D one with a Y value of 0.
* `1` gets the value 1 for a component. For example, `v3f(0.2, 0.8, 1.0).rgb1` appends an alpha of 1 to an RGB color.
* `Δ` (typed as '\Delta' followed by a Tab) gets the largest finite value for the component type. For example, `vRGBu8(20, 255, 63).rgbΔ` Adds an alpha of 255 to an RGB color.
* `∇` (typed as '\del' followed by a Tab) gets the smallest finite value for the component type. For example, `Vec2{Int8}(11, -23).xy∇` appends `-128` to the vector.

## Math

`Vec` implements many standard Julia functions for numbers:
* `zero()` and `one()`
* `typemin()` and `typemax()`
* `min()`, `max()`, and `minmax()`
* `abs()`, `round()`, `clamp()`

It implements all the standard math operators (including bitwise ops and comparisons), component-wise with two exceptions: `==` and `!=` are done for the whole `Vec`, producing a single boolean.

* For component-wise equality use `map`, for example `map(==, v1, v2)`.

Vector math is implemented with functions prefixed by `v`:
* `vdot(a, b)`
  * You can also use the operator '⋅' (typed as '\cdot' followed by a Tab): `a ⋅ b`
* `vcross(a, b)`
  * You can also use the operator '×' (typed as '\times' followed by a Tab): `a × b`
* `vdist_sqr(a, b)`, `vdist(a, b)`, `vlength_sqr(a, b)`, and `vlength(a, b)`
* `vnorm(v)`, `v_is_normalized(v)` for normalization
* `vreflect(v, normal)` and `vrefract(v, normal, IoR)`
* `vbasis(forward, up)` gets three orthogonal vectors, where the forward vector is exactly equal to `forward` and the up vector is as close as possible to `up`.

Vectors also interact nicely with Julia's multidimensional arrays:

* As an `AbstractVector`, `Vec` supports many standard array-processing functions (see [below](#Array-Like-Behavior)).
  * Note that `Vector` is Julia's term for a 1D array; don't get them confused.
* You can get an element of an array with an integer-vector index using `arr[vec]`.
  * Unfortunately, the indexing order of arrays is reversed, so in a 2D array, a `Vec` index's X coordinate is the row, and Y is the column. Fixing this in B+ would cause more problems than it solves IMO; you'd also have to reverse `vsize()`, and then tuples and vectors index differently...
* You can get the size of an array as a vector, with `vsize(arr)`

Some other general utilities:

* `vselect(a, b, t)` lets you pick values component-wise using a `VecB`, sort of like a binary `lerp()`.
* `Base.convert()` can convert the components of a `Vec`, for example `convert(v2f, Vec(3, 4))`. It can also convert a `StaticArrays.SVector` to a `Vec`.
* `Base.reinterpret()` can reinterpret the bits of a `Vec`'s components, for example `reinterpret(v2f, v2u(0xabcdef01, 0x12345678))`.

You can reinterpret the bits of a `Vec` as another component type of the same size with `Base.reinterpret()`.

## Setfield

Because vectors are value-types, they are immutable. The only way to "modify" a vector is to replace it with a copy. Fortunately, Julia has *Setfield*, a built-in package which helps you do this. Whlie it technically does lots of copying, use of *Setfield* will almost always optimize down to something equivalent to mutating your desired field.

You can "modify" a vector variable with the syntax `v = @set v.x += 10`, or just `@set! v.x += 10`. **However**, it's important to note that if you try to `@set` a vector that belongs to something else, like `@set! my_class.v.x += 10`, *Setfield* will try to make a copy of that entire owning object (`my_class`, not just `my_class.v`).

## Coordinate System

B+ allows you to define the engine-wide up axis and handedness of vector math by redefining `Bplus.Math.get_up_axis() = 2` (the default is `3` the Z axis) and `Bplus.Math.get_right_handed() = false` (defaults to `true`). For more info on how this works, see [How it works](#How-it-works) below.

An example of when this is useful is if you are primarily developing your project within `Dear ImGUI`'s graphics system rather than 3D rendering with `Bplus.GL`. Dear ImGUI is left-handed, and arguably Y-up.

There are several built-in functions that respect handedness and up vector:
  * `v_rightward(a, b)` does a cross-product based on the current handed-ness. In right-handed systems, `v_rightward(forward, up)` does `vcross(forward, up)`. In left-handed systems, it swaps them.
  * `get_up_vector(F = Float32)` gets the 3D up vector. For example, if the up axis is `3`, then it returns `Vec3{F}(0, 0, 1)`.
  * `get_horz_vector(i, F = Float32)` gets the first or second 3D horizontal vector, based on whether you pass in `1` or `2`.
    * `get_horz_axes()` gets a tuple of the two horizontal axes. For example, if the up-axis is `3` then it returns `(1, 2)`.
  * `to_3d(v, f=0)` inserts a vertical component to a 2D horizontal vector.
  * `get_vert(v)` grabs the vertical component of a 3D vector.

### How it works

Julia's blurry line between compile-time and run-time allows you to set up compile-time constants directly in the language. You can define a trivial function like `my_const() = 5`, which Julia will easily inline, and refer to `my_const()` like any other compile-time constant. Then specific projects can redefine it, e.x. `OriginalModule.my_const() = 10`, forcing Julia to recompile any functions that referenced `my_const()`. **However**, `const` global variables are never recalculated, so you can't do `const C = my_const()` and expect `C` to change when `my_const()` changes. Consts should never reference functions that are intended to be redefinable like this.

## Array-like behavior

`Vec{N, T}` implements `AbstractVector{T, 1}`, meaning Julia sees it as a kind of 1D array, and that lets you use all sorts of built-in array-processing functionality with it.

However, in many cases array-processing functions return an actual array (`Vector{T}`) as output, which is not ideal for vector math. So many of these built-in functions are re-implemented to return a `Vec`. For example, you can do a component-wise operation on a `Vec` with `map(func, v)::Vec`.

Unfortunately, one awesome Julia feature that `Vec` doesn't support well is broadcasting.  Broadcasting operators will treat `Vec` as a 1D array, which is good, but the output will be an actual array (`Vector`), which is bad. This can hopefully be improved in the future.

## Ranges

Julia lets you specify number ranges with the colon syntax: `all_one_digit_numbers = 0:9`. You can do the same with `Vec` to create multidimensional ranges.

For example, you can get every index of a 2D array `a` with the range `Vec(1, 1):Vec(size(a, 1), size(a, 2))`, or more succinctly `one(v2i):vsize(a)`. You can mix numbers and vectors, for example `1:v2i(2, 3)` is equivalent to `v2i(1, 1) : v2i(2, 3)`.

You can add a step interval other than 1 using Julia's step syntax `begin:step:end`, again mixing numbers and vectors. For example, you can skip every other column of a 2D array `a` with `one(v2i) : v2i(1, 2) : vsize(a)`.

This feature is mostly intended for integer vectors, but nothing stops you from making ranges with float vectors. However, for floats it's recommended to use [`Box` and `Interval` instead](Math.md#Box-and-Interval).

## Printing

The number of digits that a vector is printed with is controlled by the global variable `Bplus.Mat.VEC_N_DIGITS`. If you want to change this value only temporarily, call `use_vec_digits()`. If you want to print a single vector with a specific number of digits, use `show_vec`.