# Point Julia and Pkg to this project.
using Pkg
cd(joinpath(@__DIR__, ".."))
Pkg.activate(".")

# If this is a git repo (not a downloaded snapshot of the code),
#    update submodules.
if isdir(".git")
    println("This is a git repo, so we're going to update submodules")
    try
        run(`git submodule init`)
        run(`git submodule update`)
    catch e
        println("Submodule commands failed; we're going to hope they exist anyway and keep going.")
        print("\tThe error: ")
        showerror(stdout, e)
    end
end

# Add our ModernGL fork, ModernGLbp, as a dependency.
println("Pointing Pkg to ModernGLbp...")
Pkg.develop(url="dependencies/ModernGLbp.jl")

# Check that packages are all set up.
println("Checking that packages all exist and can compile...")
Pkg.instantiate()
Pkg.precompile()