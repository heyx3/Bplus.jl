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
                                            typemax(UInt32)) # Larger values create a negative time
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

function check_disk_modifications!(cd::CachedData)::Bool
    current_datetime = now()
    if (current_datetime - cd.last_check_time) >= cd.disk_check_interval
        cd.last_check_time = current_datetime
        return check_disk_modifications!(cd.files)
    else
        return false
    end
end


##   FileCacher -- manages a set of CachedData   ##

"
Keeps track of file data of a certain type and automatically checks it for changes. Has the following interface:

* Update it and check for modified files with `check_disk_modifications!()`.
    * Ideally, call this every frame.
    * If at least one file changed, it returns true, and also raises a callback to reload each changed file.
* Get a file with `get_cached_data!()`. You can provide both relative and absolute paths.
* Get the canonical representation of a file path within the cacher with `get_cached_path()`.

You can configure the cacher in a few ways:

* `reload_response(path[, old_data]) -> new_data[, dependent_files]` is called when either
    a file is being loaded for the first time, OR a file has changed and should be reloaded.
  * Returns the new file, and optionally an iterator of 'dependent' files.
      whose changes will also trigger a reload.
  * If your response throws an error then `error_response`, mentioned below, is invoked.
* `error_response(path, exception, trace[, old_data]) -> [fallback_data]` is called when a file fails to load or re-load.
  * By default it `@error`s and returns `nothing`.
  * You can change it to, for example, return a fallback 'error data' object, or the previous version of the object.
* `check_interval_ms` is a range of wait times in between checking files for changes. A wait time is randomly chosen for each file,
     to prevent them from all checking the disk at once.
* `relative_path` is the prefix for relative paths.
  * By default, it's the process's current location.
"
@kwdef mutable struct FileCacher{TCached}
    reload_response::Base.Callable # (path[, old]) -> new[, dependent_files]
    error_response::Base.Callable = default_cache_error_response # (path, exception, trace[, old]) -> [new]

    relative_path::String = pwd()
    check_interval_ms::IntervalU = 3000:5000

    files::Dict{AbstractString, CachedData{TCached}} = Dict() # Stored as their absolute, canonical paths.
    buffer::Vector{AbstractString} = [ ] # Used within some functions
end
#TODO: Rewrite this so a separate Task constantly checks all the files, sleeping in between each one

function default_cache_error_response(path, exception, trace, old_data = nothing)
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
"Loads the given file and its cache metadata"
function make_data_cache( fc::FileCacher{TCached},
                          data::TCached,
                          data_path::AbstractString,
                          dependent_files
                        )::CachedData{TCached} where {TCached}
    return CachedData(
        data,
        # Get the "last-modified" time for each dependent file, plus the main one.
        let paths = tuple((get_cached_path(fc, p) for p in (data_path, dependent_files...))...),
            file_modified_time(p) = unix2datetime(stat(p).mtime)
            Dict{AbstractString, DateTime}(p=>file_modified_time(p) for p in paths)
        end,
        get_cached_path(fc, data_path),
        now(),
        Millisecond(rand(fc.check_interval_ms))
    )
end

function check_disk_modifications!(fc::FileCacher{TCached})::Bool where {TCached}
    any_changes::Bool = false

    empty!(fc.buffer)
    current_keys = fc.buffer
    append!(current_keys, keys(fc.files))

    for path in current_keys
        data = fc.files[path]
        if check_disk_modifications!(data)
            any_changes = true

            delete!(fc.files, path)

            # Load the new data (or a fallback if it throws).
            result = try
                fc.reload_response(path, data.instance)
            catch e
                fc.error_response(path, e, catch_backtrace(), data.instance)
            end
            (new_data, new_dependent_files) = if result isa Tuple{TCached, Any}
                result
            else
                (result, ())
            end

            # Replace the data in the cache.
            if new_data isa TCached
                fc.files[path] = make_data_cache(fc, new_data, path, new_dependent_files)
            end
        end
    end

    return any_changes
end

"
Retrieves the given file data, using a cached version if available.

Returns `nothing` if the file can't be loaded and no fallback error value was provided to the cacher.
"
function get_cached_data!(fc::FileCacher{TCached}, path::AbstractString)::Optional{TCached} where {TCached}
    full_path = get_cached_path(fc, path)

    if haskey(fc.files, full_path)
        return fc.files[full_path].instance
    else
        # Load the file data (or a fallback if it throws).
        result = try
            fc.reload_response(full_path)
        catch e
            fc.error_response(full_path, e, catch_backtrace(), data)
        end
        (file_data, dependent_files) = if result isa Tuple{TCached, Any}
            result
        else
            (result, ())
        end

        # Put the data in the cache and then return it.
        if file_data isa TCached
            fc.files[full_path] = make_data_cache(fc, file_data, full_path, dependent_files)
            return file_data
        else
            return nothing
        end
    end
end


export FileCacher,
       load_uncached_data, close_cached_data, close_pooled_data,
       get_cached_data!, check_disk_modifications!