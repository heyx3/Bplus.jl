const Optional{T} = Union{T, Nothing}
@inline exists(x) = !isnothing(x)
export Optional, exists


"The inverse of `any(args...)`"
@inline none(args...) = !any(args...)
export none

"A function that combines a UnionAll and its type parameter(s)"
@inline specify(TOuter, TInner...) = TOuter{TInner...}
export specify

@inline tuple_length(T::Type{<:Tuple})::Int = length(T.parameters)
tuple_length(::Type{<:Pair}) = 2
export tuple_length

"""
A value (or values) that may or may not exist, based on a condition.
Useful for conditionally passing something into a collection or function.
E.x. `print(a, " : ", b, @optional(i>0, "  i=", i))`
"""
macro optional(condition, exprs...)
    condition = esc(condition)
    exprs = map(esc, exprs)
    return :( ($condition ? tuple($(exprs...)) : ())... )
end
"
A keyword parameter to a function that may or may not exist.
Example usage:

    my_func(; @optionalkw(a>10, my_func_arg, a))

"
macro optionalkw(condition, name, value)
    condition = esc(condition)
    name = esc(name)
    value = esc(value)
    return :( ($condition ? ($name=$value, ) : NamedTuple())... )
end
export @optional, @optionalkw


"Grabs the bytes of some bitstype, as a tuple of `UInt8`"
@inline function reinterpret_bytes(x)::NTuple{sizeof(x), UInt8}
    @bp_check(isbitstype(typeof(x)), "Can't get bytes of a non-bitstype: ", typeof(x))
    let r = Ref(x)
        ptr = Base.unsafe_convert(Ptr{typeof(x)}, r)
        ptr_bytes = Base.unsafe_convert(Ptr{NTuple{sizeof(x), UInt8}}, ptr)
        return GC.@preserve r unsafe_load(ptr_bytes)
    end
end
"Turns a subset of some bytes into a bitstype"
@inline function reinterpret_bytes(bytes::NTuple{N, UInt8}, byte_offset::Integer, T::Type)::T where {N}
    @bp_check(isbitstype(T), "Can't convert bytes to a non-bitstype: ", T)
    @bp_check(N - byte_offset >= sizeof(T),
              "Didn't provide enough bytes to create a ", T,
                ": need ", sizeof(T), ", passed ", N,
                " (with a byte offset of ", byte_offset, ")")
    let r = Ref(bytes)
        ptr = Base.unsafe_convert(Ptr{NTuple{N, UInt8}}, r)
        ptr += byte_offset
        ptrData = Base.unsafe_convert(Ptr{T}, ptr)
        return GC.@preserve r unsafe_load(ptrData)
    end
end
export reinterpret_bytes


"
An immutable alternative to Vector, using tuples.
The size is a type parameter, but you can omit it so that it's 'resizable'.
"
const ConstVector{T, N} = NTuple{N, T}
export ConstVector


"Gets the type parameter of a `Val`."
@inline val_type(::Val{T}) where {T} = T
@inline val_type(::Type{Val{T}}) where {T} = T
export val_type


"
Takes a zipped piece of data and unzips into the original iterators.

Anything that behaves like the output of `zip()` can be similarly unzipped;
    however this general implementation will iterate on `zipped` several times,
    and may not even be type-stable (we have to test it).

If the number of zipped iterators can't be deduced statically,
  you should feed it in as the second argument.
"
@inline function unzip(zipped::T, N::Integer = tuple_length(eltype(zipped))) where {T}
    # Handle the case for actual zip() data.
    if (T <: Iterators.Zip)
        return zipped.is
    else
        # Manually unzip the outputs.
        return ( (tup[i] for tup in zipped)
                   for i in 1:N )
    end
end
export unzip

"Drops the last element from an iteration"
struct drop_last{Iter}
    iter::Iter
end
function Base.iterate(d::drop_last)
    i1 = iterate(d.iter)
    if isnothing(i1)
        return nothing
    end
    (element1, state1) = i1

    i2 = iterate(d.iter, state1)
    if isnothing(i2)
        return nothing
    end
    (element2, state2) = iterate(d.iter, state1)

    return (element1, (element2, state2))
end
function Base.iterate(d::drop_last, (next_element, next_state))
    next_iter = iterate(d.iter, next_state)
    if isnothing(next_iter)
        return nothing
    end
    return (next_element, next_iter)
end
Base.IteratorSize(d::drop_last) = Base.IteratorSize(d.iter)
Base.IteratorEltype(d::drop_last) = Base.IteratorEltype(d.iter)
Base.eltype(d::drop_last) = Base.eltype(d.iter)
Base.length(d::drop_last) = Base.length(d.iter) - 1
Base.size(d::drop_last, dim...) = let s = Base.size(d.iter, dim...)
    (s isa Tuple) ? (s .- 1) : (s - 1)
end
export drop_last

"
Wraps an iterator so that it can be unwrapped by calling `pairs()`.
Each element's key in the pair is an index, using `enumerate()`.

This is needed to use iterators/generators in functions like `findfirst()`.
"
struct EnumeratedPairing{Iter}
    elements::Iter
end
Base.pairs(e::EnumeratedPairing) = enumerate(e.elements)
"
Wraps an iterator so that it can be unwrapped by calling `pairs()`.
Each element's key in the pair is an index, using `enumerate()`.
This is needed to use iterators/generators in functions like `findfirst()`.
"
enumerate_as_pair(it) = EnumeratedPairing(it)
export enumerate_as_pair

"Inserts a delimiter between each element of an iteration"
iter_join(iterable, delimiter) = drop_last(Iterators.flatten(zip(iterable, Iterators.repeated(delimiter))))
export iter_join

"A variant of 'reduce()' which skips elements that fail a certain predicate"
@inline function reduce_some(f::Func, pred::Pred, iter; init=0) where {Func, Pred}
    result = init
    for t in iter
        if pred(t)
            result = f(result, t)
        end
    end
    return result
end
export reduce_some

"A facade for using unordered collections in functions that don't normally accept them (like `map`)"
@inline iter(x) = Iterators.map(identity, x)
"Runs `map()` on unordered collections"
@inline map_unordered(f, iters...) = map(f, iter.(iters)...)
export iter, map_unordered

"Creates a vector pre-allocated for some expected size."
function preallocated_vector(::Type{T}, capacity::Int) where {T}
    v = Vector{T}(undef, capacity)
    empty!(v)
    return v
end
export preallocated_vector

"
Finds the index/key of the first element matching a desired one.
Returns `nothing` if no key was found.
"
function find_matching(target, collection, comparison = Base.:(==))::Optional
    for (key, value) in pairs(collection)
        if comparison(value, target)
            return key
        end
    end
    return nothing
end
export find_matching

"Provides `append!()` for sets, which is missing from Julia for some reason"
function Base.append!(s::AbstractSet{T}, new_items) where {T}
    for t::T in new_items
        push!(s, t)
    end
    return s
end


"
Provides a do-while loop for Julia.

Example:

```
i = 0
@do_while begin
    i += 1
end (i < 5)
@test (i == 5)
```
"
macro do_while(to_do, condition)
    return :(
        while true
            $(esc(to_do))
            $(esc(condition)) || break
        end
    )
end
export @do_while

"
Implements a module's __init__ function by delegating it to a list of callbacks.
This allows multiple independent places in your module to register some initialization behavior.
The callback list will be named `RUN_ON_INIT`.
"
macro decentralized_module_init()
    list_name = esc(:RUN_ON_INIT)
    return quote
        const $list_name = Vector{Base.Callable}()
        function $(esc(:__init__))()
            for f in $list_name
                f()
            end
        end
    end
end
export @decentralized_module_init