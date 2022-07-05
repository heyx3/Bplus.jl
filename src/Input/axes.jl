##   Data   ##

#TODO: Deadzone calculation, nonlinear scaling


##   Core definitions   ##

"
Some kind of input that can range between
    either 0,1 ('unsigned') or -1,+1 ('signed').
There are a lot of specific constraints on how an Axis should be defined,
    so it's highly recommended to use the `@bp_axis` macro to define them.

In particular:
 * It should implement `axis_value_raw(::AbstractAxis)` to read its current value
 * It should have the fields:
     * `type::Symbol = [unique serialization key]`
     * `current_raw::Float32`
 * It should add itself to the named-tuple `AXIS_SERIALIZATION_KEYS`,
     for `StructTypes` to see it
 * It should set up serialization through `StructTypes`
 * It should overload `axis_value_range()`

Convention is that it should be a mutable struct named `Axis_[X]`,
    and the serialization key should be the symbol `:[X]`.
"
abstract type AbstractAxis end
export AbstractAxis
StructTypes.StructType(::Type{AbstractAxis}) = StructTypes.AbstractType()

"Global store of what axis types there are, and what their 'key' is for de-serialization."
const AXIS_SERIALIZATION_KEYS = Ref{NamedTuple}(NamedTuple())
export AXIS_SERIALIZATION_KEYS # Exporting simplifies the @bp_axis macro.
StructTypes.subtypes(::Type{AbstractAxis}) = AXIS_SERIALIZATION_KEYS[]


##   Functions   ##

"Computes the axis's immediate value."
axis_value_raw(a::AbstractAxis, ::GLFW.Window) = error("axis_value_raw() not implemented for ", typeof(a))

function axis_update(a::AbstractAxis, window::GLFW.Window)
    a.current_raw = axis_value_raw(a, window)
end

axis_value(a::AbstractAxis) = clamp(a.current_raw, axis_value_range(a)...)
export axis_value

"Gets the inclusive min and max of the input's range"
axis_value_range(a::AbstractAxis)::NTuple{2, Float32} = axis_value_range(typeof(a))
axis_value_range(T::Type{<:AbstractAxis}) = error("axis_value_range() not implemented for ", T)
export axis_value_range


##   Axis definitions   ##

"""
Defines an input axis (a struct inheriting from `Bplus.Input.AbstractAxis`).
Refer to *Bplus/src/Input/axes.jl* for examples that show the full range of functionality.

Below is one example, of a signed axis that is triggered from two keys pressed at once:
````
@bp_axis signed DualKey "Triggered from two simultaneous keypresses" begin
    key1::GLFW.Key
    key2::GLFW.Key

    # Compute the value ('window' argument is optional):
    RAW(a, window) = if (GLFW.GetKey(window, a.key1) && GLFW.GetKey(window, a.key2))
                         1.0
                     else
                         -1.0
                     end

    # Custom constructor that sets key2 to left-ctrl:
    function Key(key1::GLFW.Key)
        Key(key1=key1, key2=GLFW.KEY_LEFT_CONTROL)
    end

    # Optional update logic before axis update ('window' argument is optional):
    UPDATE(a, window) = println("tick")
end
````

The new axis type can be created with
  `Axis_Key(window=my_window)`.
"""
macro bp_axis(signed_value, name, definition)
    # Do error-checking.
    if !isa(name, Symbol) && !Meta.isexpr(name, :curly)
        error("Axis name must be a token (optionally with type parameters),",
              " but was ", typeof(name), "(", name, ")")
    elseif !Meta.isexpr(definition, :block)
        error("Axis definition must be a `begin ... end` block, but was ",
              (definition isa Expr) ? definition.head : definition)
    elseif !in(signed_value, (:signed, :unsigned))
        error("First parameter to @bp_axis must be 'signed' or 'unsigned'")
    end

    # Process signed/unsigned.
    is_signed::Bool = (signed_value == :signed)
    value_min::Float32 = is_signed ? -one(Float32) : zero(Float32)
    value_max::Float32 = one(Float32)
    value_resting::Float32 = zero(Float32)

    # Generate names.
    local short_name::Symbol,
          struct_name::Symbol,
          struct_def,
          struct_name_typed,
          type_params::Optional{Vector},
          type_param_names::Vector{Symbol},
          type_name_str::String
    if name isa Symbol
        short_name = name
        struct_name = Symbol(:Axis_, name)
        struct_name_typed = struct_name
        struct_def = struct_name
        type_name_str = string(name)
        type_params = nothing
        type_param_names = [ ]
    elseif Meta.isexpr(name, :curly)
        short_name = name.args[1]
        struct_name = Symbol(:Axis_, short_name)
        struct_def = deepcopy(name)
        struct_def.args[1] = struct_name
        type_name_str = string(short_name)
        type_params = collect(Union{Symbol, Expr}, name.args[2:end])
        type_param_names = map(type_params) do tp
            if tp isa Symbol
                return tp
            elseif isexpr(tp, :<:)
                return tp.args[1]
            else
                error("Unhandled case: ", tp)
            end
        end
        struct_name_typed = :( $struct_name{$(type_param_names...)} )
    else
        error("Unhandled case: ", name)
    end

    definitions = definition.args

    # Get the fields and constructors.
    definitions_split = collect(Union{Nothing, Dict{Symbol, Any}, Tuple{Symbol, Any, Bool, Optional{Some}}},
      map(definitions) do def
        if is_function_def(def)
            return splitdef(def)
        elseif is_field_def(def)
            data = splitarg(def)
            # Distinguish between a default value of 'nothing',
            #    and no default value.
            if isexpr(def, :(=))
                return (data[1:3]..., Some(data[4]))
            else
                return data
            end
        else
            return nothing
        end
    end)
    filter!(exists, definitions_split)
    # Inject the fields that all Axes should have.
    insert!.(Ref(definitions_split), Ref(1), [
        (:type, Symbol, false, Some(:( Symbol($(type_name_str)) ))),
        (:current_raw, Float32, false, Some(value_resting))
    ])
    # A constructor will be generated automatically.
    # We also need to ensure there's an empty constructor for deserialization.
    ordered_constructor_params = [ ]
    kw_constructor_params = [ ]
    constructor_passed_args = [ ]
    n_unset_fields::Int = 0
    has_empty_constructor::Bool = false
    definitions_impl = map(definitions_split) do def_data
        # Is it a field?
        if def_data isa Tuple # Result of calling MacroTools.splitarg(def),
                              #    and wrapping default_value in Some()
            (field_name, field_type, is_slurped, some_default_value) = def_data
            @assert(!is_slurped, "What does it mean for a field to be slurped??")

            if isnothing(some_default_value)
                n_unset_fields += 1
            end

            # If no default is given, then it must be provided as an ordered parameter.
            # Otherwise, it can optionally be set from a keyword parameter.
            push!(constructor_passed_args, :( convert($field_type, $field_name) ))
            if isnothing(some_default_value)
                push!(ordered_constructor_params, :( $field_name ))
            else
                push!(kw_constructor_params,
                      Expr(:kw, field_name, something(some_default_value)))
            end
            return :( $field_name::$field_type )
        # Is it a constructor?
        elseif def_data isa Dict # Result of calling MacroTools.splitdef(def).
            if def_data[:name] in (:RAW, :UPDATE) # Actually a function implementation,
                                                  #    skip it for now
                return :( )
            elseif def_data[:name] == short_name # A constructor
                if isempty(def_data[:args])
                    has_empty_constructor = true
                end
                # The user gave the short name of the struct.
                # Modify the function definition to support that.
                def_data[:name] = struct_name
                # Let the short name map to the long name.
                def_data[:body] = quote
                    let $short_name = $struct_name
                        $(def_data[:body])
                    end
                end
                return combinedef(def_data)
            else
                error("Unexpected function syntax in @bp_axis: ", combinedef(def_data))
            end
        end
    end
    # Finish generating the @kwdef-like constructor.
    if exists(type_params)
        push!(definitions_impl, :(
            $struct_name_typed($(ordered_constructor_params...); $(kw_constructor_params...)) where {$(type_params...)} =
                new{$(type_param_names...)}($(constructor_passed_args...))
        ))
    else
        push!(definitions_impl, :(
            $struct_def($(ordered_constructor_params...); $(kw_constructor_params...)) = new($(constructor_passed_args...))
        ))
    end
    # If it doesn't have an empty constructor, provide one for serialization.
    if (n_unset_fields > 0) && !has_empty_constructor
        if isnothing(type_params)
            push!(definitions_impl, :( $struct_def() = new() ))
        else
            push!(definitions_impl, :( $struct_name_typed() where {$(type_params...)} = new{$(type_param_names...)}() ))
        end
    end

    # Get the "raw value" implementation.
    local raw_impl::Expr
    raw_decls = filter(definitions_split) do def_data
        (def_data isa Dict) && (def_data[:name] == :RAW)
    end
    if length(raw_decls) < 1
        error("No axis implementation found for ", name,
              " (should look like 'RAW(a) = ...')")
    elseif length(raw_decls) > 1
        error("More than one definition of RAW for axis ", name)
    end
    raw_decl = raw_decls[1]
    raw_decl[:name] = :(Input.axis_value_raw)
    if isempty(raw_decl[:args])
        push!(raw_decl[:args], :( _::$(esc(struct_name_typed)) ))
    elseif !isa(raw_decl[:args][1], Symbol)
        error("The 'axis' parameter for RAW() should be untyped!",
              " The type is added automatically")
    else
        raw_decl[:args][1] = :( $(raw_decl[:args][1])::$(esc(struct_name_typed)) )
    end
    if length(raw_decl[:args]) < 2
        push!(raw_decl[:args], :( _::GLFW.Window ))
    elseif !isa(raw_decl[:args][2], Symbol)
        error("The 'window' parameter for RAW() should be untyped1",
              " The type is added automatically")
    else
        raw_decl[:args][2] = :( $(raw_decl[:args][2])::GLFW.Window )
    end
    if exists(type_params)
        raw_decl[:whereparams] = map(esc, tuple(type_params...))
    end
    raw_impl = combinedef(raw_decl)

    # Get the "update" implementation.
    update_impl::Expr = :( )
    update_decls = filter(definitions_split) do def_data
        (def_data isa Dict) && (def_data[:name] == :UPDATE)
    end
    if length(update_decls) > 1
        error("More than one definition of UPDATE for axis ", name)
    elseif length(update_decls) == 1
        update_decl = update_decls[1]
        update_decl[:name] = :(Input.axis_update)
        # Add the first 'axis' argument if not given.
        local axis_name
        if isempty(update_decl[:args])
            axis_name = :a
            push!(update_decl[:args], :( $axis_name::$(esc(struct_name_typed)) ))
        elseif !isa(update_decl[:args][1], Symbol)
            error("The 'axis' parameter for UPDATE() should be untyped!",
                " The type is added automatically")
        else
            axis_name = update_decl[:args][1]
            update_decl[:args][1] = :( $axis_name::$(esc(struct_name_typed)) )
        end
        # Add the second 'window' argument if not given.
        window_name = :wnd
        if length(update_decl[:args]) < 2
            push!(update_decl[:args], :( $window_name::GLFW.Window ))
        elseif !isa(update_decl[:args][2], Symbol)
            error("The 'window' parameter for UPDATE() should be untyped1",
                " The type is added automatically")
        else
            window_name = update_decl[:args][2]
            update_decl[:args][2] = :( $window_name::GLFW.Window )
        end
        # Invoke the user's update behavior first, then the normal behavior.
        # Add the function alias 'UPDATE' for convenience.
        update_decl[:body] = quote
            try let UPDATE = ($(esc(:axis))::$(esc(struct_name_typed)) = $axis_name, $(esc(:window))::GLFW.Window = $window_name) ->
                                 $(update_decl[:name])(axis, window)
                $(update_decl[:body])
            end
            finally
                invoke($(update_decl[:name]), Tuple{$AbstractAxis, $(GLFW.Window)},
                       $axis_name, $window_name)
            end
        end
        if exists(type_params)
            update_decl[:whereparams] = map(esc, tuple(type_params...))
        end
        update_impl = combinedef(update_decl)
    end

    struct_impl = :( Core.@__doc__ mutable struct $struct_def <: AbstractAxis
        $(definitions_impl...)
    end )
    output = quote
        if haskey(AXIS_SERIALIZATION_KEYS[], Symbol($type_name_str))
            error("Axis named '", $type_name_str, "' already exists")
        end

        $struct_impl
        $(:(Input.axis_value_range))(::Type{<:$(esc(struct_name))}) = ($value_min, $value_max)

        StructTypes.StructType(::Type{<:$(esc(struct_name))}) = StructTypes.Mutable()
        StructTypes.excludes(::Type{<:$(esc(struct_name))}) = tuple(:current_raw)

        if $(!isnothing(type_params))
            @warn "Axis '$($type_name_str)' has type parameters, and so StructTypes \
cannot deserialize it through the abstract type `AbstractAxis`."
        else
            AXIS_SERIALIZATION_KEYS[] = (AXIS_SERIALIZATION_KEYS[]...,
                                           $(Symbol(type_name_str)) = $(esc(struct_name)), )
        end

        $raw_impl
        $update_impl
    end
    return output
end
export @bp_axis


"An unsigned axis that's stuck at 0 until triggered by a keyboard key, which sets it to 1."
@bp_axis unsigned Key begin
    key::SerializedUnion{InputKey}
    Key(k::InputKey; kw...) = Key(SerializedUnion{InputKey}(k); kw...)
    RAW(a, wnd) = (check_key(a.key.data, wnd) ? 1 : 0)
end
"A signed axis that's stuck at 0, pulled towards -1 by one key, and towards +1 by another key."
@bp_axis signed Key2 begin
    key_positive::SerializedUnion{InputKey}
    key_negative::SerializedUnion{InputKey}
    RAW(a, wnd) = (0 +
                   (check_key(a.key_positive.data, wnd) ? 1 : 0) +
                   (check_key(a.key_negative.data, wnd) ? -1 : 0))
end
"A 0-1 axis that's triggered by a mouse click"
@bp_axis unsigned Mouse begin
    button::GLFW.MouseButton = GLFW.MOUSE_BUTTON_LEFT
    RAW(a, wnd) = (GLFW.GetMouseButton(wnd, a.button) ? 1 : 0)
end

"The sum of several 'child' axes. Marked as 'signed' because that has the largest range."
@bp_axis signed Sum begin
    children::Vector{AbstractAxis}
    RAW(a) = sum(axis_value, a.children, init=zero(Float32))
    UPDATE(a) = for c in children
        UPDATE(c)
    end
end

"
An axis that keeps whatever value you set it to (useful for testing).
Controlled through its `current_raw` field.

Marked as 'signed' because that has the largest range.
"
@bp_axis signed Manual begin
    RAW(a) = a.current_raw
end

export Axis_Key, Axis_Key2, Axis_Mouse, Axis_Sum, Axis_Manual