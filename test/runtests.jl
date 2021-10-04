# Import the main codebase.
using Bplus
using Bplus.Utilities, Bplus.Math

# Ensure the @timed macro is precompiled by timing some arbitrary code.
_ = @timed sin(Threads.nthreads())

# Define some helpers.
"""
Tests that the given expression == the given value.
Also tests that executing the expression does not result in any heap allocations.
"""
macro bp_test_no_allocations(expr, expected_value, msg...)
    msg = map(esc, msg)
    return quote
        # Pre-compile the expression by running it once.
        $(esc(expr))
        # Run it, and time the result.
        actual_value = nothing
        result::NamedTuple = @timed(actual_value = $(esc(expr)))
        # Test.
        @bp_check(result.value == $(esc(expected_value)), $(msg...))
        @bp_check(result.bytes == 0,
                  "The expression '", $(string(expr)),
                  " allocated ", Base.format_bytes(result.bytes))
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
    include(f_path)
end