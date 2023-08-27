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