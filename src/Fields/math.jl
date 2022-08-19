"Holds a tuple of arguments, and represents a specific math expression"
abstract type AbstractMathField{NIn, NOut, F} <: AbstractField{NIn, NOut, F} end

macro make_math_field(name, defs_expr)
    # Do error-checking.
    if !isa(name, Symbol)
        error("First argument to @make_math_field should be a name, like 'Add'")
    elseif !Meta.isexpr(defs_expr, :block)
        error("Second argument should be a begin ... end block of definitions")
    end

    # Generate names.
    struct_name = Symbol(name, :Field)
    struct_name_esc = esc(struct_name)

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
        value_computation = defs[:value]
    else
        error("Definition of 'value' is required but not given",
              " (e.x. 'value = reduce(+, input_values)')")
    end
    if haskey(defs, :gradient)
        gradient_computation = defs[:gradient]
    else
        error("Definition of 'gradient' is required but not given",
              " (e.x. 'gradient = reduce(+, input_gradients)')")
    end

    # Generate data for value/derivative computation.
    locals_for_value = [ ]
    if get(defs, :VALUE_CALC_ALL_INPUT_VALUES, true)
        push!(locals_for_value,
              :( input_values = math_field_input_values(field, pos, prep_data) ))
    end
    locals_for_gradient = [ ]
    if get(defs, :GRADIENT_CALC_ALL_INPUT_VALUES, false)
        push!(locals_for_gradient,
              :( input_values = math_field_input_values(field, pos, prep_data) ))
    end
    if get(defs, :GRADIENT_CALC_ALL_INPUT_GRADIENTS, true)
        push!(locals_for_gradient,
              :( input_gradients = math_field_input_gradients(field, pos, prep_data) ))
    end

    # Generate the final code.
    return quote
        struct $struct_name{NIn, NOut, F, TInputs <: Tuple} <: AbstractMathField{NIn, NOut, F}
            inputs::TInputs
            function $struct_name_esc(inputs::Union{AbstractField{NIn, NOut, F},
                                                    AbstractField{NIn, 1, F}}...
                                     ) where {NIn, NOut, F}
                n_inputs::Int = length(inputs)
                if !($input_is_valid_expr)
                    error("Invalid number of inputs into '", $name, "' field: ", n_inputs,
                          ". Constraints: ", $(string(input_is_valid_expr)))
                end

                processed_inputs = process_inputs(Val(NIn), Val(NOut), F, inputs)
                return new{NIn, NOut, F, typeof(inputs)}(processed_inputs)
            end
        end
        function $(esc(:get_field))( field::$struct_name_esc{NIn, NOut, F, TInputs},
                                     pos::Vec{NIn, F},
                                     prep_data::Tuple = ntuple(i->nothing, Val(length(TInputs.parameters)))
                                   ) where {NIn, NOut, F, TInputs}
            $(locals_for_value...)
            return $value_computation
        end
        function $(esc(:get_field_gradient))( field::$struct_name_esc{NIn, NOut, F, TInputs},
                                              pos::Vec{NIn, F},
                                              prep_data::Tuple = ntuple(i->nothing, Val(length(TInputs.parameters)))
                                            ) where {NIn, NOut, F, TInputs}
            $(locals_for_gradient...)
            return $gradient_computation
        end
        export $struct_name
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
                SwizzleOutputField(inputs[$input_idx], $swizzle)
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

@generated function prepare_field( m::TField,
                                   grid_size::Vec{NIn, Int}
                                 )::Tuple where {NIn, NOut, F, TField<:AbstractMathField{NIn, NOut, F}}
    inputs_tuple_type = fieldtype(TField, :inputs)
    inputs_types = inputs_tuple_type.parameters
    output_elements = map(1:length(inputs_types)) do i::Int
        return :( prepare_field(m.inputs[$i], grid_size) )
    end
    return :( tuple($(output_elements...)) )
end


@make_math_field Add begin
    INPUT_COUNTS = 2:∞
    value = reduce(+, input_values)
    gradient = reduce(+, input_gradients)
end
@make_math_field Subtract begin
    INPUT_COUNTS = 2:∞
    value = foldl(-, input_values)
    gradient = foldl(-, input_gradients)
end

@make_math_field Multiply begin
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
@make_math_field Divide begin
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

#TODO: Everything else