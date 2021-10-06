# Import dependencies.
using TupleTools, Setfield, StaticArrays

# Import the main codebase.
using Bplus
using Bplus.Utilities, Bplus.Math

# Ensure the @timed macro is precompiled by timing some arbitrary code.
_ = @timed sin(Threads.nthreads())

# Define some helpers.
"""
Tests that the given expression equals the given value,
   and does not allocate any heap memory when evaluating.
"""
macro bp_test_no_allocations(expr, expected_value, msg...)
    expr_str = string(expr)
    expected_str = string(expected_value)
    expr = esc(expr)
    expected_value = esc(expected_value)
    msg = map(esc, msg)
    return quote
        # Hide the expressions in a function to avoid issues with memory allocations and globals.
        function run_test()
            gen_actual() = $expr
            run_timer() = @timed(gen_actual())
            # Pre-compile the expression by running it once.
            # Then run it for real, and profile with the @timed macro.
            try
                run_timer()
            catch e
                error("Error evaluating ", $expr_str, ":\n\t",
                         sprint(showerror, e, catch_backtrace()))
            end
            result = run_timer()
            actual_value = result.value
            expected_value = $expected_value
            # Test.
            @bp_check(actual_value == expected_value,
                     "Expected: `", $expr_str, "` == `", $expected_str, "`.\n",
                     "       => `", actual_value, "` == `", expected_value, "`.\n",
                     "\t", $(msg...))
            @bp_check(result.bytes == 0,
                     "The expression `", $expr_str,
                     "` allocated ", Base.format_bytes(result.bytes))
        end
        run_test()
    end
end

# Execute all Julia files in this folder, other than this one.
test_files = readdir(@__DIR__, join=true)
filter!(test_files) do name
    return !endswith(name, "runtests.jl") &&
           endswith(name, ".jl")
end
for f_path in test_files
    f_name = split(f_path, ('/', '\\'))[end]
    println("Running ", f_name, "...")
    include(f_name)
end

# If we haven't crashed yet, then I guess the tests worked.
println("All tests passed!")