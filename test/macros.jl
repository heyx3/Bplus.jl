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
                  "SplitDef.", name, " should be ", f_expected, ", but was ", f_actual,
                  "; in function:\n", input, "\n\n")
    end
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