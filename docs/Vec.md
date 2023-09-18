Part of the [`Bplus.Math` module](Math.md). The core game math types are: `Vec{N, T}`, [`Quaternion{F}`](Quat.md), and [`Mat{C, R, F}`](Matrix.md).

# `Vec{N, T}`

An `N`-dimensional vector of `T`-type components. Unlike Julia's built-in `Vector`, this is static, immutable, and most importantly allocatable on the stack.

Under the hood, it holds a tuple `data::NTuple{N, T}`.

### Aliases

#### By dimension

* `Vec2{T}`
* `Vec3{T}`
* `Vec4{T}`

#### By component type

* `VecF{N}` : `Float32`
* `VecD{N}` : `Float64`
* `VecI{N}` : `Int32`
* `VecU{N}` : `UInt32`
* `VecB{N}` : `Bool`
* `VecT{T, N}` allows you to specify a vector's component type before its dimension. For example, `VecT{Float32} == VecF`.

#### By dimension *and* component type

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

#### By intent

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

### Construction

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

# TODO: Implement the below.

### Swizzling

### Math

### Setfield

### Array-like behavior

### Printing