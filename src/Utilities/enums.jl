# Julia enums are weirdly C-like, in that their elements are global objects.
# This macro wraps them in a module to prevent name collisions.

"""
An alternative to the @enum macro, with the following differences:
    * Keeps the enum values in a singleton struct to prevent name collisions.
    * Provides `MyEnum.from(s::AbstractString)` to parse the enum.
    * Provides `MyEnum.from(i::Integer)` to convert from int to enum.
    * Provides `MyEnum.instances()` to get a tuple of the elements.
    * Provides an alias for the enum type, like `MyEnum_t`.
"""
macro bp_enum(name, args...)
    noop = Expr(:block)
    return :( @bp_enum_dep $noop $name $(args...) )
end

"""
A variant of @bp_enum for enums which have a dependency on some module.
The first argument is an expression which will be evaluated inside the enum's sub-module,
  such as `using ModernGL`.
"""
macro bp_enum_dep(definitions, name, args...)
    # Get the enum name/type.
    enum_name = nothing
    enum_type = Int32
    if (name isa Symbol)
        enum_name = name
    else
        enum_name = name.args[1]
        enum_type = name.args[2]
    end
    
    # Set up some names that will be used in the generated code.
    # Note that the "enum name" will actually be the name of the module;
    #    the @enum inside that module needs to use a different name.
    inner_name = Symbol(enum_name, :_)
    converter_name = esc(:from)
    alias_name = esc(Symbol(enum_name, :_t))
    enum_name = esc(enum_name)
    definitions = esc(definitions)

    # Generate a set of functions of the form "from(::Val{:abc}) = abc"
    #   to help with parsing from a string.
    # Also generate code that puts the elements in a tuple, for easy iteration.
    converter_dispatch = Expr(:block)
    args_tuple = Expr(:tuple)
    for arg in args
        # The enum values could be a name, or a name + a value.
        arg_name::Symbol = Meta.isexpr(arg, :(=)) ?
                               arg.args[1] :
                               arg
        arg_symbol_expr = :( Symbol($(string(arg_name))) )

        push!(converter_dispatch.args, :(
            $converter_name(::Val{$arg_symbol_expr}) = $(esc(arg_name))
        ))
        push!(args_tuple.args, esc(arg_name))
    end

    #TODO: It seems that putting this macro definition into a module breaks it.
    #TODO: Is it possible to add the @__doc__ back in safely?
    return Expr(:toplevel, :(
        module $enum_name
            $definitions
            @enum $inner_name::$enum_type $(args...)
            $converter_name(i::Integer) = $(esc(inner_name))(i)
            $converter_name(s::AbstractString) = $converter_name(Val(Symbol(s)))
            $converter_dispatch
            Base.parse(::Type{$(esc(inner_name))}, s::AbstractString) = $converter_name(Val(Symbol(s)))
            $(esc(:instances))() = $args_tuple
        end), :(
            const $alias_name = $enum_name.$inner_name
        ) )
end

export @bp_enum @bp_enum_dep