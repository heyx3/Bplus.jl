"Holds a tuple of arguments, and represents a specific math expression"
abstract type AbstractMathField{NIn, NOut, F} <: AbstractField{NIn, NOut, F} end


##################
#  Helper macro  #
##################

"""
Short-hand for generating an `AbstractMathField`.
Sample syntax:

````
@make_math_field Add "+" begin
    INPUT_COUNT = 2
    value = input_values[1] + input_values[2]
    gradient = input_gradients[1] + input_gradients[2]
end
````

In the above example, the new struct is named `AddField`, and the corresponding DSL function/operator is `+`.

You construct an instance of `AddField` by providing it with the input fields (and without any type parameters).
Input fields with 1D outputs will be automatically promoted to the dimensionality of the other fields' outputs.

The full set of definitions you can provide is as follows:
* **(Required)** `INPUT_COUNT = [value]` defines how many inputs the node can have. Valid formats:
    * A constant: `INPUT_COUNT = 3`
    * A range: `INPUT_COUNT = 2:2:6`
    * A min bound: `INPUT_COUNT = 3:∞` (using the char *\\infty*)
    * An explicit set: `INPUT_COUNT = { 0, 2, 3 }`
* **(Required)** `value = [expr]` computes the value of the field, given the following parameters:
    * `field`: your field instance.
    * `NIn`, `NOut`, `F`, `TFields<:Tuple` : The type parameters for your field,
        including the tuple of its inputs (all of which have the same `NIn`, `NOut`, and `F` types).
    * `pos::Vec{NIn, F}`: the position within the field being sampled.
    * `input_values::NTuple{_, Vec{NOut, F}}` : the value of each input field at this position.
        * If you don't need this, you can improve performance by disabling it with `VALUE_CALC_ALL_INPUT_VALUES` (see below).
    * `prep_data::Tuple` : the output of `prepare_field` for each input field.
* `gradient = [expr]` computes the gradient of the field, given the following parameters:
    * `field`: your field instance
    * `NIn`, `NOut`, `F`, `TFields<:Tuple` : The type parameters for your field,
        including the tuple of its inputs (all of which have the same `NIn`, `NOut`, and `F` types).
    * `pos::Vec{NIn, F}`: the position input into the field.
    * `input_gradients::NTuple{_, Vec{NIn, Vec{NOut, F}}}` : the gradient of each input field at this position.
        * If you don't need this, you can improve performance by disabling it with `GRADIENT_CALC_ALL_INPUT_GRADIENTS` (see below).
    * `input_values::NTuple{_, Vec{NOut, F}}` : the value of each input field at this position.
        * **Not provided** by default; you must enable it with `GRADIENT_CALC_ALL_INPUT_VALUES` (see below).
    * `prep_data::Tuple` : the output of `prepare_field` for each input field.
    If not given, falls back to the default behavior of all fields (numerical solution).
* `VALUE_CALC_ALL_INPUT_VALUES = [true|false]`. If true (the default value),
    then the computation of `value` has access to a local var, `input_values`,
    containing all input fields' values. This can be disabled for performance if your math op
    doesn't always need every input's value.
* `GRADIENT_CALC_ALL_INPUT_VALUES = [true|false]`. If true (default value is false),
    then the computation of `gradient` has access to a local var, `input_values`,
    containing all input fields' values. This is disabled by default for performance;
    enable if your math op needs every input's value to compute its own gradient.
* `GRADIENT_CALC_ALL_INPUT_GRADIENTS = [true|false]`. If true (the default value),
    then the computation of `gradient` has access to a local var, `input_gradients`,
    containing all input fields' gradients. This can be disabled for performance if your math op
    doesn't always need every input's gradient.
"""
macro make_math_field(name, dsl_func_name, defs_expr)
    # Do error-checking.
    if !isa(name, Symbol)
        error("First argument to @make_math_field should be a name, like 'Add'")
    elseif !isa(dsl_func_name, String)
        error("Second argument to @make_math_field should be a string literal")
    elseif !Meta.isexpr(defs_expr, :block)
        error("Third argument to @make_math_field should be a begin ... end block of definitions")
    end

    # Generate names.
    struct_name = Symbol(name, :Field)
    struct_name_esc = esc(struct_name)
    NIn = esc(:NIn)
    NOut = esc(:NOut)
    F = esc(:F)
    TInputs = esc(:TInputs)
    field_inputs = :inputs
    local_field = esc(:field)
    local_pos = esc(:pos)
    local_prep_data = esc(:prep_data)
    local_input_values = esc(:input_values)
    local_input_gradients = esc(:input_gradients)

    # Pull out the individual definitions.
    defs_list = filter(def -> !isa(def, LineNumberNode), defs_expr.args)
    defs = Dict{Symbol, Any}()
    for def in defs_list
        @assert(Meta.isexpr(def, :(=)),
                "Each definition should be of the form '[name] = [value]'. Invalid definition \"$def\"")
        (name, value) = def.args
        @assert(!haskey(defs, name), "Definition for '$name' exists more than once")
        defs[name] = value
    end

    # Process input information.
    input_is_valid_expr = :( true )
    if haskey(defs, :INPUT_COUNTS)
        input_expr = defs[:INPUT_COUNTS]
        # A constant?
        if input_expr isa Integer
            input_is_valid_expr = :( n_inputs == $input_expr )
        # A range?
        elseif Meta.isexpr(input_expr, :call) && input_expr.args[1] == :(:)
            # Support an "infinite" upper-bound.
            if input_expr.args[end] == :∞
                input_expr.args[end] = typemax(Int)
            end
            input_is_valid_expr = :( n_inputs in $input_expr )
        # An explicit set? E.x. `{ 1, 2, 4, 6 }`
        elseif Meta.isexpr(input_expr, :braces)
            input_is_valid_expr = :( n_inputs in tuple($(input_expr.args...)) )
        else
            error("Unexpected kind of INPUT_COUNTS: '", input_expr, "'")
        end
    end

    # Process output information.
    local value_computation, gradient_computation
    if haskey(defs, :value)
        value_computation = esc(defs[:value])
    else
        error("Definition of 'value' is required but not given",
              " (e.x. 'value = reduce(+, input_values)')")
    end
    gradient_computation = if haskey(defs, :gradient)
                               esc(defs[:gradient])
                           else
                               nothing
                           end

    # Generate data for value/derivative computation.
    locals_for_value = [ ]
    if get(defs, :VALUE_CALC_ALL_INPUT_VALUES, true)
        push!(locals_for_value,
              :( $local_input_values = math_field_input_values($local_field, $local_pos, $local_prep_data) ))
    end
    locals_for_gradient = [ ]
    if get(defs, :GRADIENT_CALC_ALL_INPUT_VALUES, false)
        push!(locals_for_gradient,
              :( $local_input_values = math_field_input_values($local_field, $local_pos, $local_prep_data) ))
    end
    if get(defs, :GRADIENT_CALC_ALL_INPUT_GRADIENTS, true)
        push!(locals_for_gradient,
              :( $local_input_gradients = math_field_input_gradients($local_field, $local_pos, $local_prep_data) ))
    end

    # Generate the final code.
    return quote
        Core.@__doc__ struct $struct_name{$NIn, $NOut, $F, $TInputs <: Tuple} <: AbstractMathField{$NIn, $NOut, $F}
            $field_inputs::$TInputs
            function $struct_name_esc($field_inputs::Union{AbstractField{$NIn, NOut_, $F},
                                                           AbstractField{$NIn, 1, $F}}...
                                     ) where {$NIn, NOut_, $F}
                # Type inference needs some help if all fields are 1D.
                $NOut = if all(f -> field_output_size(f) == 1, $field_inputs)
                            1
                        else
                            NOut_
                        end

                n_inputs::Int = length($field_inputs)
                if !($input_is_valid_expr)
                    error("Invalid number of inputs into '", $dsl_func_name, "', ",
                          $name, ". Constraints: ", $(string(input_is_valid_expr)))
                end

                processed_inputs = process_inputs(Val($NIn), Val($NOut), $F, $field_inputs)
                return new{$NIn, $NOut, $F, typeof(processed_inputs)}(processed_inputs)
            end
        end
        export $struct_name

        function $(esc(:get_field))( $local_field::$struct_name_esc{$NIn, $NOut, $F, $TInputs},
                                     $local_pos::Vec{$NIn, $F},
                                     $local_prep_data::Tuple
                                   ) where {$NIn, $NOut, $F, $TInputs}
            $(locals_for_value...)
            return $value_computation
        end
        $(
            if isnothing(gradient_computation)
                :( )
            else; :(
                function $(esc(:get_field_gradient))( $local_field::$struct_name_esc{$NIn, $NOut, $F, $TInputs},
                                                      $local_pos::Vec{$NIn, $F},
                                                      $local_prep_data::Tuple
                                                    ) where {$NIn, $NOut, $F, $TInputs}
                    $(locals_for_gradient...)
                    return $gradient_computation
                end
            )
            end
        )

        function $(esc(:field_from_dsl_func))( ::Val{Symbol($dsl_func_name)},
                                               context::DslContext, state::DslState,
                                               args::Tuple
                                             )
            $struct_name(field_from_dsl.(args, Ref(context), Ref(state))...)
        end
        function $(esc(:dsl_from_field))(field::$struct_name_esc)
            return :( $(Symbol($dsl_func_name))($(dsl_from_field.(field.$field_inputs)...)) )
        end
    end
end

"
Pre-processes the inputs of a field, so that 1D outputs are stretched
    to match the dimensions of the other outputs.
"
@generated function process_inputs( ::Val{NIn}, ::Val{NOut}, ::Type{F},
                                    inputs::TFields
                                  )::NTuple where {NIn, NOut, F, TFields<:Tuple}
    # If the output is 1-dimensional, then no conversion is needed.
    if NOut < 2
        return quote
            @assert(NOut == 1, "Invalid value for NOut: '$NOut'")

            # Assert each parameter is a field with 1D output.
            $(map(1:length(TFields.parameters)) do i::Int
                :(
                    @assert(inputs[$i] isa AbstractField{NIn, NOut, F},
                            "Field input $($i) isn't 1D: $(typeof(inputs[$i]))")
                )
            end...)

            return inputs
        end
    end

    # For 2+ dimensional outputs, find 1D inputs and promote them.
    output_elements = [ ]
    for input_idx in 1:length(TFields.parameters)
        if TFields.parameters[input_idx] <: AbstractField{NIn, 1, F}
            swizzle_str = repeat('x', NOut)
            swizzle = QuoteNode(Symbol(swizzle_str))
            push!(output_elements, :(
                SwizzleField(inputs[$input_idx], $swizzle)
            ))
        elseif TFields.parameters[input_idx] <: AbstractField{NIn, NOut, F}
            push!(output_elements, :( inputs[$input_idx] ))
        else
            push!(output_elements, :(
                error("Input $($input_idx) is the wrong type: ",
                      typeof(inputs[$input_idx]))
            ))
        end
    end
    return quote
        return tuple($(output_elements...))
    end
end

@generated function math_field_input_values( m::TField,
                                             pos::Vec{NIn, F},
                                             prepared_data
                                           )::ConstVector{Vec{NIn, F}} where {NIn, NOut, F, TField<:AbstractMathField{NIn, NOut, F}}
    inputs_tuple_type = fieldtype(TField, :inputs)
    inputs_types = inputs_tuple_type.parameters
    output_values = map(1:length(inputs_types)) do i::Int
        return :( get_field(m.inputs[$i], pos, prepared_data[$i]) )
    end
    return :( tuple($(output_values...)) )
end
@generated function math_field_input_gradients( m::TField,
                                                pos::Vec{NIn, F},
                                                prepared_data
                                              )::Tuple where {NIn, NOut, F, TField<:AbstractMathField{NIn, NOut, F}}
    inputs_tuple_type = fieldtype(TField, :inputs)
    inputs_types = inputs_tuple_type.parameters
    output_gradients = map(1:length(inputs_types)) do i::Int
        return :( get_field_gradient(m.inputs[$i], pos, prepared_data[$i]) )
    end
    return :( tuple($(output_gradients...)) )
end

@generated function prepare_field(m::TField)::Tuple where {NIn, NOut, F, TField<:AbstractMathField{NIn, NOut, F}}
    inputs_tuple_type = fieldtype(TField, :inputs)
    inputs_types = inputs_tuple_type.parameters
    output_elements = map(1:length(inputs_types)) do i::Int
        return :( prepare_field(m.inputs[$i]) )
    end
    return :( tuple($(output_elements...)) )
end


####################
#  Concrete types  #
####################

# Simple math ops
@make_math_field Add "+" begin
    INPUT_COUNTS = 2:∞
    value = reduce(+, input_values)
    gradient = reduce(+, input_gradients)
end
@make_math_field Subtract "-" begin # Also defines negation
    INPUT_COUNTS = 1:∞
    value = if length(input_values) > 1
                foldl(-, input_values)
            else
                -input_values[1]
            end
    gradient = if length(input_gradients) > 1
                   foldl(-, input_gradients)
               else
                   -input_gradients[1]
               end
end
@make_math_field Multiply "*" begin
    INPUT_COUNTS = 2:∞

    value = reduce(*, input_values)

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = begin
        # Apply the "product rule": d/dx f(x)g(x) = f'(x)g(x) + f(x)g'(x)
        current_value::Vec{NOut, F} = input_values[1]
        current_gradient::Vec{NIn, Vec{NOut, F}} = input_gradients[1]
        for i::Int in 2:length(field.inputs)
            current_gradient = (current_gradient * input_values[i]) +
                               (current_value * input_gradients[i])
            current_value += input_values[i]
        end
        return current_gradient
    end
end
@make_math_field Divide "/" begin
    INPUT_COUNTS = 2

    value = input_values[1] / input_values[2]

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = begin
        (numerator_value, denominator_value) = input_values
        (numerator_gradient, denominator_gradient) = input_gradients
        # Apply the "division rule": d/dx f(x)/g(x) = [f'(x)g(x) - f(x)g'(x)] / [g(x)g(x)]
        return ((numerator_gradient * denominator_value) - (numerator_value * denominator_gradient)) /
               (denominator_value * denominator_value)
    end
end
"`pow(x, y) = x^y`"
@make_math_field Pow "pow" begin
    INPUT_COUNT = 2
    value = input_values[1] ^ input_values[2]

    # Turns out, the derivative of u(x)^v(x) is a bit complicated.
    # Here's a reference: https://math.stackexchange.com/questions/3353508/what-is-the-derivative-of-a-function-of-the-form-uxvx
    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = let (a,b) = input_values,
                   (da, db) = input_gradients
        (a^b) * ((b * (da / a)) + (db * map(log, a)))
    end
end
@make_math_field Sqrt "sqrt" begin
    INPUT_COUNT = 1
    value = map(sqrt, input_values[1])
    # d/dx sqrt(x) = 1/(2 * sqrt(x))
    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = (convert(F, 0.5) / sqrt(input_values[1])) * input_gradients[1]
end

# Trig functions
@make_math_field Sin "sin" begin
    INPUT_COUNTS = 1
    value = map(sin, input_values[1])

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = input_gradients[1] * map(cos, input_values[1])
end
@make_math_field Cos "cos" begin
    INPUT_COUNTS = 1
    value = map(cos, input_values[1])

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = input_gradients[1] * map(sin, -input_values[1])
end
"Tangent trig function"
@make_math_field Tan "tan" begin
    INPUT_COUNTS = 1
    value = map(tan, input_values[1])

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = input_gradients[1] * square(map(sec, input_values[1]))
end

# Numeric stuff
"
Floating-point modulo: `mod(a, b) == a % b`.
The behavior with negative numbers matches Julia's `%` operator.
"
@make_math_field Mod "mod" begin
    INPUT_COUNT = 2
    value = input_values[1] % input_values[2]
    # Gradient is very tricky to work out due to the denominator continuously changing.
end
"Rounds values down to the nearest integer."
@make_math_field Floor "floor" begin
    INPUT_COUNT = 1
    value = map(floor, input_values[1])
    # Gradient is zero almost everywhere. In the moment of transition between values, it's undefined.
    # However, replacing this behavior with finite differences results in a less degenerative,
    #    more aesthetically-useful value.
end
"Rounds values up to the nearest integer."
@make_math_field Ceil "ceil" begin
    INPUT_COUNT = 1
    value = map(ceil, input_values[1])
    # Gradient is zero almost everywhere. In the moment of transition between values, it's undefined.
    # However, replacing this behavior with finite differences results in a less degenerative,
    #    more aesthetically-useful value.
end
@make_math_field Abs "abs" begin
    INPUT_COUNT = 1
    value = map(abs, input_values[1])
    # If the value is negative, then the gradient will be flipped.
    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = Vec((
        Vec((
            let v = input_values[1][component],
                g = input_gradients[1][axis][component]
              (v < zero(F)) ? -g : g
            end for component in 1:NOut
        )...)
          for axis in 1:NIn
    )...)
end
"`clamp(x, min=0, max=1)`"
@make_math_field Clamp "clamp" begin
    INPUT_COUNT = { 1, 3 }
    value = if length(field.inputs) == 1
                clamp(input_values[1],
                      zero(typeof(input_values[1])),
                      one(typeof(input_values[1])))
            elseif length(field.inputs) == 3
                clamp(input_values...)
            else
                error("Unhandled case: ", length(field.inputs))
            end
    # Gradient is unchanged within the clamp range, and 0 outside it.
    GRADIENT_CALC_ALL_INPUT_VALUES = true
    GRADIENT_CALC_ALL_INPUT_GRADIENTS = false
    gradient = let is_in_range::VecB = (input_values[1] >= input_values[2]) &&
                                       (input_values[1] <= input_values[3])
        vselect(zero(GradientType(field)),
                get_field_gradient(field.inputs[1], pos),
                is_in_range)
    end
end
"`min(a, b)`"
@make_math_field Min "min" begin
    INPUT_COUNT = 2:∞
    value = min(input_values...)
    # Choose the gradient of the current minimum field.
    # The gradient at the points where it switches is undefined,
    #    so just ignore that case.
    GRADIENT_CALC_ALL_INPUT_VALUES = true
    GRADIENT_CALC_ALL_INPUT_GRADIENTS = true
    gradient = begin
        field_idcs_per_out_axis::NTuple{NOut, Int} = ntuple(Val(NOut)) do i_out
            input_value_channels = map(v -> v[i_out], input_values)
            return findmin(input_value_channels)[2]
        end
        gradients_per_in_axis::NTuple{NIn, NTuple{NOut, F}} = ntuple(Val(NIn)) do i_in
            return ntuple(Val(NOut)) do i_out
                field_idx = field_idcs_per_out_axis[i_out]
                input_gradients[field_idx][i_in]
            end
        end
        return GradientType(field)((Vec{NOut, F}(v...) for v in gradients_per_in_axis)...)
    end
end
"`max(a, b)`"
@make_math_field Max "max" begin
    INPUT_COUNT = 2:∞
    value = max(input_values...)
    # Choose the gradient of the current maximum field.
    # The gradient at the points where it switches is undefined,
    #    so just ignore that case.
    GRADIENT_CALC_ALL_INPUT_VALUES = true
    GRADIENT_CALC_ALL_INPUT_GRADIENTS = true
    gradient = begin
        field_idcs_per_out_axis::NTuple{NOut, Int} = ntuple(Val(NOut)) do i_out
            input_value_channels = map(v -> v[i_out], input_values)
            return findmax(input_value_channels)[2]
        end
        gradients_per_in_axis::NTuple{NIn, NTuple{NOut, F}} = ntuple(Val(NIn)) do i_in
            return ntuple(Val(NOut)) do i_out
                field_idx = field_idcs_per_out_axis[i_out]
                input_gradients[field_idx][i_in]
            end
        end
        return GradientType(field)((Vec{NOut, F}(v...) for v in gradients_per_in_axis)...)
    end
end

# Interpolation
"`step(a, t)` outputs 1 if `t >= a`, or `0` otherwise."
@make_math_field Step "step" begin
    INPUT_COUNT = 2
    value = vselect(zero(Vec{NOut, F}), one(Vec{NOut, F}),
                    input_values[2] >= input_values[1])
    # Gradient is zero almost everywhere. In the moment of transition between values, it's undefined.
    # However, using finite differences results in a more intuitive and useful gradient.
end
@make_math_field Lerp "lerp" begin
    INPUT_COUNT = 3
    value = input_values[1] + (input_values[3] * (input_values[2] - input_values[1]))

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = +(
        input_gradients[1],
        input_values[3] * (input_gradients[2] - input_gradients[1]),
        input_gradients[3] * (input_values[2] - input_values[1])
    )
end
@make_math_field Smoothstep "smoothstep" begin
    INPUT_COUNT = 1
    value = let t = clamp(input_values[1], convert(F, 0), convert(F, 1))
        t * t * (convert(F, 3) + (convert(F, -2) * t))
    end

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = let t = input_values[1],
                   tg = input_gradients[1]
        raw_gradient = (convert(F, 6) * t) * tg *
                       (convert(F, 1) * (convert(F, -6) * t))
        is_outside_range::Vec{NIn, Vec{NOut, Bool}} = (t < 0) | (t > 1)
        return vselect(raw_gradient, zero(GradientType(field)), is_outside_range)
    end
end
@make_math_field Smootherstep "smootherstep" begin
    INPUT_COUNT = 1
    value = let t = clamp(input_values[1], convert(F, 0), convert(F, 1))
        t * t * t * (convert(F, 10) + (t * (convert(F, -15) + (t * convert(F, 6)))))
    end

    GRADIENT_CALC_ALL_INPUT_VALUES = true
    gradient = let t = input_values[1],
                   tg = input_gradients[1]
        raw_gradient = (t * t) * tg *
                       (convert(F, 30) + (t * (convert(F, -60) + (convert(F, 30) * t))))
        is_outside_range::Vec{NIn, Vec{NOut, Bool}} = (t < 0) | (t > 1)
        return vselect(raw_gradient, zero(GradientType(field)), is_outside_range)
    end
end