# Test func_def_to_call():
for (input, expected_output) in [ (:( f() = 5 ), :( f() )),
                                  (:( f(a) = a+7 ), :( f(a) )),
                                  (:( f(a, b, c; d, e) = a+7 ), :( f(a, b, c; d, e) )),
                                  (:( f(a, b::Int, c=10, d::Int = 20
                                        ; e, f=30, g::Int, h::Int=40) = a+7 ),
                                     :( f(a, b, c, d; e, f, g, h) )),
                                  (:( f()::Int = 5 ), :( f() )),
                                  (:( f(a)::Int = a+7 ), :( f(a) )),
                                  (:( f(a, b, c; d, e)::Int = a+7 ), :( f(a, b, c; d, e) )),
                                  (:( f(a, b::Int, c=10, d::Int = 20
                                        ; e, f=30, g::Int, h::Int=40)::Int = a+7 ),
                                     :( f(a, b, c, d; e, f, g, h) )),
                                  (:( f(a...)::Int = a .+ 7 ), :( f(a...) )),
                                  (:( f(a, b, c...; d, e...)::Int = a+7 ), :( f(a, b, c...; d, e...) )),
                                  (:( f(a, b::Int, c=10, d::Int...
                                        ; e, f=30, g::Int, h::Int...)::Int = a+7 ),
                                     :( f(a, b, c, d...; e, f, g, h...) )),
                                ]
    local actual_output
    try
        actual_output = func_def_to_call(input)
    catch e
        error("ERROR while testing func_def_to_call(", input, "):\n\t",
              sprint(showerror, e, catch_backtrace()))
    end

    @bp_check(expected_output == actual_output,
              "Expected func_def_to_call(", input, ") to be:\n\t",
                expected_output, "\n\t",
                " but was: \n\t",
                actual_output)
end

# Test SplitDef:
for (input, expected) in [ (
                               :( f() = 5 ),
                               (:f, [ ], [ ], 5, nothing, (), nothing, false)
                           ),
                           (
                               :( abc(def, ghi::Int)::Float32 = def*ghi ),
                               (:abc, [ :def, :( ghi::Int ) ], [ ],
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
                                   :ghi, [ :( jkl::T ) ], [ :mnop, Expr(:kw, :qrs, 5) ],
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
                                   :ghi, [ :( jkl::T ) ], [ :mnop, Expr(:kw, :qrs, 5) ],
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
        @bp_check(f_actual == f_expected,
                  "SplitDef.", name, " should be ", f_expected, ", but was ", f_actual,
                  "; in function:\n", input, "\n\n")
    end
end