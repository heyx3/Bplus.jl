#=

    UI layout:

    ------------------------------------------------------------------------------
    |    [[Path tab]] [Favorites tab] [Search tab]            [x] Show Un-       |
    | |---------------------------------------------------|        selectable    |
    | |  Path: [  C:\Users\manni\Documents\             ] |                      |
    | |---------------------------------------------------|   [Refresh]          |
    |                                                                            |
    |     |  Name   ^    |   Last Modified   |   Type   |   Size   | Selectable  |
    |                                                                            |
    |       MyDir/         10/11/1992 10:00am    DIR        43 MB...             |
    |       MyTinyDir/     10/11/1992 10:00am    DIR        1 MB                 |
    |       a.txt          10/11/1992 9:00am     .txt       43 MB                |
    |       b.txt          10/12/1992 9:00am     .txt       42 MB                |
    |                                                                            |
    |                                                                            |
    |  New name: [   a.txt     ]         [Open]   [Cancel]                       |
    ------------------------------------------------------------------------------

=#

@bp_enum(FileDialogColumns,
    name, last_modified, type, size, selectable
)
@bp_enum(FileDialogTabs,
    path, favorites, search
)

"A file or folder in the file dialog GUI"
struct Entry
    name::AbstractString
    last_modified::Float64
    type::AbstractString
    size::Int
    selectable::Bool
    is_folder::Bool
end

"Persistent user settings for file dialogs. Serializable with JSON3/StructTypes."
Base.@kwdef mutable struct FileDialogSettings
    directory::AbstractString
    file_name::AbstractString

    sort_column::E_FileDialogColumns = FileDialogColumns.name
    sort_is_descending::Bool = true

    show_unselectable_options::Bool = false
end
StructTypes.StructType(::Type{FileDialogSettings}) = StructTypes.Mutable()

"Configuration for a specific file dialog instance."
Base.@kwdef struct FileDialogParams
    is_writing::Bool
    is_folder_mode::Bool = false

    confirm_button_label::Optional{AbstractString} = "OK"
    cancel_button_label::Optional{AbstractString} = "Cancel"

    allowed_tabs::Set{E_FileDialogTabs} = Set(FileDialogTabs.instances())
    reading_pattern::Optional{Regex} = nothing
end

"The current state of a specific file dialog instance."
mutable struct FileDialogState
    current_tab::Optional{E_FileDialogTabs} = nothing
    entries::Vector{Entry} = [ ]
end


@bp_enum(FileDialogResult,
    confirmed, canceled
)
function gui_file_dialog(settings::FileDialogSettings,
                         params::FileDialogParams,
                         state::FileDialogState
                        )::Optional{E_FileDialogResult}
    gui_within_group() do # The tabs
        #TODO: Implement tabs
    end
    CImGui.SameLine()
    gui_within_group() do # Side buttons
        @c CImGui.Checkbox("Show unselectable", &settings.show_unselectable_options)
        if CImGui.Button("Refresh")
            state.entries = map(readdir(settings.directory, sort=false)) do name::AbstractString
                info = stat(name)
                is_folder = isfile(joinpath(settings.directory, name))
                return Entry(
                    name,
                    info.mtime,
                    is_folder ? "DIR" : string(name[findlast('.', name):end]),
                    info.size,
                    isnothing(params.reading_pattern) ||
                      exists(matches(params.reading_pattern, name)),
                    is_folder
                )
            end
        end
    end

    CImGui.Dummy(gVec2(0, 40))

    #TODO: File scroll

    CImGui.Dummy(gVec2(0, 20))

    #TODO: File name, Confirm, Cancel
end

#TODO: Helper to try loading FileDialogSettings from disk

export FileDialogColumns, E_FileDialogColumns, FileDialogSettings,
       FileDialogParams,
       FileDialogTabs, E_FileDialogTabs, FileDialogState,
       FileDialogResult, gui_file_dialog


##  Helper functions  ##

function sort_files(file_names::Vector{AbstractString}, sort_column::E_FileDialogColumns, descending::Bool)
    sort!(
        file_names, rev=descending,
        by = if sort_column == FileDialogColumns.name
                 data -> (data.is_folder, data.name)
             elseif sort_column == FileDialogColumns.last_modified
                 data -> (data.is_folder, data.last_modified)
             elseif sort_column == FileDialogColumns.type
                 data -> (data.is_folder, data.type)
             elseif sort_column == FileDialogColumns.size
                 data -> (data.is_folder, data.size)
             elseif sort_column == FileDialogColumns.selectable
                 data -> (data.is_folder, data.selectable)
             else
                 error("Unhandled column: ", sort_column)
             end
    )
end