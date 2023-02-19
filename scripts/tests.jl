cd(joinpath(@__DIR__, ".."))
insert!(LOAD_PATH, 1, ".")
insert!(LOAD_PATH, 1, "test")

if isempty(ARGS)
    println("No arguments, running all tests")
elseif length(ARGS) == 1
    println("Only testing Julia file '", ARGS[1], "'")
    TEST_NAME = ARGS[1]
else
    println("ERROR: Expected 0 or 1 arguments, got: ", ARGS)
    exit(1)
end

include(joinpath(@__DIR__, "..", "test", "runtests.jl"))