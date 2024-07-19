Julia is a niche scientific computing language. What about it is interesting to game dev?

TL;DR: scientists and game developers are dealing with the same problem.

## The two-language problem

Scientists are not expert software engineers, yet they need high performance, so they end up splitting their time and attention between one language for performant libraries (usually C/C++) and another for high-level simplicity and usability (often Python). This isn't ideal.

If you're familiar at all with gamedev, you already know that we have the same issue! Balancing the needs of physics and rendering engineers with the needs of game designers and artists is difficult. Unity tries to go middle-of-the-road with C# (plus a C++ backend which you can't modify), while Unreal has the dichotomy of Blueprints and C++.

Julia manages to solve this problem very well IMHO, and it is just as useful for game developers as it is for PhD's.

## Syntax

Code is extremely terse, simple to write, and simple to read, yet the resulting programs can be heavily-optimized (the benchmarks I read have found that it's approaching the speed of C, and it's still a young language). Thanks to an extremely clever JIT engine, combined with aggressive Whole-Code-Optimization using inlining, constant-folding, and type inference, you can write complex abstractions which easily become zero-cost.

Compare this to C++, which achieves zero-cost abstractions at the price of legendary verbosity and inscrutable error messages. Julia has been a wonderful breath of fresh air for me.

Other high-performance scientific computing languages exist, like Matlab, but their syntax is much weirder. Julia's is quirky, yet readable.

It's also incredibly flexible. You can write extremely mathy code, like defining the cross product with `v₁ × v₂ = [blah blah])`, or you can write that same code with plain imperative syntax:

````
function ×(forward, up)
    x = [blah]
    y = [blah blah]
    z = [blah blah blah]
    return Vec(x, y, z)
end
````

## High-performance "Dynamic" Typing

Julia acts like a dynamically-typed language, which is great for high-level users. And unlike most high-performance languages, it *is* truly possible to have variables whose types are completely unknown at compile-time.

Additionally, the language offers (and **heavily** leans on) "multiple-dispatch", which is like runtime function overloading -- if you have a variable that is either `Int` or `Float32`, and you call a function that has different overloads for `Int` vs `Float32` parameters, the correct overload is figured out at runtime. Which, again, is super nice for high-level users, but if your data is dynamically-typed then any function calls with that data have a heavy performance cost.

So, why isn't this a fatal problem for high-peformance code?

In short, the dynamic typing does not propagate into other function calls, making dynamically-typed data the exception rather than the rule!

Nearly every Julia function gets re-compiled for each combination of specific parameter types. When calling a function with arguments of unknown type, you do pay the cost of dynamic dispatch, but the function you dispatch to knows the types of these parameters at compile-time. In other words, parameters are *always* strongly-typed unless you explicitly mark them otherwise.

Therefore, the only time you run into dynamically-typed data is when you create it within your own function, or if you call a function that returns it. Both of these cases can be caught with code introspection tools built into Julia, and easily mitigated. In fact, from what I've heard there is some new VSCode integration that highlights dynamically-typed variables for you, making it trivial.

## Metaprogramming Annihilates Boilerplate

On the rare occasion you have to write something that might be called "boilerplate", you can automate the task with metaprogramming features. Julia has syntactical macros (think Scheme, not C), with a built-in concept of code represented as data. Yet another breath of fresh air for C++ developers, whose choice for metaprogramming is between text-pasting macros and inscrutable, verbose templates.

For example, in order to upload data structures to the GPU, you must follow one of two onerous padding and alignment rules: `std140` or `std430`. In B+, this is completely automated by simply wrapping your struct in `@std140` or `@std430`! The bit pattern of your struct will then precisely match the bit pattern expected by the GPU. You can even query how many padding bytes were needed, helping you attempt to minimize its size.

But it's not just about macros. For example, let's say I'm implementing a vector type for game math (because I *am* doing that):

````
struct Vec{N, T}
    components::NTuple{N, T}
end

# You can add and subtract vectors.
+(a::Vec, b::Vec) = Vec(map(+, a.components, b.components))
-(a::Vec, b::Vec) = Vec(map(-, a.components, b.components))
````

Already much less verbose than C++, for example I don't have to write the type parameters of `Vec` unless I really care about them (and as mentioned above, there's no performance penalty for omitting them). But it could still get annoying to rewrite this for every single operation and function. Plus, if I decide to change how they work (e.x. mark them all `@inline`), I have to fix each one by hand.

So instead, let's generate the code with a `for` loop.

````
# Name every built-in, two-parameter operator I want to overload for Vec.
# Symbols are like strings, but for metaprogramming.
const SUPPORTED_FUNCTIONS::Array{Symbol} = [
    :+, :-, :*, :/,
    :>, :<, :>=, :<=, :!=, :(==),
    :min, :max, :pow
]
# Compile each of them.
for f in SUPPORTED_FUNCTIONS
    @eval $f(a::Vec, b::Vec) = Vec(map($f, a.components, b.components))
end
````

This is also a good example of the fuzzy line between compile-time and run-time in Julia. You can think of it like `constexpr` in C++, but again, *far* less verbose, and also completely unlimited. You could define a compile-time constant by reading it in from a text file, or standard input! (Theoretically. In practice, the compiler suppresses `stdin` for some reason). You can similarly embed a file in your program merely by reading the file, and storing the contents in a `const` String or list of bytes. Anything you can do at run-time, you can also do at compile-time.

## Compile-time flags at Runtime!?

Because Julia is a JIT language, there's never a point where compile-time permanently stops. Any time you are running at the "global" level of code, outside of any function stack, you can change things. (You actually can change things from within an executing function too, but there be dragons).

For example, let's say you want to add a "debug or release?" flag for asserts. You might naively implement it as a const global:

````
const IS_DEBUG = false # Change to 'true' locally when debugging

macro assert(condition, messsage)
    # I'm skipping over a few details, but this is basically how you make a macro
    return quote
        if IS_DEBUG && !($condition)
            error($message)
        end
    end
end
````

Now you can do `@assert(some_slow_check(), "The check failed!")` and expect it to be compiled out when `IS_DEBUG` is false. But, there's actually a much more interesting way to do it. Define it as a *function* instead:

````
is_debug() = false
macro assert(condition, messsage)
    return quote
        if is_debug() && !($condition)
            error($message)
        end
    end
end
````

A key feature of Julia is the ability to add new overloads to functions, potentially replacing old ones. So now, you can switch to "debug" mode without having to edit the core codebase. Instead, you can write the following in your test scripts, outside the main project:

````
using MyGame # Bring in the main codebase, including 'is_debug()'
MyGame.is_debug() = true # Recompile everything!
MyGame.run() # Run in debug mode!
````

The JIT will invalidate and eventually recompile any functions that invoked `is_debug()`, causing the asserts to suddenly appear!

My framework uses this technique to define a global up vector and coordinate handedness. If you prefer Dear ImGUI's coordinate system over OpenGL's, you can trivially recompile the engine for left-handed, Y-down coordinates, by adding two lines at the top of your own codebase.

# The downsides

Julia language devs will always prioritize scientific computing, so there are some things that it won't do much (or at all) that gamedev cares about.

The biggest issue is mobile. Nobody is running climate simulations on their Android phone, so it's very unlikely that Julia ever gets support for running on mobile. It's also based entirely around dynamic code compilation, which could be an outright deal-breaker on AOT platforms like iOS. I'm not bothered by this as I'm coding with and for my RTX2070-powered desktop, but plenty of you will reasonably consider this a deal-breaker.

Another issue is that compilation into a static executable was an after-thought. They've been working hard on *PackageCompiler.jl*, a package that deploys Julia code as a standalone executable binary, and it does work! But it's a weird process, and the binary is massive because you still need some dynamic compilation during play and therefore you need the entire Julia runtime, which includes things like LLVM and Clang.

Finally, the language uses 1-based indexing. This is not nearly as annoying as I feared when I started using the language, *but* once you interop with OpenGL which obviously uses 0-based indexing for buffer data, off-by-1 errors become more likely.
