# Test SplitArg:
for (input, expected) in [ (
                                :( f(a, b=7, c::Int, d::Float64=0.1 )),
                                SplitArg.([
                                    :a,
                                    :( b=7 ),
                                    :( c::Int ),
                                    :( d::Float64=0.1 )
                                ])
                           ) ]
    input_args = input.args[2:end]
    if isexpr(input_args[1], :parameters)
        input_args = input_args[2:end]
    end

    actual = SplitArg.(input_args)
    @bp_check(map(e -> getfield.(Ref(e), fieldnames(SplitArg)), expected) ==
                map(a -> getfield.(Ref(a), fieldnames(SplitArg)), actual),
              "Expected ", input, " to have the args ", expected, ", but got ", actual)
end

# Test SplitDef:
for (input, expected) in [ (
                               :( f() = 5 ),
                               (:f, [ ], [ ], 5, nothing, (), nothing, false)
                           ),
                           (
                               :( abc(def, ghi::Int)::Float32 = def*ghi ),
                               (:abc, SplitArg.([ :def, :( ghi::Int ) ]), [ ],
                                :( def * ghi ), :Float32,
                                (), nothing, false)
                           ),
                           (
                               ( quote
                                   function ghi(jkl::T; mnop, qrs=5) where {T}
                                       return jkl
                                   end
                               end ).args[2], # Get the actual :macrocall expr
                               (
                                   :ghi,
                                   SplitArg.([ :( jkl::T ) ]),
                                   SplitArg.([ :mnop, Expr(:kw, :qrs, 5) ]),
                                   :( return jkl ),
                                   nothing,
                                   (:T, ),
                                   nothing, false
                               )
                           ),
                           (
                               ( quote
                                   "ABCdef"
                                   @inline function ghi(jkl::T; mnop, qrs=5) where {T}
                                       return jkl
                                   end
                               end ).args[2], # Get the actual :macrocall expr
                               (
                                   :ghi,
                                   SplitArg.([ :( jkl::T ) ]),
                                   SplitArg.([ :mnop, Expr(:kw, :qrs, 5) ]),
                                   :( return jkl ),
                                   nothing,
                                   (:T, ),
                                   "ABCdef",
                                   true
                               )
                           ),
                           (
                               :( abc(def, ghi::Int)::Float32 ),
                               (:abc, SplitArg.([ :def, :( ghi::Int ) ]), [ ],
                                nothing, :Float32,
                                (), nothing, false)
                           )
                         ]
    input = rmlines(input)
    actual = SplitDef(input)
    field_names = fieldnames(SplitDef)
    actual_fields = getproperty.(Ref(actual), field_names)
    for (name, f_actual, f_expected) in zip(field_names, actual_fields, expected)
        f_actual = prettify(f_actual)
        # SplitArg requires a little extra effort to compare.
        if name in (:args, :kw_args)
            f_actual = map(a -> getfield.(Ref(a), fieldnames(SplitArg)), f_actual)
            f_expected = map(e -> getfield.(Ref(e), fieldnames(SplitArg)), f_expected)
        end
        @bp_check(f_actual == f_expected,
                  "SplitDef.", name, " should be (", typeof(f_expected), ") ", f_expected, ", but was (",
                  typeof(f_actual), ") ", f_actual,
                  "; in function:\n", input, "\n\n")
    end
end

# Test SplitMacro:
@bp_check(isnothing(SplitMacro(:( f() = 5 ))))
let sm = SplitMacro(:( @a(b, c) ))
    @bp_check(exists(sm))
    @bp_check(sm.name == Symbol("@a"), sm)
    @bp_check(sm.args == [ :b, :c ], sm)
    @bp_check(string(combinemacro(sm)) ==
                "$(sm.source) @a b c",
              "Got: ", string(combinemacro(sm)))
end


# Test combinecall():
for (input, expected) in [ (:( f(a) ), :( f(a) )),
                           (
                                :( f(a, b::Int, c=:c, d::Float64=10.5) ),
                                :( f(a, b, c, d) )
                           ),
                           (
                                :( f(; a, b::Int, c=3, d::UInt8=4) ),
                                :( f(; a=a, b=b, c=c, d=d) )
                           ),
                           (
                                :( f(a, b::Int, c=3, d::UInt8=4
                                     ;
                                     e, f::String, g=:hi, h::UInt=42) ),
                                :( f(a, b, c, d; e=e, f=f, g=g, h=h) )
                           )
                         ]
    split = SplitDef(:( $input = nothing ))
    actual = combinecall(split)
    # It's actually hard to compare two AST's, as far as I know.
    @bp_check(string(expected) == string(actual),
              "Expected ", input, " to convert to ",
                string(expected), ", but got ", string(actual))
end

# Test is_function_decl():
run_test(expr, expected=true) = @bp_check(is_function_decl(expr) == expected,
                                          expected ? "Valid" : "Invalid",
                                            " function declaration not caught by is_function_decl(): ",
                                            expr)
run_tests_1(header, valid_header=true) = begin # Test function call and then function definition
    run_test(header, false)
    run_test(:( $header = nothing ), valid_header)
end
run_tests_2(signature, valid_signature=true) = begin # Test with/without return values and type params
    run_tests_1(:( $signature ), valid_signature)
    run_tests_1(:( $signature::Int ), valid_signature)
    run_tests_1(:( $signature::T where {T} ), valid_signature)
    run_tests_1(:( $signature::T where {T<:AbstractArmAndALeg} ), valid_signature)
    run_tests_1(:( $signature where {T} ), valid_signature)
    run_tests_1(:( $signature where {T<:HelloWorld} ), valid_signature)
end
run_tests_3(name, valid_name=true) = begin # Test with various parameter combinations
    run_tests_2(:( $name() ), valid_name)
    run_tests_2(:( $name(i) ), valid_name)
    run_tests_2(:( $name(i=6) ), valid_name)
    run_tests_2(:( $name(i::Int = 6) ), valid_name)
    run_tests_2(:( $name(; j) ), valid_name)
    run_tests_2(:( $name(; j=7) ), valid_name)
    run_tests_2(:( $name(; j::Int) ), valid_name)
    run_tests_2(:( $name(; j::Int = 7) ), valid_name)
    run_tests_2(:( $name(i; j) ), valid_name)
    run_tests_2(:( $name(i; j=7) ), valid_name)
    run_tests_2(:( $name(i; j::Int) ), valid_name)
    run_tests_2(:( $name(i; j::Int = 7) ), valid_name)
    run_tests_2(:( $name(i=6; j) ), valid_name)
    run_tests_2(:( $name(i=6; j=7) ), valid_name)
    run_tests_2(:( $name(i=6; j::Int) ), valid_name)
    run_tests_2(:( $name(i=6; j::Int = 7) ), valid_name)
    run_tests_2(:( $name(i::Int; j) ), valid_name)
    run_tests_2(:( $name(i::Int; j=7) ), valid_name)
    run_tests_2(:( $name(i::Int; j::Int) ), valid_name)
    run_tests_2(:( $name(i::Int; j::Int = 7) ), valid_name)
    run_tests_2(:( $name(i::Int = 6; j) ), valid_name)
    run_tests_2(:( $name(i::Int = 6; j=7) ), valid_name)
    run_tests_2(:( $name(i::Int = 6; j::Int) ), valid_name)
    run_tests_2(:( $name(i::Int = 6; j::Int = 7) ), valid_name)
end
run_tests_3(:f)
run_tests_3(:(Base.f))
run_tests_3(:(Base.f))
run_tests_3(:(a + b), false)
run_tests_3(:(a()), false)