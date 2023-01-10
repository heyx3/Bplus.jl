
"
Union of some outer type, specialized for many inner types.
Examples:
* `@unionspec(Vector{_}, Int, Float64) == Union{Vector{Int}, Vector{Float64}}`
* `@unionspec(Array{Bool, _}, 2, 4) == Union{Array{Bool, 2}, Array{Bool, 4}}`
"
macro unionspec(TOuter, TInners...)
    # Search through tOut for any underscore tokens,
    #    and replace them with tIn.
    function specialize_unders(tOut, tIn)
        if tOut isa Symbol
            if tOut == :_
                return tIn
            else
                return tOut
            end
        elseif tOut isa Expr
            return Expr(tOut.head, map(arg -> specialize_unders(arg, tIn), tOut.args)...)
        else #LineNumberNode or something else to leave alone
            return tOut
        end
    end

    specializations = map(tIn -> esc(specialize_unders(TOuter, tIn)),
                          TInners)

    return :( Union{$(specializations...)} )
end
export @unionspec


"Gets a tuple of the types in a Union."
@inline union_types(T) = (T, )
@inline function union_types(u::Union)
    # Supposedly, 'u.a' is the first type and 'u.b' is a Union of the other types.
    # However, sometimes a is the Union, and b is the type.
    # So I pass both sides through 'union_types()' to be sure.
    (union_types(u.a)..., union_types(u.b)...)
end
export union_types


"
When deserializing a union of types, this determines the order that the types are tried in.
Lower-priority values are tried first.
"
union_ordering(T::Type)::Float64 =
    # If this is an AbstractType, give it a specific priority.
    # Otherwise, unknown/user-made types are given the lowest priority.
    (StructTypes.StructType(T) isa StructTypes.AbstractType) ?
        union_ordering(Val(:abstract)) :
        +Inf
#
union_ordering(::Type{<:Enum}) = 0.0
union_ordering(::Type{<:Integer}) = 1.0
union_ordering(::Type{<:AbstractFloat}) = 2.0
union_ordering(::Type{<:Dates.TimeType}) = 3.0
union_ordering(::Type{Symbol}) = 4.0
union_ordering(::Type{<:AbstractString}) = 5.0
union_ordering(::Val{:abstract}) = 6.0
#TODO: Make the dictionary and vector priorities dynamic based on the priority of their input types
union_ordering(::Type{<:AbstractDict}) = 7.0
union_ordering(::Type{<:AbstractVector}) = 8.0
const MAX_BUILTIN_UNION_PRIORITY = union_ordering(Vector)
export union_ordering


"Organizes the types in a union by their `union_ordering()`."
@inline union_parse_order(U::Union) = sort(collect(union_types(U)), by=union_ordering)
union_parse_order(T::Type) = (T, )
#NOTE: originally I used TupleTools.sort(), but it breaks
#    for unions containing DataTypes and UnionAll's, because
#    those are two different types of things, so the tuple isn't homogenous.
export union_parse_order


"
A Union of types that can be serialized/deserialized through `StructTypes`.
Any types that can individually be handled by `StructTypes` should also be usable in this union.
It is not super efficient, as it uses try/catch
    to find the first type that successfully deserializes.
"
struct SerializedUnion{U}
    data::U
end
"Alias for `SerializedUnion{Union{...}}`"
macro SerializedUnion(types...)
    return :( $SerializedUnion{Union{$(map(esc, types)...)}} )
end
export SerializedUnion, @SerializedUnion

Base.convert(::Type{<:U}, su::SerializedUnion{U}) where {U} = su.data
Base.convert(::Type{SerializedUnion{U}}, data::U) where {U} = SerializedUnion{U}(data)
Base.:(==)(u::SerializedUnion{U}, data::U) where {U} = (u.data == data)
Base.:(==)(data::U, u::SerializedUnion{U}) where {U} = (u.data == data)
Base.hash(un::SerializedUnion, u::UInt) = hash(un.data, u)

StructTypes.StructType(::Type{<:SerializedUnion}) = StructTypes.CustomStruct()
StructTypes.lower(u::SerializedUnion) = u.data
function StructTypes.construct(::Type{SerializedUnion{U}}, incoming::T) where {U, T}
    failed_attempts = [ ]
    for UT in union_parse_order(U)
        try
            return SerializedUnion{U}(StructTypes.construct(UT, incoming))
        catch e
            push!(failed_attempts, e)
        end
    end
    error("Unable to deserialize. Errors from attempts:\n",
            join(failed_attempts, "\n\t"))
end