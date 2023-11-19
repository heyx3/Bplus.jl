The module `Bplus.Utilities`.

## Asserts

You can do runtime checks with `@bp_check(condition, msg...)`. It throws an error if the condition is false, and provides the given message data (by converting each to `string()` and appending them together).

To add asserts and a debug/release flag to your project, invoke `@make_toggleable_asserts(prefix)`. Invoking it adds a new compile-time flag, plus some helper macros. In B+ each sub-module has its own toggleable asserts; in your own program you probably just want one for the whole project.

For example, if you invoke `@make_toggleable_asserts(my_game_)`, then you'll get the following new definitions:
  1. `@my_game_assert(condition, msg...)` is the debug-only version of `@bp_check`.
  2. `@my_game_debug()` is a compile-time boolean constant for whether you're in debug mode.
  3. `@my_game_debug(debug_code[, release_code])` runs some debug-only code, and alternatively some optional release-only code.
  4. `my_game_asserts_enabled() = false` is the core function driving this feature. You can enable "debug mode" by explicitly redefining it, either within the module or after the module. For example, `MyGameModule.my_game_asserts_enabled() = true; MyGameModule.run_game()`.
     * You can see an example of toggling debug flags in B+'s unit tests (*test/runtests.jl*).
     * The way this works is that the implementation of `my_game_asserts_enabled()` is so simple that Julia will always inline it, causing the value (`true` or `false`) to propagate out and effectively remove debug statements from the codebase entirely when in release mode.
     * Redefining the function forces Julia to recompile all functions which referenced it, causing debug code to suddenly appear.

**Important note**: constants do not get recompiled! For example, if you declare `const C = @my_game_debug(100, 1000)`, then toggle the debug flag, `C` will not change its value from 1000 to 100.

*Note*: `@make_toggleable_asserts` is a custom implementation of the *ToggleableAsserts.jl* package, which provides some of this functionality globally.

## Optional

* `Optional{T}` is an alias for `Union{T, Nothing}`.
* `exists(t)` is an alias for `!isnothing(t)`.
* `@optional(condition, outputs...)` helps you optionally pass parameters into a function call. It evaluates to `()...` if the condition is false, or `outputs...` if it is true.
  * I'm guessing that the use of this macro is very type-unstable if the condition isn't known at compile-time, so don't use it in high-performance contexts unless you can check that it doesn't cause allocations/dynamic dispatch.
* `@optionalkw(condition, name, value)` is like `@optional` but for named parameters. It evalues to `NamedTuple()` if the condition is false, or `$name=$value` if the condition is true.

## Collections and Iterators

* `preallocated_vector(T, capacity::Int)` creates a vector with a specific initial capacity.
* `Base.append!` is now implemented for sets. This is type piracy, but it's also a very strange hole in Julia's API to not be able to append an iterator to a set.
* `UpTo{N, T}` is a type-stable, immutable container for 0 to N elements of type `T`. For example, calculating the intersection of a ray and sphere could return `UpTo{2, Float32}`.
  * The type implements `AbstractVector{T}`, so it can be treated like a 1D array using all of Julia's built-in functions for array manipulation.
  * `append(a::UpTo{N, T}, b::UpTo{M, T})` creates a new `UpTo{N+M, T}`.
* `SerializedUnion{U<:Union}` is a value that can be serialized with its type (using the ubiquitous *StructTypes* package for serialization, which also means it works with the *JSON3* package). The macro `@SerializedUnion(A, B, ...)` helps you specify the type more easily.
  * For example, if you want to write and read a `Union{Int, String}` using *JSON3*, you can do it by reading and writing with the type `@SerializedUnion(Int, String)`.
  * The order of priority for trying to parse the different types in a serialized union is controlled by the function `union_ordering(T)::Float64`. Lower values are tried first.

### Tuples

* `tuple_length(::Type{<:Tuple})::Int` gets the number of elements in a tuple type.
  * For a tuple object you can use `length(t)`; this is for tuple *types*.
* `ConstVector{T, N}` is an alias for `NTuple{N, T}` which allows you to specify the element type without the count. For example, `ConstVector{Int64}` matches a tuple of all `Int64`, of any length.

### Functional programming

* `unzip(zipped)` takes the output of a `zip()` call (or something that looks like it) and splits them back out into the original iterators.
  * If the input isn't from an actual call to `zip()`, then the unzipping will require iterating through the input several times (once for each output iterator).
* `reduce_some(f, predicate, iter; init=0)` works like the built-in `reduce()`, but also skips over elements that fail a certain predicate.
* `find_matching(element, collection, comparison=Base.:(==))` gets the index/key of the first element matching a desired one. Returns `nothing` if none matches. Optionally uses a comparison other than `==`.
* `iter(x)` provides a facade for iterators that don't work on functions like `map()`. For example, while you can't do  `map(f, my_dictionary)`, you *can* do `map(f, iter(my_dictionary))`.
* `map_unordered(f, iters...)` uses `iter()` to run `map()` on unordered collections that don't normally support `map()`.
* `drop_last(iter)` removes the last element of an iteration.
* `iter_join(iter, delimiter)` inserts `delimiter` in-between elements `iter`, like `join()`.

## Enums

### Scoped enums

`@bp_enum(name, elements...)` is a more powerful version of Julia's `@enum`, with the same declaration syntax. It has the following improvements (assuming your enum is named `MyEnum`):
  * The enum values are kept in their own scope, by turning `MyEnum` into a sub-module.
  * The actual enum type inside the module is aliased to `E_MyEnum` outside the module, for convenience.
  * The values can be parsed from a string (or `Val{S}`, where `S` is a `Symbol`) with `MyEnum.parse(s)`.
  * The values can be converted from their integer value with `MyEnum.from(i)`, or from their index in the declaration order with `MyEnum.from_index(idx)`.
  * The enum values can be converted to their integer index with `MyEnum.to_index(e)`.
  * A tuple of all enum values can be gotten with `MyEnum.instances()`.

### Scoped bitflags

`@bp_bitflag` is for enums that reprsent bit-flags. Along with the above features of `@bp_enum`, `@bp_bitflag` adds the following (assuming your enum is named `MyBitflag`):
  * Default values of elements increase by powers of two -- the first element is `1`, second is `2`, third is `4`, fourth is `8`, etc.
    * To define a "none" element followed by default numbering, put it at the beginning. For example, `@bp_bitflag MyBitflag z=0 a b c`.
    * For the full numbering rules, refer to the *BitFlags.jl* package which powers this macro.
  * Combine two bitflags with `a | b`.
  * Intersect two bitflags with `a & b`.
  * Remove bitflags with `a - b` (equivalent to `a & (~b)`).
  * Check for subsets with `a <= b` ("a is a subset of b?") and supersets with `a >= b` ("a is a superset of b?").
  * `Base.contains` can be used to check if one bitflag is totally contained within another: `Base.contains(haystack::E_MyBitflag, needle::E_MyBitflag))`.
  * The special value `MyBitflag.ALL` contains all the bitfield elements combined together. Equivalent to `reduce(|, MyBitflag.instances())`.
  * Special aggregate values are defined with the `@` symbol. These do not show up in `instances()`, `to_index()`, or `from_index()`. For example:
````
@bp_bitflag(MyFlags,
    a, b, c, d, e,
    # Aggregates:
    @ab(a|b), # Declares 'ab'
    @abc(ab|c) # Declares 'abc'
    @ace((abc - b) | e) # Declares 'ace'
)
````

*Note*:  `@bp_bitflag` is meant to be a replacement for `@bitflag`, from the *BitFlags* package. `@bitflag` inherits a lot of the same problems as `@enum`, and also misses some big convenience features.

## Numbers

* `@f32(n)` is short-hand to cast a value to `Float32`.
* `Scalar8`, `Scalar16`, `Scalar32`, `Scalar64`, and `Scalar128` are aliases for built-in number types of the given bit sizes.
  * Note that `Scalar8` does not include `Bool`.
  * For example, `Scalar32` is `Union{UInt32, Int32, Float32}`.
* `ScalarBits` is a union of all the scalar types mentioned above.
* `type_str(::Type{<:Scalar})` gets a short-hand for a number or boolean type.
  * For example, `type_str(UInt8)` returns `"u8"`.
* `Binary(x)` is a decorator for numbers that prints them as their binary representation.
  * You can support this decorator for your own type by implementing the interface described in `Binary`'s doc-string.

## Random

* `rand_ntuple(rng, t::NTuple)` picks a random element from an `NTuple`.
* `RandIterator(n, rng)` efficiently iterates through `1:N` in a random order.

`PRNG` is a custom RNG struct which implements Julia's `AbstractRNG` interface. This means you can use it like any of the built-in RNG types. However, this one is designed for speed and quick spin-up time, which is important for procedural generation purposes.

You can construct it by passing 0 or more numbers to use as seeds. You can also construct it with an initial state (a `UInt32` followed by a `NTuple{3, UInt32}`).

### Mutable version

Like all RNG's offered by Julia, `PRNG` is mutable, which also means it's pass-by-reference and probably heap-allocated. This is OK for RNG's which are created once and last for a while.

### Immutable version

In some contexts, you want to quickly create and destroy thousands or millions of RNG's. For example, generating a random texture usually involves creating one RNG per pixel, using the pixel coordinates as the seed, and only generating a handful of numbers with each one.

For this use-case, use `ConstPRNG`. It is the same RNG as an immutable struct, which in Julia also means it's pass-by-value and not usually heap-allocated.

`ConstPRNG` implements many of the same functions as `PRNG`. However, its return type is different. It always returns a tuple of the usual random output and the next state of the RNG. Here is an example of how you can use it:
````
function generate_pixel(x, y)
    rng = ConstPRNG(x, y)
    (red, rng) = rand(rng, Float32)
    (green, rng) = rand(rng, Float32)
    (blue, rng) = rand(rng, Float32)
    return (red, green, blue)
````

## Macros

* `@do_while code condition` implements a do-while loop. The syntax can pretty closely match do-while loops in C-like languages, for example:
````
i::Int = 0
@do_while begin
    println(i)
    i += 1
end i<10
````
* `@decentralized_module_init` allows you to add runtime initialization code in multiple different places within a single module. After invoking it, you make use of it by statically adding lambdas to the global list `RUN_ON_INIT`. All these lambdas will execute once at the start of each program run (or more precisely, the first time this module is loaded with `using` or `import`). You can find examples of this in the B+ codebase.
  * This is valuable if you have a lot of disparate data to be computed at the beginning of each program run, rather than the more common `const` globals which are computed at compile-time. For example, a pointer to some C function or allocated memory will be different every time you run the program.
  * You cannot add lambdas outside the module, or within a function running at run-time. If you try, that function will be ignored since the module has already finished initializing by the point your code is reached.
    * Note the distinction between compile-time and run-time here: you are registering the lambdas with  `RUN_ON_INIT` at compile-time, so that they can be later executed at run-time. The blurry line between write-time, compile-time, and run-time is key to Julia's expressive power, but it's tricky to get your head around at first.
    * You shouldn't add lambdas within a submodule either, although I belive it works. You should just define `__init__()` or `@decentralized_module_init` for that sub-module.
  * This macro is implemented by **a)** declaring the list `RUN_ON_INIT`, and **b)** implementing the special Julia function `__init__()` to call every lambda in the list.
  * The order of lambda execution is always the order they were added to the list, a.k.a. the order of your `push!` calls.

## Functions

* `reinterpret_bytes(x)::NTuple{sizeof(x), UInt8}` gets the bytes of some bitstype data.
* `reinterpret_bytes(bytes::NTuple{sizeof(T), UInt8}, byte_offset, T)::T` creates a bitstype using the given bytes, or a subset of them using the given byte offset.
* `val_type(::Val{T})` and `val_type(::Type{Val{T}})` return `T`. For example, `val_type(Val(:hi))` returns `:hi`.
  * For reference, `Val{T}` is a built-in Julia type that turns some bits data into a type parameter. It's an important part of building complex zero-cost abstractions.

## Interop

* `InteropString` juggles a string across two representations: the Julia `String` and a C-style, null-terminated buffer (of type `Vector{UInt8}`).
  * Create a new string with `i_str = InteropString(s::String[, capacity_size_multiple])`.
  * Get the Julia string's current value with `string(i_str)` (the core `string` function defined in `Base` and available everywhere).
  * Set the Julia string with `update!(i_str, new_value::String)`.
    * Note that `update!()` is also declared in the package *DataStructures*, so if you use that module you have to put the module name in front of each `update!` call, or pick the default one with an alias `const update! = Bplus.Utilities.update!`.
  * If the C version of the string was modified, regenerate the Julia string with `update!(i_str)`.

## Metaprogramming

### Unions

* `@unionspec(T{_}, A, B, ...)` turns into `Union{T{A}, T{B}, ...}`
* `union_types(U)` gets a tuple of the individual types inside a union.
  * If you pass a single type, then you get a 1-tuple of that type.

### Expression manipulation

Much of this section requires some familiarity with Julia macros and metaprogramming, a.k.a. code that generates code. It also builds off of the *MacroTools* package.

* `FunctionMetadata(expr)` strips out top-level information from a function definition. In particular it checks for doc-strings, `@inline`, and `@generated`. For example, `FunctionMetadata(quote "Multiplies x by 5" @inline f(x) = x*5 end)` creates an instance of `FunctionMetadata("Multiplies x by 5", true, false, :( f(x) = x*5 ))`.
  * If you pass some invalid syntax into `FunctionMetadata(expr)`, it outputs `nothing` rather than throwing an error. This lets you more easily check whether an expression is a valid function definition or not. However it doesn't check the core expression (e.x. `f(x) = x*5`); only the things surrounding it.
  * You can turn an instance back into the original expression with `MacroTools.combinedef(fm::FunctionMetadata)::Expr`.
* `SplitDef(expr)` implements a type-safe alternative to `MacroTools.splitarg()`, breaking an argument in a function signature into all its individual components. Refer to the fields of the `SplitArg` struct for more info.
  * You can use `MacroTools.combinearg(a::SplitArg)` to get the AST for the parameter declaration.
  * You can create a deep copyp of a `SplitArg` with the constructor: `SplitArg(src)`. Do not use `deepcopy()`, as that fails for some AST's.
* `SplitDef(expr)` implements a type-safe alternative to `MacroTools.splitdef()`, breaking a function definition into all its individual components. Refer to the fields of the `SplitDef` struct for more info. Along with type-safety, this struct supports function metadata like doc-strings, `@inline`, and `@generated`.
  * You can use `MacroTools.combinedef(s::SplitDef)` to get the AST for the function.
  * You can use the new function `combinecall()` to get the function as just a call rather than a definition. For example, `combinecall(SplitDef(:( f(a, b=5; c) = a*b+c ))` returns `:( f(a, b; c=c) )`.
  * You can create a deep copy of a `SplitDef` with the constructor: `SplitDef(src)`. Do not use `deepcopy()`, as that fails for some AST's.
* `SplitMacro(expr)` works like `SplitDef` but for macro calls rather than function deinitions, breaking the macro call into its name, line number node, and arguments.
  * You can use `combinemacro(m::SplitMacro)` to put it back to gether into an expression.
* Various helper functions for checking the syntax of an expression:
  * `is_scopable_name(expr)::Bool` checks for a name symbol, possibly prefixed by one or more dot operators (e.x. `:( Base.iszero )`)
  * `is_function_call(expr)::Bool` checks for a function call/signature, like `MyGame.run(i::Int; j=5)`.
  * `is_function_decl(expr)::Bool` checks for a function definition, like `f() = 5`.
* `expr_deepcopy()` works like `deepcopy()`, but without copying certain things that may show up in an AST and which aren't supposed to be copied. For example, literal references to modules.
* `ASSIGNMENT_INNER_OP` maps modifying assignment operators to the plain operator. For example, `ASSIGNMENT_INNER_OP[:+=]` maps to `:+`.
  * `ASSIGNMENT_WITH_OP` is a map in the opposite direction: `ASSIGNMENT_WITH_OP[:+]` maps to `:+=`.
* `compute_op(s::Symbol, a, b)` performs one of the modifying assignments (e.x. `+=`) on `a` and `b`. For example, `compute_op(:+=, a, b)` computes `a + b`.