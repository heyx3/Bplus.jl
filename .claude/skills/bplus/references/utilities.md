# BplusCore.Utilities

The grab-bag module. No OpenGL dependency; usable in any Julia project via `BplusCore`.

## Asserts and debug flags

`@bp_check(condition, msg...)` — runtime check, `error()`s if false, stringifies and concatenates every `msg`. Always on.
`@bp_check_throws(statement, msg...)` — asserts that an expression throws. For tests.

`@make_toggleable_asserts(prefix_)` sets up a per-module debug/release flag. Invoking `@make_toggleable_asserts(my_game_)` generates:

1. `@my_game_assert(condition, msg...)` — the debug-only `@bp_check`.
2. `@my_game_debug()` — a compile-time boolean constant.
3. `@my_game_debug(debug_code[, release_code])` — runs code only in the corresponding mode.
4. `my_game_asserts_enabled() = false` — the driver.

Enable debug mode by redefining the driver, inside or outside the module:

```julia
using MyGameModule
MyGameModule.my_game_asserts_enabled() = true
MyGameModule.run_game()
```

The function body is trivial enough that Julia always inlines it, so `false` constant-folds the debug code out of the program entirely. Redefining it forces recompilation of every function that referenced it, and the debug code reappears.

**`const`s are never recomputed.** `const C = @my_game_debug(100, 1000)` keeps whatever value it got at load time forever. Never let a `const` depend on a redefinable flag function.

Each B+ module has its own flag:

| Module | Function |
| --- | --- |
| `BplusCore.Math` | `bp_math_asserts_enabled()` |
| `BplusApp.GL` | `bp_gl_asserts_enabled()` |
| `BplusApp.GUI` | `bp_gui_asserts_enabled()` |
| `BplusApp.Input` | `bp_input_asserts_enabled()` |
| `BplusTools` (helpers) | `bp_helpers_asserts_enabled()` |
| `BplusTools.ECS` | `bp_ecs_asserts_enabled()` |
| `BplusTools.Fields` | `bp_fields_asserts_enabled()` |
| `BplusTools.SceneTree` | `bp_scene_tree_asserts_enabled()` |

In your own project you probably want one flag for the whole thing rather than per-module.

This is a custom implementation of the *ToggleableAsserts.jl* package, which offers some of the same thing globally.

## Optional

- `Optional{T}` = `Union{T, Nothing}`
- `exists(t)` = `!isnothing(t)`
- `get_something(a, b)` — `a` unless it's `nothing`, else `b`
- `@optional(condition, outputs...)` — splices `outputs...` into a call when true, `()...` when false
- `@optionalkw(condition, name, value)` — same for named parameters; `NamedTuple()` when false

`@optional` is likely very type-unstable when the condition isn't compile-time known. Check for allocations before using it in a hot path.

## Collections and iterators

- `preallocated_vector(T, capacity::Int)`
- `Base.append!` for sets — type piracy, but the hole in Julia's API is odd enough to be worth it
- `UpTo{N, T}` — type-stable, immutable, stack-allocated container for 0…N elements. Implements `AbstractVector{T}`. This is what ray-shape intersection returns. `append(a::UpTo{N,T}, b::UpTo{M,T})::UpTo{N+M,T}`.
- `SerializedUnion{U<:Union}` — a union value that serializes *and deserializes* with its type, via *StructTypes* (so *JSON3* works). Build the type with `@SerializedUnion(A, B, ...)`. Parse priority is controlled by `union_ordering(T)::Float64`; lower is tried first.
- `IterSome(lambda, T=Any)` — iterator driven by `idx::Int -> Union{Nothing, Some{T}}`, ending when you return `nothing`. Pass `T` to help inference.
- `RandIterator(n, rng)` — efficiently walks `1:n` in random order.

### Tuples

- `tuple_length(::Type{<:Tuple})::Int` — for tuple *types*; use `length(t)` on instances
- `ConstVector{T, N}` = `NTuple{N, T}`, letting you name the element type without the count (`ConstVector{Int64}` matches any length)

### Functional

`unzip(zipped)` (reiterates the input once per output unless it came from a real `zip()`), `reduce_some(f, predicate, iter; init=0)`, `find_matching(element, collection, comparison=Base.:(==))` (returns the index/key or `nothing`), `iter(x)` (facade making things like `Dict` work with `map()`), `map_unordered(f, iters...)`, `drop_last(iter)`, `iter_join(iter, delimiter)`, `iter_join_flatten(iter, delimiter)`.

## Enums

### `@bp_enum(name, elements...)`

Same declaration syntax as Julia's `@enum`, but much more capable. For `@bp_enum MyEnum a b c`:

- Values are scoped — `MyEnum` becomes a sub-module, so it's `MyEnum.a`, not a bare `a`.
- The enum type itself is aliased outside the module as `E_MyEnum`.
- `MyEnum.instances()` — tuple of all values
- `MyEnum.from(s)` / `parse(E_MyEnum, s)` from a `String` or `Symbol`; `tryparse` returns `nothing` instead of throwing; `MyEnum.enable_parsing_from(s -> my_conversion(s))` at runtime (e.g. in `__init__`) adds support for other string forms
- `MyEnum.from(i)` from the integer value; `MyEnum.from_index(idx)` from declaration order; `MyEnum.to_index(e)` back
- `rand(E_MyEnum)` is generated for you
- C interop converts between a pointer to the enum and a pointer to its underlying integer type

### `@bp_bitflag`

Everything above, plus:

- Default values go up by powers of two (1, 2, 4, 8, …). To get a "none" element followed by that numbering, put it first: `@bp_bitflag MyBitflag z=0 a b c`. Full numbering rules come from *BitFlags.jl*.
- `a | b` combine, `a & b` intersect, `a - b` remove (= `a & ~b`)
- `a <= b` is "a is a subset of b"; `a >= b` is superset; `Base.contains(haystack, needle)` for full containment
- `MyBitflag.ALL` — everything OR-ed together
- Aggregates declared with `@`, which don't appear in `instances()`/`to_index()`/`from_index()`:

```julia
@bp_bitflag(MyFlags,
    a, b, c, d, e,
    @ab(a|b),
    @abc(ab|c),
    @ace((abc - b) | e)
)
```

Prefer this over *BitFlags*' `@bitflag`, which inherits `@enum`'s scoping problems.

### Anonymous enums

For small function-parameter enums, no declaration needed:

```julia
function do_stuff(i::Int, mode::@ano_enum(Mode1, Mode2, Mode3))
    if mode isa @ano_enum(Mode1, Mode2)
        i = -i
    end
    i *= 2
    if mode isa @ano_enum(Mode1, Mode3)
        i *= 10
    end
    return i
end

do_stuff(i, @ano_value(Mode2))
```

Used inside B+ itself — e.g. the `Box` constructor's integer-vs-float division mode.

## Numbers

`@f32(n)` casts to Float32. `Scalar8`/`Scalar16`/`Scalar32`/`Scalar64`/`Scalar128` are unions of the built-in number types of that bit size (`Scalar32 == Union{UInt32, Int32, Float32}`; `Scalar8` excludes `Bool`). `ScalarBits` unions all of them. `type_str(::Type{<:Scalar})` gives a short name (`type_str(UInt8) == "u8"`). `Binary(x)` decorates a number to print in binary; implement the interface in its doc-string to support your own type.

## Random — `PRNG` and `ConstPRNG`

`PRNG` implements Julia's `AbstractRNG`, so it drops into anything that takes an RNG. It exists because the built-in RNGs are tuned for statistical quality over spin-up cost, and procedural generation wants the opposite. Construct with zero or more seed numbers, or with an explicit state (a `UInt32` plus an `NTuple{3, UInt32}`).

Like all Julia RNGs, `PRNG` is mutable — meaning pass-by-reference and probably heap-allocated. Fine for a long-lived generator.

`ConstPRNG` is the same algorithm as an **immutable** struct: pass-by-value, usually not heap-allocated. Use it when you're creating and discarding thousands or millions of RNGs — one per pixel of a generated texture, say, seeded by the pixel coordinate. The trade-off is that every call returns a tuple of `(value, next_rng)`:

```julia
function generate_pixel(x, y)
    rng = ConstPRNG(x, y)
    (red,   rng) = rand(rng, Float32)
    (green, rng) = rand(rng, Float32)
    (blue,  rng) = rand(rng, Float32)
    return (red, green, blue)
end
```

`rand_ntuple(rng, t::NTuple)` picks a random element of a tuple.

## Flood fill

```julia
flood_fill(seeds, in_connections, out_start_area, out_new_node, out_end_area, [allocations])
```

Fully generic — you own both the space and the output.

- `seeds` — any enumeration of nodes a fill may start from
- `in_connections` — `(node, outputs) -> nothing`, appends every outward connection
- `out_start_area` — `() -> nothing`, a new area has begun
- `out_new_node` — `node -> nothing`, a node joined the current area
- `out_end_area` — optional `() -> nothing`, the area is complete
- `allocations` — optional `FloodFillAllocations{TNode}`, built with `FloodFillAllocations(TNode)`

Hold one `FloodFillAllocations` across many fills to avoid re-allocating.

## Macros

`@do_while code condition` — a do-while loop that reads much like the C form:

```julia
i::Int = 0
@do_while begin
    println(i)
    i += 1
end i<10
```

`@decentralized_module_init` — lets a module register run-time init code from several places instead of one `__init__()`. After invoking it, statically `push!` lambdas onto the global `RUN_ON_INIT` list; they run once at the start of each program run (precisely: the first time the module is loaded), in push order. Implemented by declaring `RUN_ON_INIT` and defining `__init__()` to call each entry.

Use it for things that must be computed per *run* rather than per *compile* — pointers to C functions or allocated memory, for instance, which differ every launch.

The registration happens at compile time. You cannot add lambdas from outside the module or from a function executing at run time — the module has already finished initializing, and your push is silently ignored. Sub-modules should define their own `__init__()` or invoke the macro themselves.

## Byte reinterpretation

`reinterpret_bytes(input, output)` converts between bitstype data.

Inputs, for a bitstype `T`: a `T` instance; `AbstractArray{T}` (use `@view` for a subset); `Ref{T}`; `Tuple{Ref{T}, Integer}`; `Tuple{Ptr{T}, Integer}`.
Outputs: `AbstractArray{T}`; `Ref{T}`; `Ptr{T}`; or the type `T` itself, in which case the reinterpreted value is returned rather than written.

`reinterpret_bytes_slow(src, dest_bytes = [new allocation])` bitwise-copies each element of `src` into the destination — needed when the source elements aren't contiguous or aren't all one bitstype. Allocates and returns a `Vector{UInt8}` if you pass no destination.

`val_type(::Val{T})` / `val_type(::Type{Val{T}})` → `T`. (`Val{T}` lifts bits data into a type parameter; it's central to zero-cost abstractions in Julia.)

## Interop

`InteropString` keeps a string in two forms at once — a Julia `String` and a C-style null-terminated `Vector{UInt8}`.

- `i_str = InteropString(s::String[, buffer_capacity_multiplier])`
- `string(i_str)` — current Julia value
- `update!(i_str, new_value::String)` — set from Julia
- `update!(i_str)` — regenerate the Julia string after the C side modified the buffer

`update!` collides with *DataStructures*' `update!`. If you use that package, qualify the call or alias it: `const update! = BplusCore.Utilities.update!`.

## Metaprogramming

Much of this assumes familiarity with Julia macros, and builds on *MacroTools*.

### Unions

`@unionspec(T{_}, A, B, C)` → `Union{T{A}, T{B}, T{C}}` (equivalent to the more verbose `Union{(T{t} for t in (A,B,C))...}`).
`union_types(U)` → a tuple of the individual types; a single type gives a 1-tuple.

### Expression inspection

- `visit_exprs(to_do, root)` — like `MacroTools.prewalk` but non-modifying, with extra location info per sub-expression
- `FunctionMetadata(expr)` — strips top-level info off a function definition (doc-string, `@inline`, `@generated`). Returns `nothing` on invalid syntax rather than throwing, which makes it a convenient validity check — though it doesn't inspect the core expression, only what surrounds it. `MacroTools.combinedef(fm)` rebuilds the `Expr`.
- `is_scopable_name(expr)` — a Symbol, optionally dot-prefixed (`:(Base.iszero)`), optionally with trailing type params (`:(A.B{C})`)
- `is_function_call(expr)` — `:( MyGame.run(i::Int; j=5) )`
- `is_function_decl(expr)` — `:( f() = 5 )`
- `expr_deepcopy()` — `deepcopy` that skips things in an AST which must not be copied, such as literal module references

### Expression splitting

Deeper and broader analogues of `MacroTools.splitdef`/`splitarg`. All inherit `AbstractSplitExpr`, are constructed from an expression (or another instance, to deep-copy), rebuild via `combine_expr(s)`, and mostly detect an `esc()` wrapper. Field meanings are documented in the definitions.

- `SplitArg(expr)` — an argument declaration, `:( i::Int=3 )`
- `SplitType(expr)` — a type declaration, `:( A{X} <: B )`. Deep-validates elements by default; pass `false` as the second argument to skip.
- `SplitDef(expr)` — a function declaration, e.g. `quote "Returns 0." @inline f() = 0 end`. Set the body to `nothing` (the value, not `:nothing`) to make it a signature; set the name to `nothing` to make it a lambda; setting both is an error.
- `SplitMacro` — a macro invocation, `:( @m a b+c )`

### Assignment operators

`ASSIGNMENT_INNER_OP[:+=] == :+`; `ASSIGNMENT_WITH_OP[:+] == :+=`; `compute_op(:+=, a, b)` computes `a + b`.
