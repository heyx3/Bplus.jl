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
@inline function reinterpret_bytes(bytes::NTuple{N, UInt8}, first_byte::Integer, T::Type)::T where {N}
    @bp_check(isbitstype(T), "Can't convert bytes to a non-bitstype: ", T)
    let r = Ref(bytes)
        ptr = Base.unsafe_convert(Ptr{NTuple{N, UInt8}}, r)
        ptr += first_byte
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


"Takes two zipped pieces of data and unzips them into two tuples."
@inline unzip2(zipped) = (tuple((first(a) for a in zipped)...),
                          tuple((last(a) for a in zipped)...))
export unzip2

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