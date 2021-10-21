# Import dependencies.
using TupleTools, Setfield, StaticArrays, InteractiveUtils,
      ModernGL, GLFW

# Import the main codebase.
using Bplus
using Bplus.Utilities, Bplus.Math, Bplus.GL

# Enable all asserts for the codebase.
@inline Bplus.Math.bp_math_asserts_enabled() = true
@inline Bplus.GL.bp_gl_asserts_enabled() = true

# Ensure the @timed macro is precompiled by timing some arbitrary code.
_ = @timed sin(Threads.nthreads())


#############################
#          Helpers          #
#############################

"""
Tests that the given expression equals the given value,
   and does not allocate any heap memory when evaluating.
NOTE: the expression is evaluated twice, to ensure it's precompiled,
   so don't mutate any state!
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
            run_timer() = @timed($expr)
            # Try several times until we get a result which does't allocate,
            #    in case the GC does any weird stuff (and to handle the initial precompilation)
            result = nothing
            n_tries::Int = 0
            for i in 1:10
                result = run_timer()
                n_tries += 1
                if result.bytes < 1
                    break
                end
            end
            actual_value = result.value
            # Get the expected value, and repeat its operation the same number of times
            #    in case the expressions mutate something.
            expected_value = nothing
            for i in 1:n_tries
                expected_value = $expected_value
            end
            # Test.
            @bp_check(actual_value == expected_value, "\n",
                     "    Expected: `", $expected_str, "` => `", expected_value, "`.\n",
                     "      Actual: `", $expr_str,     "` => `", actual_value, "`.\n",
                     "\t", $(msg...))
            @bp_check(result.bytes == 0,
                     "The expression `", $expr_str,
                     "` allocated ", Base.format_bytes(result.bytes),
                     ". ", $(msg...))
        end
        run_test()
    end
end

const ALL_SIGNED = (Int8, Int16, Int32, Int64, Int128)
const ALL_UNSIGNED = map(unsigned, ALL_SIGNED)
const ALL_INTEGERS = TupleTools.vcat(ALL_SIGNED, ALL_UNSIGNED)

const ALL_FLOATS = (Float16, Float32, Float64)

const ALL_REALS = TupleTools.vcat(ALL_INTEGERS, ALL_FLOATS)

#############################


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

#TODO: Add tests for "toggleable assert" macros