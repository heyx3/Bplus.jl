Part of the [`Bplus.Math` module](Math.md). The core game math types are: `Vec{N, T}`, [`Quaternion{F}`](Quat.md), and [`Mat{C, R, F}`](Matrix.md).

# `Quaternion{F}`

A set of 4 numbers that can efficiently and effectively represent 3D rotations.

Like `Vec`, this type is immutable and able to be allocated on the stack. Fortunately, immutability isn't a big deal for quaternions are you aren't going to be fiddling with their individual components much.

## Aliases

* `fquat` is a Float32 quaternion.
* `dquat` is a Float64 quaternion.

## Construction

* You can provide the 4 components of the Quaternion directly as individual values, or a tuple, or a `Vec4`.
* You can provide nothing, to create an identity quaternion. For example, `fquat()`.
* You can provide an axis and angle (in radians). For example, `Quaternion(get_up_vector(), deg2rad(180))`
* You can provide a start and end vector, both normalized, to get the rotation from one to the other. For example, `Quaternion(get_up_vector(), vnorm(v3f(1, 1, 1)))`.
* You can provide a rotation matrix (see [`Mat{C, R, F}`](Matrix.md)) to convert it to a quaternion.
* You can provide a `VBasis` (the output of `vbasis(forward, up)`) to create a Quaternion representing the rotation that creates this basis.
  * This constructor needs to know what an "un-rotated" basis is. For example, what direction is Up? By default, this comes from [the coordinate system configured for B+](Vec.md#Coordinate-System).
* You can provide a start and end `VBasis` to create a rotation transforming the former to the latter.
* You can provide a sequence of quaternions in chronological order, to make one quaternion representing all those rotations done in sequence.

## Operations

* `q_apply(q, v)` to rotate a `Vec`.
* `a >> b` and `a << b` to combine two quaternion rotations in the order specified by the arrow (respectively, "a then b" and "b then a").
* For a quick and cheap interpolation between quaternion rotations, `lerp(a, b, t)` works just fine.
* For higher-quality interpolation, such as an animation, `q_slerp(a, b, t)` is better.
* `q_basis()` gets the vector basis (as a `VBasis` instance) created by the given rotation. It assumes specific axes for "forward", "up", and "right", which you can configure with named parameters.
* `q_is_identity(q)` tests whether a Quaternion represents "no rotation".
* `q_slerp(a, b, t)` to interpolate between `a` and `b` using a nicer-quality interpolation that requires more math to compute.
  * The standard `lerp()` function can also be used for Quaternions, but for animation it does not produce as nice an effect.

## Conversions

* `q_axisangle(q)::Tuple{Vec3, F}` returns the axis and radians of the rotation represented by this quaternion.
* `q_mat3x3(q)` and `q_mat4x4(q)` get the rotation matrix for this quaternion.

## Arithmetic

It's not recommended that you use these directly unless you know what you're doing.
* Multiplication (`a*b`) and negation (`-a`) are defined as you'd expect.
* `qnorm(q)` and `q_is_normalized(q)` help with normalization.

## Printing

Quaternions are printed as an axis and angle. The number of digits in each are controlled by `QUAT_AXIS_DIGITS` and `QUAT_ANGLE_DIGITS`, respectively.

To temporarily change the number of digits used, invoke `use_quat_digits()`.

To print a single quaternion with a specific number of digits, use `show_quat()`.