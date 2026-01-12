Part of the [`Math` module](Math.md). The core game math types are: `Vec{N, T}`, [`Quaternion{F}`](Quat.md), and [`Mat{C, R, F}`](Matrix.md).

# `Vec{N, T}`

An `N`-dimensional vector of `T`-type components. Unlike Julia's built-in `Vector`, this is static, immutable, and most importantly allocatable on the stack.

Under the hood, it holds a tuple `data::NTuple{N, T}`.

Obviously there are some similarities between this type and a static vector (see the *StaticArrays* package),
    but if `Vec` were simply an alias for `SVector` then I would be committing a **lot** of type piracy,
    so the use of a custom struct is warranted.

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
* `Vec(components...)` pass in the individual components, which are all promoted to the same type. For example, `Vec(1.5, 2)` creates a `v2d`.
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
* `Vec{T}()` and `Vec{N, T}()` : constructs a 0-D vector of component type `T`.
  * Another variant `Vec{T}(::Tuple{})` and `Vec{N, T}(::Tuple{})` is also provided.
  * These constructors are useful for recursive functions.


`vappend()` lets you combine vectors and scalars into bigger vectors. For example, `vappend(Vec(1, 2), 3, Vec(4, 5, 6)) == Vec(1, 2, 3, 4, 5, 6)`.

`Vec` is serializable like an array, so it supports the *StructTypes* package and therefore packages like *JSON3*.

## Components/Swizzling

You can get a tuple of a vector's components by accessing its `data` field.

You can access the individual components of a vector in several ways:
* Naming its `x`, `y`, `z`, or `w` component, also named `r`, `g`, `b`, and `a`. For example, `Vec(1, 2, 3).x == 1`.
* Combining components in any order to "swizzle" a new vector. For example, `Vec(55, 66, 77).xxzy == Vec(55, 55, 77, 66)`.
* Getting a single component using its index (1-based, like everything else in Julia). For example, `v[2] == v.y`.
* Accessing a tuple of component indices. For example, `v[1, 1, 3, 2] == v.xxzy`.

Swizzling can also use some special characters to insert certain constant values:
* `0` gets the value 0. For example, `v2f(3, 10).x0y` turns a 2D vector into 3D one by inserting a Y value of 0.
* `1` gets the value 1. For example, `v3f(0.2, 0.8, 1.0).rgb1` appends an alpha of 1 to an RGB color.
* `Δ` (typed as '\Delta' followed by a Tab) gets the largest finite value for the component type. For example, `vRGBu8(20, 200, 63).rgbΔ` Adds an alpha of 255 to an RGB color.
* `∇` (typed as '\del' followed by a Tab) gets the smallest finite value for the component type. For example, `Vec2{Int8}(11, -23).xy∇` appends `-128` to the vector.

You can get a range of components by feeding a range into the vector, like `v[2:4]`, **but** it will be type-unstable as the size isn't known at compile time. If the range is a constant, you can feed it in as a `Val` to get a type-stable result: `v[Val(2:4)]`.

## Math

`Vec` implements many standard Julia functions for numbers:
* `zero()` and `one()`
* `typemin()` and `typemax()`
* `min()`, `max()`, and `minmax()`
* `abs()`, `round()`, `clamp()`, `floor()`, `ceil()`

It implements all the standard math operators (including bitwise ops and comparisons), component-wise with two exceptions: `==` and `!=` are done for the whole `Vec`, producing a single boolean.

* For component-wise equality use `vequal(a, b)`.
You could also do `map(==, a, b)`.

Vector math is implemented with functions prefixed by `v`:
* `vdot(a, b)` is the dot product.
  * You can also use the operator `⋅` (typed as '\cdot' followed by a Tab): `a ⋅ b`
* `vcross(a, b)` is the cross product (only defined for `Vec3`).
  * You can also use the operator `×` (typed as '\times' followed by a Tab): `a × b`
* Calculate distances/lengths with `vdist_sqr(a, b)`, `vdist(a, b)`, `vlength_sqr(a, b)`, and `vlength(a, b)`.
* `vnorm(v)`, `v_is_normalized(v)` for normalization
* `vreflect(v, normal)` and `vrefract(v, normal, IoR)`
* `vbasis(forward, up)` gets three orthogonal vectors, where the forward vector is exactly equal to `forward` and the up vector is as close as possible to `up`.

Vectors also interact nicely with Julia's multidimensional arrays:

* As an `AbstractVector`, `Vec` supports many standard array-processing functions (see [below](#Array-Like-Behavior)).
  * Note that `Vector` is Julia's term for a 1D array; don't get them confused.
* You can get an element of a multidimensional array at an integer-vector index, with `arr[vec]`.
  * Unfortunately, the indexing order of Julia arrays is reversed, so in a 2D array, a `Vec` index's X coordinate is the row, and Y is the column.
  * To get the more intuitive indexing order, for example when working with arrays of pixels, you can `reverse()` the vector or wrap the index with `TrueOrdering` like so: `a[TrueOrdering(v)] == a[reverse(v)] == a[v.z, v.y, v.x]`.
* You can get the size of a multidimensional array as a vector, with `vsize(arr, I=Int32)`.
  * As mentioned above, the indexing order of Julia arrays is reversed, so for example the `vsize` of a 2D array gives you the row count in the X component and the column count in the Y component.
  * Pass `true_order=true` to swap the output's elements and get the more intuitive ordering, useful when working with pixel grids

Some other general utilities:

* `vselect(a, b, t)` lets you pick values component-wise using a `VecB`, sort of like a binary `lerp()`.
  * Note that you can also use `lerp()` for this, although it may be a tiny bit less efficient.
* `vindex(p, size)` to convert a multidimensional coordinate into a flat index, for an array of some multidimensional size.
  `vindex(i::Int, size)` can convert in the opposite direction -- a flat index to a multidimensional one.
* `Base.convert()` can convert the components of a `Vec`, for example `convert(v2f, Vec(3, 4))`. It can also convert a `StaticArrays.SVector` to a `Vec`.
* `Base.reinterpret()` can reinterpret the bits of a `Vec`'s components, for example `reinterpret(v2f, v2u(0xabcdef01, 0x12345678))`.

## Setfield

Because vectors are value-types, they are immutable. The only way to "modify" a vector is to replace it with a copy. Fortunately, Julia has *Setfield*, a built-in package which helps you do this. While it technically does lots of copying, use of *Setfield* will almost always optimize down to something equivalent to mutating your desired field.

You can use Setfield to "modify" a vector variable with the syntax `@set! v.x += 10`, or create a mutated copy with the expression `v2 = @set v1.x += 10`. **However**, it's important to note that if you try to `@set` a vector that belongs to something else, like `@set! my_class.v.x += 10`, *Setfield* will try to make a copy of that entire owning object (`my_class`, not just `my_class.v`).

## Coordinate System

B+ allows you to define the engine-wide up axis and handedness of vector math by redefining `BplusCore.Math.get_up_axis() = 2` (the default is `3` for the Z axis) and `BplusCore.Math.get_right_handed() = false` (the default is `true`) within your own project. For more info on how this works, see [How it works](#How-it-works) below.

An example of when this is useful is if you are primarily developing your project within `Dear ImGUI`'s 2D graphics system rather than 3D OpenGL. Dear ImGUI is left-handed, and arguably Y-up.

There are several built-in functions that respect this configuration:
  * `v_rightward(forward, up)` does a cross-product based on the current handed-ness. In right-handed systems, it does `vcross(forward, up)`. In left-handed systems, it swaps them.
  * `get_up_vector(F = Float32)` gets the 3D up vector. For example, if the up axis is `3`, then it returns `Vec3{F}(0, 0, 1)`.
  * `get_horz_vector(i, F = Float32)` gets the first or second 3D horizontal vector, based on whether you pass in `1` or `2`.
    * `get_horz_axes()` gets a tuple of the horizontal indices. For example, if the up-axis is `3` then it returns `(1, 2)`.
  * `to_3d(v, f=0)` inserts a vertical component to a 2D horizontal vector.
  * `get_vert(v)` grabs the vertical component of a 3D vector.

### How it works

Julia's blurry line between compile-time and run-time allows you to set up compile-time constants directly in the language. You can define a trivial function like `my_const() = 5`, which Julia will easily inline, and refer to `my_const()` like any other compile-time constant. Then specific projects can redefine it, e.x. `using ThePackage; ThePackage.my_const() = 10`. This forces Julia to recompile any functions that referenced `my_const()`. **However**, `const` global variables are never recalculated, so you can't do `const C = my_const()` and expect `C` to change when `my_const()` changes. Consts should never reference functions that are intended to be redefinable like this.

## Array-like behavior

`Vec{N, T}` implements `AbstractVector{T, 1}`, meaning Julia sees it as a kind of 1D array, and that lets you use all sorts of built-in array-processing functionality with it.

However, most built-in array-processing functions return an actual array (`Vector{T}`) as output, which is not ideal for vector math. We re-implement many of them to return a `Vec`. For example, calling `map()` on a `Vec` will return another `Vec`.

Unfortunately, one awesome Julia feature that `Vec` doesn't support well is broadcasting.  Broadcasting operators will treat `Vec` as a 1D array, which is good, but the output will be an actual array (`Vector`), which is bad. This can hopefully be improved in the future.

An interesting note about Julia's multidimensional array literals: normally they would notice that `Vec` is an `AbstractVector` and treat it as an extra dimension. For example, a literal matrix of `v3f` would become a 3D array of Float32, with 3 Z-slices. However, the relevant functions (`hcat`, `vcat`, and `hvncat`) are overloaded to prevent this, so that a matrix literal of `Vec` stays a matrix of `Vec`.

## Ranges

Julia lets you specify number ranges with the colon syntax `begin : end`, for example `all_one_digit_numbers = 0:9`.
You can do the same with `Vec` to create multidimensional ranges.
For example, you can get every index of a 2D array `a` with the range `Vec(1, 1):Vec(size(a, 1), size(a, 2))`, or more succinctly `one(v2i):vsize(a)`. You can generalze it to work with any number of array dimensions by mixing numbers and vectors: `1:vsize(a)`.

You can add a step interval other than 1 using Julia's usual stepped range syntax `begin:step:end`, and again you can mix numbers and vectors. For example, you can skip every other column of a 2D array `a` with `1 : v2i(1, 2) : vsize(a)`.

This feature is mostly intended for integer vectors. While nothing stops you from making ranges with float vectors, it's highly recommended to use [`Box` and `Interval` instead](Math.md#Box-and-Interval).

## Printing

The number of digits that a vector is printed with is controlled by the global variable `BplusCore.Mat.VEC_N_DIGITS`. If you want to change this value only temporarily, call `use_vec_digits()`. If you want to print a single vector with a specific number of digits, use `show_vec`.