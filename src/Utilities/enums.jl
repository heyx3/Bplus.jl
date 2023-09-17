

"
Anonymous enums, implemented with Val.
A stricter alternative to passing symbols as parameters.
`@ano_enum(ABC, DEF, GHI) == Union{Val{:ABC}, Val{:DEF}, Val{GHI}}`
Note that this creates some overhead if the user can't construct the Val at compile-time.
"
macro ano_enum(args::Symbol...)
    arg_names = map(string, args)
    arg_exprs = map(s -> :(Val{Symbol($s)}), arg_names)
    return :( Union{$(arg_exprs...)} )
end
export @ano_enum


"""
An alternative to the @enum macro, with the following differences:
* Keeps the enum values in a module to prevent name collisions.
* Provides an overload of `base.parse()` to parse the enum from a string.
* Provides `MyEnum.from(i::Integer)` to convert from int to enum.
* Provides `MyEnum.from_index(i::Integer)` to get an enum value
    from its index in the original declaration.
* Provides `MyEnum.to_index(e)` to get the index of an enum value
    in the original declaration.
* Provides `MyEnum.instances()` to get a tuple of the elements.
* Provides an alias for the enum type, `E_MyEnum`.

If you wish to add definitions inside the enum module (e.x. import a package),
  make your own custom macro that returns `generate_enum()`.
  put a `begin ... end` block just after the enum name, containing the desired code.

If you want your enum to act like a bitfield, use `@bp_bitflag` instead.
"""
macro bp_enum(name, args...)
    return generate_enum(name, :(begin end), args, false)
end
#NOTE: Nobody needs to use 'generate_enum' externally, just eval the data inside the invoker's module.
#      However, it already works and would be kinda arduous to change.

"""
Combines the features of `@bp_enum` with the bitfield behavior of `@bitflag` from *BitFlags.jl*.
Along with the usual `@bp_enum` definitions, you also get:

* Power-of-two element numbering by default (if you want a 0 element, put it at the beginning).
* Aggregate elements with the syntax `@element_name a|b|c`. Note that these are not enumerated in `instances()`.
* `a | b` to combine two bitflags.
* `a & b` to filter bitflags.
* `a - b` to remove bitflags (equivalent to `a & (~b)`).
* `Base.contains(value, element)::Bool` as a short-hand for `(value & element) != 0`
* `ALL` as a value representing all elements of the bitfield enabled
* Pretty printing of combination values.
"""
macro bp_bitflag(name, args...)
    return generate_enum(name, :(begin end), args, true)
end
Base.contains


"""
The inner logic of @bp_enum.
Also takes a block of "definitions", in case something needs to be imported into the enum's module.
"""
function generate_enum(name, definitions, args, is_bitfield::Bool)
    # Get the enum name/type.
    process_enum_decl(expr) =
        if expr isa Symbol
            (expr, is_bitfield ? UInt32 : Int32)
        elseif Meta.isexpr(name, :(::))
            (name.args[1], name.args[2])
        elseif Meta.isexpr(name, :escape)
            process_enum_decl(expr.args[1])
        else
            error("Unknown type of expression for enum name: ",
                      name.head, "\n\t", sprint(dump, name))
        end
    enum_name, enum_type = process_enum_decl(name)

    # Set up some names that will be used in the generated code.
    # Note that the "enum name" will actually be the name of the module;
    #    the @enum inside that module needs to use a different name.
    inner_name = Symbol(enum_name, :_)
    converter_name = :from
    index_converter_from_name = :from_index
    index_converter_to_name = :to_index
    alias_name = Symbol(:E_, enum_name)

    # For bitfields, extract special aggregate values.
    # They are not part of the actual enum definition.
    bitflag_aggregates = Expr[ ]
    #TODO: Keep aggregates in 'args', rewritten like normal elements, but not in @enum.
    args = filter(args) do arg
        if @capture arg @agname_(agvalue_)
            agname = Symbol(string(agname)[2:end]) # Chop off the '@' symbol
            push!(bitflag_aggregates, :( const $agname = $agvalue ))
            return false
        elseif isexpr(arg, :macrocall)
            error("Invalid format for aggregate bitflag: ", arg)
        else
            return true
        end
    end

    # Generate a set of functions of the form "from(::Val{:abc}) = abc"
    #   to help with parsing from a string.
    # Also generate code that puts the elements in a tuple, for easy iteration.
    # Thirdly, un-escape the arguments so we can pass them into the @enum call.
    converter_dispatch = Expr(:block)
    args_tuple = Expr(:tuple)
    args = map(args) do arg
        while Meta.isexpr(arg, :escape)
            arg = arg.args[1]
        end
        # The enum values could be a name, or a name + a value.
        if Meta.isexpr(arg, :(=))
            arg_name::Symbol = arg.args[1]
        elseif arg isa Symbol
            arg_name = arg
        else
            error("Unexpected enum argument: ", arg, "\n\t", sprint(dump, arg))
        end
        arg_symbol_expr = :( Symbol($(string(arg_name))) )

        push!(converter_dispatch.args, :(
            $converter_name(::Val{$arg_symbol_expr}) = $arg_name
        ))
        push!(args_tuple.args, arg_name)

        return Meta.isexpr(arg, :escape) ? arg.args[1] : arg
    end

    main_macro = is_bitfield ?
                     :( @bitflag $inner_name::$enum_type $(args...) ) :
                     :( @enum $inner_name::$enum_type $(args...) )

    output = Expr(:toplevel, esc(:(
        module $enum_name
            $definitions
            $(is_bitfield ? :( using BitFlags ) : :())
            $main_macro
            $(bitflag_aggregates...)
            $converter_name(i::Integer) = $inner_name(i)
            $converter_name(s::AbstractString) = $converter_name(Val(Symbol(s)))
            @inline $index_converter_from_name(i::Integer) = instances()[i]
            @inline $index_converter_to_name(e::$inner_name) = begin
                for (key, value) in pairs(instances())
                    if value == e
                        return key
                    end
                end
                return nothing
            end
            $converter_dispatch
            Base.parse(::Type{$inner_name}, s::AbstractString) = $converter_name(Val(Symbol(s)))
            instances() = $args_tuple
            # Add support for passing an array of enum values into a C function
            #    as if it's an array of the underlying type.
            Base.unsafe_convert(::Type{Ptr{$enum_type}}, r::Ref{$inner_name}) =
                Base.unsafe_convert(Ptr{$enum_type}, Base.unsafe_convert(Ptr{Nothing}, r))

            # Bitflags-specific operations:
            $(!is_bitfield ? :() : quote
                Base.:(-)(a::$inner_name, b::$inner_name) =
                    $inner_name($enum_type(a) & (~$enum_type(b)))
                Base.contains(haystack::$inner_name, needle::$inner_name)::Bool =
                    ($enum_type(haystack) & $enum_type(needle)) != zero($enum_type)

                const ALL = let a = instances()[1]
                    for inst in instances()
                        a |= inst
                    end
                    a
                end
            end)
        end)),
        esc(:(
            const $alias_name = $enum_name.$inner_name
        ))
    )
    return output
end

export @bp_enum, @bp_bitflag