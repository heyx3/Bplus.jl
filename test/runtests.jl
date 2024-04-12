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
for (sub_package, modules) in Bplus.SUB_MODULES
    sub_package_name = Symbol(sub_package)
    @eval module $(Symbol(:Test_, sub_package_name))
        using $sub_package_name
        include(joinpath(
            pathof($sub_package_name), "..", "..",
            "test", "runtests.jl"
        ))
    end
    reset_julia_env()
end