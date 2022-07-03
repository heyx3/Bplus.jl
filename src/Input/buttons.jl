##   Data   ##

"The different ways a button can be pressed."
@bp_enum(ButtonModes,
    is_on, is_off,
    just_pressed, just_released
)
export ButtonModes

evaluate_button(mode::ButtonModes, previous::Bool, current::Bool)::Bool = (
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
     * `mode::ButtonModes = BM_IsOn`
     * `type::Symbol = [unique serialization key]`
     * `current_raw::Bool = false`
     * `previous_raw::Bool = false`
 * It should add itself to the named-tuple `BUTTON_SERIALIZATION_KEYS`,
     for `StructTypes` to see it

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
button_value_raw(b::AbstractButton, window::GLFW.Window) = error("button_value_raw() not implemented for ", typeof(b))
export button_value_raw # Exporting simplifies the @bp_button macro.

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
@bp_button DualKey begin
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
    if (name != Symbol) && !Meta.isexpr(name, :curly)
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
          type_params::Optional{Vector{Symbol}}
          type_name_str::String
    if name isa Symbol
        short_name = name
        struct_name = name
        struct_def = esc(name)
        type_name_str = string(name)
        type_params = nothing
    elseif Meta.isexpr(name, :curly)
        short_name = name.args[1]
        struct_name = Symbol(:Button_, short_name)
        struct_def = deepcopy(name)
        struct_def.args[1] = struct_name
        type_name_str = string(short_name)
        type_params = name.args[2:end]
    else
        error("Unhandled case: ", name)
    end

    definitions = definition.args

    # Get the fields and constructors.
    definitions_split = map(definitions) do def
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
    end
    filter!(exists, definition_split)
    definitions_impl = map(definitions_split) do def_data
        # Is it a field?
        if def_data isa Tuple # Result of calling MacroTools.splitarg(def),
                              #    and wrapping default_value in Some()
            (field_name, field_type, is_slurped, some_default_value) = def_data
            @assert !is_slurped
            field_name = esc(field_name)
            field_type = esc(field_type)
            if exists(some_default_value)
                return :( $field_name::$field_type = $(esc(some(some_default_value))) )
            else
                return :( $field_name::$field_type )
            end
        # Is it a constructor?
        elseif def_data isa Dict # Result of calling MacroTools.splitdef(def).
            if def_data[:name] in (:RAW, :UPDATE) # Actually a function implementation,
                                                  #    skip it for now
                return :( )
            elseif def_data[:name] == short_name # A constructor
                # The user gave the short name of the struct.
                # Modify the function definition to support that.
                def_data[:name] = struct_name
                # Let the short name map to the long name.
                def_data[:body] = quote
                    let $(esc(short_name)) = $struct_name
                        $(esc(def_data[:body]))
                    end
                end
                return combinedef(def_data)
            else
                error("Unexpected function syntax in @bp_button: ", combinedef(def_data))
            end
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
    raw_decl[:name] = :button_value_raw
    if isempty(raw_decl[:args])
        push!(raw_decl[:args], :( _::$struct_def ))
    elseif !isa(raw_decl[:args][1], Symbol)
        error("The 'button' parameter for RAW() should be untyped!",
              " The type is added automatically")
    else
        raw_decl[:args][1] = :( $(raw_decl[:args][1])::$struct_def )
    end
    if length(raw_decl[:args] < 2)
        push!(raw_decl[:args], :( _::GLFW.Window ))
    elseif !isa(raw_decl[:args][2], Symbol)
        error("The 'window' parameter for RAW() should be untyped1",
              " The type is added automatically")
    else
        raw_decl[:args][2] = :( $(raw_decl[:args][2])::GLFW.Window )
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
        update_decl[:name] = :button_update
        # Add the first 'button' argument if not given.
        button_name = esc(:b)
        if isempty(update_decl[:args])
            push!(update_decl[:args], :( $button_name::$struct_def ))
        elseif !isa(update_decl[:args][1], Symbol)
            error("The 'button' parameter for UPDATE() should be untyped!",
                " The type is added automatically")
        else
            button_name = esc(update_decl[:args][1])
            update_decl[:args][1] = :( $button_name::$struct_def )
        end
        # Add the second 'window' argument if not given.
        window_name = esc(:wnd)
        if length(update_decl[:args] < 2)
            push!(update_decl[:args], :( $window_name::GLFW.Window ))
        elseif !isa(update_decl[:args][2], Symbol)
            error("The 'window' parameter for UPDATE() should be untyped1",
                " The type is added automatically")
        else
            window_name = esc(update_decl[:args][2])
            update_decl[:args][2] = :( $window_name::GLFW.Window )
        end
        # Invoke the normal update behavior first, and
        #     add 'UPDATE' as a real function for the body to reference.
        update_decl[:body] = quote
            invoke(button_update, Tuple{AbstractButton, GLFW.Window},
                   $button_name, $window_name)
            let $(esc(:UPDATE)) = (button::$struct_def=$button_name, window::GLFW.Window=$window_name) ->
                                      button_update(button, window)
                $(update_decl[:body])
            end
        end
        update_impl = combinedef(update_decl)
    end

    return quote
        Core.@__doc__ Base.@kwdef mutable struct $struct_def <: AbstractButton
            type::Symbol = Symbol($type_name_str)
            mode::ButtonModes = ButtonModes.is_on
            previous_raw::Bool = false
            current_raw::Bool = false

            $(definitions_impl...)
        end
        StructTypes.StructType(::Type{$struct_name}) = StructTypes.UnorderedStruct()
        BUTTON_SERIALIZATION_KEYS[] = (BUTTON_SERIALIZATION_KEYS[]...,
                                       $(esc(Symbol($type_name_str))) = $struct_name)
        $raw_impl
        $update_impl
    end
end
export bp_button


"
For some keys that appear on both sides of the keyboard,
    you can use a special alias that means 'either key'.
"
const InputKey = Union{GLFW.Key, @ano_enum(ALT, CTRL, SHIFT, OS)}
export InputKey
check_key(input::GLFW.Key, window::GLFW.Window) = GLFW.GetKey(window, input)
check_key(input::Val{:ALT}, window::GLFW.Window) = GLFW.GetKey(window, GLFW.KEY_LEFT_ALT) ||
                                                   GLFW.GetKey(window, GLFW.KEY_RIGHT_ALT)
check_key(input::Val{:CTRL}, window::GLFW.Window) = GLFW.GetKey(window, GLFW.KEY_LEFT_CONTROL) ||
                                                    GLFW.GetKey(window, GLFW.KEY_RIGHT_CONTROL)
check_key(input::Val{:SHIFT}, window::GLFW.Window) = GLFW.GetKey(window, GLFW.KEY_LEFT_SHIFT) ||
                                                     GLFW.GetKey(window, GLFW.KEY_RIGHT_SHIFT)
check_key(input::Val{:OS}, window::GLFW.Window) = GLFW.GetKey(window, GLFW.KEY_LEFT_SUPER) ||
                                                  GLFW.GetKey(window, GLFW.KEY_RIGHT_SUPER)


"
A button that's triggered by a keyboard key.
For some keys that appear on both sides of the keyboard,
    you can use a special alias that means 'either key'.
"
@bp_button Key begin
    key::InputKey
    RAW(b, wnd) = check_key(key, wnd)
end
"A button that's triggered by a mouse button"
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
"A button that's triggered if all given buttons are simultaneously on"
@bp_button All begin
    children::Vector{AbstractButton} = AbstractButton[ ]
    RAW(b) = all(button_value(c) for c in b.children)
    UPDATE(b) = for c in children
        UPDATE(c)
    end
end