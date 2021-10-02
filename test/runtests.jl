# Import the main codebase.
using Bplus
using Bplus.Utilities, Bplus.Math

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