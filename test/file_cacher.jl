# Test the caching of custom data.

mutable struct MyCacheableData
    i::Int
    f::Float32
    previous_caches::Vector{MyCacheableData}
end
StructTypes.StructType(::Type{MyCacheableData}) = StructTypes.Mutable()
MyCacheableData() = MyCacheableData(typemin(Int), +Inf32, [ ])

# For testing, define equality based on fields and not specific instances.
Base.:(==)(a::MyCacheableData, b::MyCacheableData)::Bool = all((
    a.i == b.i,
    isapprox(a.f, b.f, nans=true),
    length(a.previous_caches) == length(b.previous_caches),
    all(a.previous_caches[i] == b.previous_caches[i]
          for i in 1:length(a.previous_caches))
))


const TEMP_PATH = joinpath(tempdir(), "Bplus_test_FileCacher")
mkpath(TEMP_PATH)
file_full_path(relative_path) = joinpath(TEMP_PATH, relative_path)

file_update(relative_path, data::MyCacheableData) = open(file_full_path(relative_path), "w") do f
    print(f, JSON3.write(data))
end
file_load(relative_path) = open(file_full_path(relative_path)) do f
    return JSON3.read(read(f, String), MyCacheableData)
end
file_delete(relative_path) = rm(file_full_path(relative_path))


const ERROR_CACHED_DATA = MyCacheableData(-666, NaN32, [ ])
const CACHER = FileCacher{MyCacheableData}(
    reload_response = (path, old...) -> let new = file_load(path)
        push!(new.previous_caches, old...)
        new
    end,
    error_response = (path, ex, trace, old...) -> begin
        # println(stderr, "Failed to ", isempty(old) ? "load" : "reload", " ", path, ": ")
        # showerror(stderr, ex, trace)
        ERROR_CACHED_DATA
    end,
    relative_path = TEMP_PATH,
    check_interval_ms = 1:1
)

function check_equality(actual::MyCacheableData, expected::MyCacheableData,
                        error_msg_data...)
    if actual != expected
        error("Failed ", error_msg_data..., ":",
              "\n\tActual  : ", actual,
              "\n\tExpected: ", expected)
    end
end
check_equality(a::String, b, rest...) = check_equality(get_cached_data!(CACHER, a), b, rest...)
check_equality(a, b::String, rest...) = check_equality(a, get_cached_data!(CACHER, b), rest...)

# Double-check that we can work within the temp directory.
const CHECKFILE_PATH = joinpath(TEMP_PATH, "BplusDummy.nothing")
try
    open((file -> nothing), CHECKFILE_PATH, "w")
    open((file -> nothing), CHECKFILE_PATH, "r")
catch e
    error("Unable to use temp directory '$TEMP_PATH' for test: ",
          sprint(showerror, e))
end

# Ensure that the temp directory is cleaned up at the end of the tests.
try
    # Create some initial files.
    file_update("a.json", MyCacheableData(1, 1.1, [ ]))
    file_update("b.json", MyCacheableData(2, 2.2, [ ]))
    file_update("c.json", MyCacheableData(3, 3.3, [ ]))
    check_equality(MyCacheableData(1, 1.1, [ ]), "a.json",
                "Loading initial a.json")
    check_equality(MyCacheableData(2, 2.2, [ ]), "b.json",
                "Loading initial b.json")
    check_equality(MyCacheableData(3, 3.3, [ ]), "c.json",
                "Loading initial c.json")

    # The cache should now be holding all three of them.
    @bp_check(keys(CACHER.files) == Set(file_full_path.([ "a.json", "b.json", "c.json" ])),
            "Wrong cached file keys: ", keys(CACHER.files))

    # Update one of them.
    file_update("a.json", MyCacheableData(-1, -0.0111, [ ]))
    sleep(0.1)
    found_modifications::Bool = check_disk_modifications!(CACHER)
    @bp_check(found_modifications, "Didn't notice modification to a.json")
    check_equality("a.json", MyCacheableData(-1, -0.0111, [ MyCacheableData(1, 1.1, [ ]) ]))

    # Delete B, and check that it is now missing in the cache.
    file_delete("b.json")
    sleep(0.1)
    found_modifications = check_disk_modifications!(CACHER)
    @bp_check(found_modifications, "Didn't notice deletion of b.json")
    check_equality("b.json", ERROR_CACHED_DATA)
finally
    rm(TEMP_PATH, recursive=true)
end