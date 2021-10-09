# Julia enums are weirdly C-like, in that their elements are global objects.
# This macro wraps them in a module to prevent name collisions.

"""
Identical to the @enum macro, with the following differences:
    * Keeps the enum values in a module to prevent name collisions with outside code.
    * Provides `MyEnum.from(s::AbstractString)` to parse the enum.
    * Provides `MyEnum.from(i::Integer)` to convert from int to enum.
    * Provides an alias for the enum type, like `MyEnum_t`.
"""
macro bp_enum(name, args...)
    # Get the enum name/type.
    enum_name::Symbol = :_
    enum_type = Int32
    if name isa Symbol
        enum_name = name
    else
        enum_name = name.args[1]
        enum_type = name.args[2]
    end
    
    # The "enum name" will actually be the name of the module.
    # The @enum inside that module needs to use a different name.
    inner_name = Symbol(enum_name, :_)

    # The name of the function that converts from int/string data to the enum.
    converter_name = esc(:from)

    # Generate a set of functions of the form "from(::Val{:abc}) = abc"
    #   to help with parsing from a string.
    converter_dispatch = Expr(:block)
    for arg in args
        # The enum values could be a name, or a name + a value.
        arg_name::Symbol = Meta.isexpr(arg, :(=)) ?
                               arg.args[1] :
                               arg
        arg_symbol_expr = :( Symbol($(string(arg_name))) )
        push!(converter_dispatch.args, :(
            $converter_name(::Val{$arg_symbol_expr}) = $(esc(arg_name))
        ))
    end

    # Along with defining the enum/module,
    #    define a readable alias for the enum type.
    return Expr(:block,
        # The :toplevel Expr is needed to define a module in a quote block.
        Expr(:toplevel, :(
            module $(esc(enum_name))
                Base.@__doc__( @enum $inner_name::$enum_type $(args...) )
                $converter_name(i::Integer) = $(esc(inner_name))(i)
                $converter_name(s::AbstractString) = $converter_name(Val(Symbol(s)))
                $converter_dispatch
                Base.parse(::Type{$(esc(inner_name))}, s::AbstractString) = $converter_name(Val(Symbol(s)))
            end
        ) ),
        # Provide the "MyEnum_t" type alias.
        quote
            const $(esc(Symbol(enum_name, :_t))) = $(esc(enum_name)).$inner_name
        end
    )
end
export @bp_enum