# Before running this file, if you define the global TEST_NAME, it will only test that file.
# Otherwise, it'll run every file in this folder.
# E.x. to only run tests for Vec (from vec.jl), you can set `TEST_NAME = "vec"`.

# Each test is siloed into its own module to avoid name collisions.
const TESTS_DEPENDENCIES = quote
    # External dependencies:
    using Random, TupleTools, MacroTools, Setfield, InteractiveUtils,
        StaticArrays, StructTypes, JSON3,
        DataStructures, Suppressor,
        ModernGL, GLFW, CImGui

    # The main codebase:
    using Bplus
    using Bplus.Utilities, Bplus.Math, Bplus.GL,
        Bplus.Helpers, Bplus.Fields, Bplus.SceneTree, Bplus.Input, Bplus.GUI

    # Unambiguate the ⋅ operator, between Images and Bplus.Math.
    const ⋅ = vdot
end
eval(TESTS_DEPENDENCIES)

# Enable all asserts for the codebase.
@inline Bplus.Utilities.bp_utils_asserts_enabled() = true
@inline Bplus.Math.bp_math_asserts_enabled() = true
@inline Bplus.GL.bp_gl_asserts_enabled() = true
@inline Bplus.Fields.bp_fields_asserts_enabled() = true
@inline Bplus.Input.bp_input_asserts_enabled() = true
@inline Bplus.Helpers.bp_helpers_asserts_enabled() = true
@inline Bplus.SceneTree.bp_scene_tree_asserts_enabled() = true
@inline Bplus.GUI.bp_gui_asserts_enabled() = true


#############################
#          Helpers          #
#############################

println("#TODO: Look at existing attempts to test memory allocations in Julia: https://github.com/JuliaObjects/ConstructionBase.jl/blob/master/test/runtests.jl#L349-L355")

"""
Tests that the given expression equals the given value,
   and does not allocate any heap memory when evaluating.
NOTE: the expression could be evaluated more than once, to ensure it's precompiled,
   so be careful with mutating expressions!
"""
macro bp_test_no_allocations(expr, expected_value, msg...)
    return impl_bp_test_no_allocations(expr, expected_value, msg, :())
end
"""
Tests that the given expression equals the given value,
   and does not allocate any heap memory when evaluating.
Also takes some code which does initial setup, and may allocate.
NOTE: the expression could be evaluated more than once, to ensure it's precompiled,
   so be careful with mutating expressions!
"""
macro bp_test_no_allocations_setup(setup, expr, expected_value, msg...)
    return impl_bp_test_no_allocations(expr, expected_value, msg, setup)
end

function impl_bp_test_no_allocations(expr, expected_value, msg, startup)
    expr_str = string(prettify(expr))
    expected_str = string(prettify(expected_value))
    expr = esc(expr)
    expected_value = esc(expected_value)
    msg = map(esc, msg)
    startup = esc(startup)
    return quote
        # Hide the expressions in a function to avoid global scope.
        @noinline function run_test()
            $startup
            # Try several times until we get a result which does't allocate,
            #    to handle precompilation *and* the GC doing anything weird.
            result = nothing
            n_tries::Int = 0
            for i in 1:10
                result = @timed($expr)
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
            @bp_check(true || result.bytes == 0,
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

const ALL_REALS = TupleTools.vcat(ALL_INTEGERS, ALL_FLOATS, (Bool, ))


# Import the above definitions into each test module.
push!(TESTS_DEPENDENCIES.args, quote
    import ..@bp_test_no_allocations, ..@bp_test_no_allocations_setup,
           ..ALL_SIGNED, ..ALL_UNSIGNED, ..ALL_INTEGERS, ..ALL_FLOATS, ..ALL_REALS
end)

#############################


# If a desired file is given, just run that one.
if @isdefined(TEST_NAME)
    include(TEST_NAME * ".jl")
# Otherwise, execute all Julia files in this folder, (other than this one).
else
    test_files = readdir(@__DIR__, join=true)
    filter!(test_files) do name
        return !endswith(name, "runtests.jl") &&
               endswith(name, ".jl")
    end
    for f_path in test_files
        f_name = basename(f_path)
        println("\nRunning ", f_name, "...")
        try
            @eval module $(Symbol(:UnitTests_, f_name))
                $TESTS_DEPENDENCIES
                include($(f_path))
            end
        finally end
    end
end

# If we haven't crashed yet, then I guess the tests worked.
println("Tests finished!")

#TODO: Add tests for "toggleable assert" macros