# Defines FileCacher{T}, which manages assets loaded from disk,
#    including checking for new versions and hot-reloading them.


"
For a variety of objects, this checks all their known files' last-modified timestamp,
    and returns whether any of them have changed.
"
function check_disk_modifications! end


##   FileAssociations -- a set of files to check   ##

"A set of files, and their last-known values for 'last time modified'"
const FileAssociations = Dict{AbstractString, DateTime}
function check_disk_modifications!(fa::FileAssociations)
    any_changes::Bool = false
    for file_path in keys(fa)
        last_changed_time = unix2datetime(isfile(file_path) ?
                                            stat(file_path).mtime :
                                            zero(Float64))
        if last_changed_time > fa[file_path]
            #TODO: Are we sure it's officially legal to change values while iterating over the keys?
            fa[file_path] = last_changed_time
            any_changes = true
        end
    end
    return any_changes
end


##   CachedData -- something that depends on some FileAssociations   ##

"A cached version of some file data"
mutable struct CachedData{TCached}
    instance::TCached

    # Any changes to these files should cause a cache invalidation.
    files::FileAssociations
    root_file_full_path::AbstractString

    last_check_time::DateTime
    disk_check_interval::Millisecond
end

"
Implement this function for your `TCached` data type.
It should return the loaded instance, optionally paired with an iterator of 'dependent' file paths
    that can also trigger a reload of this data.

You must overload the first version, but you can also support the second version
    to re-use resources from a previously-closed instance (see `close_cached_data()`).
"
load_uncached_data(T, path) = error("load_uncached_data() not implemented for ", T)
load_uncached_data(T, path, pooled_resources) = error("load_uncached_data() not implemented for ", T)
load_uncached_data(T, path, ::Nothing) = load_uncached_data(T, path)

"
Cleans up the data, and optionally returns stuff to be re-used
    for new instances (see `load_uncached_data()`).
This is called 'pooling'.

If you *do* return extra stuff, and that pooled data itself needs cleanup,
    then you should also overload `close_pooled_data()` to handle a rare case
    where that stuff doesn't get used for pooling.
"
close_cached_data(c) = nothing
"
Cleans up 'pooled' data, meaning data returned from `close_cached_data()`
    in the hopes of re-using it for new instances.
In rare cases, this pooled data is not re-used, and then this function is called.

The first parameter is the type of the cached data (usually referred to as `TCached`).
"
close_pooled_data(T, d) = nothing

function check_disk_modifications!(cd::CachedData)::Bool
    current_datetime = now()
    if (current_datetime - cd.last_check_time) >= cd.disk_check_interval
        cd.last_check_time = current_datetime
        return check_disk_modifications!(c.files)
    else
        return false
    end
end


##   FileCacher -- manages a set of CachedData   ##

"
Keeps track of file data of a certain type and automatically checks it for changes.

To use it, define the following things:

* Some 'TCached' type representing your loaded file data
* `load_uncached_data(::Type{TCached}, path::AbstractString[, pooled_resources])` to load the file data
  * Optionally return it in a tuple with a iterator of other dependent files that affect this data
  * If you want to facilitate object pooling, return some resources when closing an instance (see below) and take those resources as a third parameter.
* `close_cached_data(c::TCached)` is called when an out-of-date file is cleaned up.
  * By default, it does nothing.
  * If you overload this function, you can return some stuff that will get passed to the next `load_uncached_data()` call, allowing you to set up pooling.

You can configure the cacher in a few ways:

* 'error_response' is called when a file doesn't exist. It can log and/or return a fallback.
  * By default it `@error`s and returns `nothing`.
* 'check_interval_ms' is a range of wait times in between checking files for changes. A wait time is randomly chosen for each file,
     to prevent them from all checking the disk at once.
* 'relative_path' is the path that all files are relative to (except for ones that are given as an absolute path).
  * By default, it's the process's current location.
"
mutable struct FileCacher{TCached}
    error_response::Base.Callable
    check_interval_ms::IntervalU
    relative_path::String
    files::Dict{AbstractString, CachedData{TCached}} # Stored as their absolute, canonical paths.
    buffer::Vector{AbstractString} # Used within some functions

    @inline function FileCacher{TCached}(; check_interval_ms = IntervalU(min=3000, max=5000),
                                           error_response = default_cache_error_response,
                                           relative_path = pwd()
                                        ) where {TCached}
        return new(
            error_response,
            convert(IntervalU, check_interval_ms),
            string(relative_path),
            Dict{AbstractString, CachedData{TCached}}(),
            Vector{AbstractString}()
        )
    end
end
#TODO: Rewrite this so a separate Task constantly checks all the files, sleeping in between each one

function default_cache_error_response(path, exception, trace)
    @error "Unable to load $path." ex=(exception, trace)
    return nothing
end
function get_cached_path(fc::FileCacher, relative_path::AbstractString)::String
    if isabspath(relative_path)
        return normpath(string(relative_path))
    else
        return normpath(fc.relative_path, string(relative_path))
    end
end
"
Attempts to load the given file and its cache metadata.
Returns `nothing` if it failed.

Optionally takes some pooled resources to give to the data constructor.
"
function make_data_cache( fc::FileCacher{TCached},
                          full_path,
                          pooled_resource...
                        )::Optional{CachedData{TCached}} where {TCached}
    # Load the data.
    (new_data, new_dependent_files) = try
        loaded = load_uncached_data(TCached, full_path, pooled_resource...)
        if loaded isa Tuple
            loaded
        else
            (loaded, ())
        end
    # If it fails, call the fallback lambda and try to use that instead.
    catch e
        close_pooled_data.(Ref(TCached), pooled_resource)
        (fc.error_response(full_path, e, catch_backtrace()), ())
    end

    # If we have a file to cache, cache it.
    if new_data isa TCached
        return CachedData(
            new_data,
            # Get the "last-modified" time for each dependent file, plus the main one.
            let paths = tuple(full_path, (get_cached_path(fc, p) for p in new_dependent_files)...),
                file_modified_time(p) = unix2datetime(state(p).mtime)
                Dict(p=>file_modified_time(p) for p in paths)
            end,
            full_path, now(), rand(fc.check_interval_ms)
        )
    # Otherwise, its an error case without a valid fallback file. Wipe it out of the cache.
    else
        return nothing
    end
end

function check_disk_modifications!(fc::FileCacher{TCached})::Bool where {TCached}
    any_changes::Bool = false

    empty!(fc.buffer)
    keys = fc.buffer
    append!(keys, keys(fc.files))

    for path in keys
        data = fc.files[path]
        if check_disk_modifications!(data)
            any_changes = true
            pooled_resources = close_cached_data(data)
            new_cache = make_data_cache(fc, path, pooled_resources)
            if exists(new_cache)
                fc.files[path] = new_cache
            else
                delete!(fc.files, path)
            end
        end
    end

    return any_changes
end

"
Retrieves the given file data, using a cached version if available.

If the file can't be loaded, and no fallback 'error' file was provided to the cacher,
    then `nothing` is returned.
"
function get_cached_data!(fc::FileCacher{TCached}, path::AbstractString)::Optional{TCached} where {TCached}
    full_path = get_cached_path(fc, path)

    if haskey(fc.files, full_path)
        return fc.files[full_path]
    else
        new_value = make_data_cache(fc, full_path)
        if exists(new_value)
            fc.files[full_path] = new_value
        end
        return new_value
    end
end


export FileCacher,
       load_uncached_data, close_cached_data, close_pooled_data,
       get_cached_data, check_disk_modifications!