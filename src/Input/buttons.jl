##   Data   ##

# The different ways a button can be pressed
@bp_enum(ButtonModes,
    is_on, is_off,
    just_pressed, just_released
)
export E_ButtonModes, ButtonModes

evaluate_button(mode::E_ButtonModes, previous::Bool, current::Bool)::Bool = (
    if mode == ButtonModes.is_on
        current
    elseif mode == ButtonModes.is_off
        !current
    elseif mode == ButtonModes.just_pressed
        current && !previous
    elseif mode == ButtonModes.just_released
        !current && previous
    else
        error("Unhandled case: ", mode)
    end
)


##   Core definitions   ##

"
Some kind of input that can be `on` or `off`.
There are a lot of specific constraints on how a button should be defined,
    so it's highly recommended to use the `@bp_button` macro to define them.

In particular:
 * It should implement `button_value_raw(::AbstractButton)` to read the input's current value
     (without regard for `ButtonModes`)
 * It should have the fields:
     * `mode::E_ButtonModes = BM_IsOn`
     * `type::Symbol = [unique serialization key]`
     * `current_raw::Bool = false`
     * `previous_raw::Bool = false`
 * It should add itself to the named-tuple `BUTTON_SERIALIZATION_KEYS`,
     for `StructTypes` to see it
 * It should set up serialization through `StructTypes`

Convention is that it should be a `Base.@kwdef` mutable struct named `Button_[X]`,
    and the serialization key should be the symbol `:[X]`.
"
abstract type AbstractButton end
export AbstractButton
StructTypes.StructType(::Type{AbstractButton}) = StructTypes.AbstractType()

"Global store of what button types there are, and what their 'key' is for de-serialization."
const BUTTON_SERIALIZATION_KEYS = Ref{NamedTuple}(NamedTuple())
export BUTTON_SERIALIZATION_KEYS # Exporting simplifies the @bp_button macro.
StructTypes.subtypes(::Type{AbstractButton}) = BUTTON_SERIALIZATION_KEYS[]


##   Functions   ##

"Computes the button's immediate value (not considering `ButtonModes`."
button_value_raw(b::AbstractButton, ::GLFW.Window) = error("button_value_raw() not implemented for ", typeof(b))

function button_update(b::AbstractButton, window::GLFW.Window)
    b.previous_raw = b.current_raw
    b.current_raw = button_value_raw(b, window)
end

button_value(b::AbstractButton) = evaluate_button(b.mode, b.previous_raw, b.current_raw)
export button_value


##   Button definitions   ##

"""
Defines an input button (a struct inheriting from `Bplus.Input.AbstractButton`).
Refer to *Bplus/src/Input/buttons.jl* for examples that show the full range of functionality.

Below is one example, of a button that is triggered from two keys pressed at once:
````
@bp_button DualKey "Triggered from two simultaneous keypresses" begin
    key1::GLFW.Key
    key2::GLFW.Key

    # Compute the value ('window' argument is optional):
    RAW(b, window) = GLFW.GetKey(window, b.key1) && GLFW.GetKey(window, b.key2)

    # Custom constructor that sets key2 to left-ctrl:
    function Key(key1::GLFW.Key)
        Key(key1=key1, key2=GLFW.KEY_LEFT_CONTROL)
    end

    # Optional update logic after button update ('window' argument is optional):
    UPDATE(b, window) = println("tick")
end
````

The new button type can be created with
  `Button_Key(window=my_window[, mode=ButtonModes.just_pressed])`.
"""
macro bp_button(name, definition)
    # Do error-checking.
    if !isa(name, Symbol) && !Meta.isexpr(name, :curly)
        error("Button name must be a token (optionally with type parameters),",
              " but was ", typeof(name), "(", name, ")")
    elseif !Meta.isexpr(definition, :block)
        error("Button definition must be a `begin ... end` block, but was ",
              (definition isa Expr) ? definition.head : definition)
    end

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
        struct_name = Symbol(:Button_, name)
        struct_name_typed = struct_name
        struct_def = struct_name
        type_name_str = string(name)
        type_params = nothing
        type_param_names = [ ]
    elseif Meta.isexpr(name, :curly)
        short_name = name.args[1]
        struct_name = Symbol(:Button_, short_name)
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
    # Inject the fields that all Buttons should have.
    insert!.(Ref(definitions_split), Ref(1), [
        (:type, Symbol, false, Some(:( Symbol($(type_name_str)) ))),
        (:mode, E_ButtonModes, false, Some(ButtonModes.is_on)),
        (:previous_raw, Bool, false, Some(false)),
        (:current_raw, Bool, false, Some(false))
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
            push!(constructor_passed_args, field_name)
            if isnothing(some_default_value)
                push!(ordered_constructor_params, :( $field_name::$field_type ))
            else
                push!(kw_constructor_params,
                      Expr(:kw, :( $field_name::$field_type ), something(some_default_value)))
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
                error("Unexpected function syntax in @bp_button: ", combinedef(def_data))
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
        error("No button implementation found for ", name,
              " (should look like 'RAW(b) = ...')")
    elseif length(raw_decls) > 1
        error("More than one definition of RAW for button ", name)
    end
    raw_decl = raw_decls[1]
    raw_decl[:name] = :(Input.button_value_raw)
    if isempty(raw_decl[:args])
        push!(raw_decl[:args], :( _::$(esc(struct_name_typed)) ))
    elseif !isa(raw_decl[:args][1], Symbol)
        error("The 'button' parameter for RAW() should be untyped!",
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
        error("More than one definition of UPDATE for button ", name)
    elseif length(update_decls) == 1
        update_decl = update_decls[1]
        update_decl[:name] = :(Input.button_update) #TODO: Try interpolating the literal function again
        # Add the first 'button' argument if not given.
        local button_name
        if isempty(update_decl[:args])
            button_name = :b
            push!(update_decl[:args], :( $button_name::$(esc(struct_name_typed)) ))
        elseif !isa(update_decl[:args][1], Symbol)
            error("The 'button' parameter for UPDATE() should be untyped!",
                " The type is added automatically")
        else
            button_name = update_decl[:args][1]
            update_decl[:args][1] = :( $button_name::$(esc(struct_name_typed)) )
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
            try let UPDATE = ($(esc(:button))::$(esc(struct_name_typed)) = $button_name, $(esc(:window))::GLFW.Window = $window_name) ->
                                 $(update_decl[:name])(button, window)
                $(update_decl[:body])
            end
            finally
                invoke($(update_decl[:name]), Tuple{$AbstractButton, $(GLFW.Window)},
                       $button_name, $window_name)
            end
        end
        if exists(type_params)
            update_decl[:whereparams] = map(esc, tuple(type_params...))
        end
        update_impl = combinedef(update_decl)
    end

    struct_impl = :( Core.@__doc__ mutable struct $struct_def <: AbstractButton
        $(definitions_impl...)
    end )
    output = quote
        if haskey(BUTTON_SERIALIZATION_KEYS[], Symbol($type_name_str))
            error("Button named '", $type_name_str, "' already exists")
        end

        $struct_impl

        StructTypes.StructType(::Type{<:$(esc(struct_name))}) = StructTypes.Mutable()
        StructTypes.excludes(::Type{<:$(esc(struct_name))}) = tuple(:previous_raw, :current_raw)

        if $(!isnothing(type_params))
            @warn "Button '$($type_name_str)' has type parameters, and so StructTypes \
cannot deserialize it through the abstract type `AbstractButton`."
        else
            BUTTON_SERIALIZATION_KEYS[] = (BUTTON_SERIALIZATION_KEYS[]...,
                                           $(Symbol(type_name_str)) = $(esc(struct_name)), )
        end

        $raw_impl
        $update_impl
    end
    return output
end
export @bp_button


@bp_enum AggregateKeys alt ctrl shift os
"
For some keys that appear on both sides of the keyboard,
    you can use a special alias that means 'either key'.
"
const InputKey = Union{GLFW.Key, E_AggregateKeys}
export AggregateKeys, E_AggregateKeys
export InputKey

check_key(input::GLFW.Key, window::GLFW.Window) = GLFW.GetKey(window, input)
check_key(input::E_AggregateKeys, window::GLFW.Window) = (
    if input == AggregateKeys.alt
        GLFW.GetKey(window, GLFW.KEY_LEFT_ALT) ||
        GLFW.GetKey(window, GLFW.KEY_RIGHT_ALT)
    elseif input == AggregateKeys.ctrl
        GLFW.GetKey(window, GLFW.KEY_LEFT_CONTROL) ||
        GLFW.GetKey(window, GLFW.KEY_RIGHT_CONTROL)
    elseif input == AggregateKeys.shift
        GLFW.GetKey(window, GLFW.KEY_LEFT_SHIFT) ||
        GLFW.GetKey(window, GLFW.KEY_RIGHT_SHIFT)
    elseif input == AggregateKeys.os
        GLFW.GetKey(window, GLFW.KEY_LEFT_SUPER) ||
        GLFW.GetKey(window, GLFW.KEY_RIGHT_SUPER)
    else
        error("Unhandled case: ", input)
    end
)


"
A button that's triggered by a keyboard key.
For some keys that appear on both sides of the keyboard,
    you can use a special alias that means 'either key'.
"
@bp_button Key begin
    key::SerializedUnion{InputKey}
    Key(k::GLFW.Key; kw...) = Key(SerializedUnion{InputKey}(k); kw...)
    Key(k::E_AggregateKeys; kw...) = Key(SerializedUnion{InputKey}(k); kw...)
    RAW(b, wnd) = check_key(b.key, wnd)
end
"A button that's triggered by a mouse click"
@bp_button Mouse begin
    button::GLFW.MouseButton = GLFW.MOUSE_BUTTON_LEFT
    RAW(b, wnd) = GLFW.GetMouseButton(wnd, b.button)
end

"A button that's triggered by any one of a group of buttons"
@bp_button Any begin
    children::Vector{AbstractButton} = AbstractButton[ ]
    RAW(b) = any(button_value(c) for c in b.children)
    UPDATE(b) = for c in children
        UPDATE(c)
    end
end
"A button that's triggered if all buttons in a group are simultaneously on"
@bp_button All begin
    children::Vector{AbstractButton} = AbstractButton[ ]
    RAW(b) = all(button_value(c) for c in b.children)
    UPDATE(b) = for c in children
        UPDATE(c)
    end
end

"
A button that's on or off when you say it is (useful for testing).
    Controlled through its `current_raw` field.
"
@bp_button Manual begin
    RAW(b) = b.current_raw
end

export Button_Key, Button_Mouse, Button_Any, Button_All, Button_Manual