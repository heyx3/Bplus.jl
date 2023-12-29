# Make sure the test is always running in the same directory and within the same project.
using Pkg
const MAIN_PROJECT_DIR = joinpath(@__DIR__, "..")
function reset_julia_env()
    cd(MAIN_PROJECT_DIR)
    Pkg.activate(".")
end
reset_julia_env()

# Run each sub-package's unit tests.
using Bplus
for sub_package in Bplus.SUB_PACKAGES
    @eval module $(Symbol(:Test_, sub_package))
        using $sub_package
        include(joinpath(
            pathof($sub_package), "..", "..",
            "test", "runtests.jl"
        ))
    end
    reset_julia_env()
end