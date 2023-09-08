const AXIS_POSITIVE_DIR = tuple(
    v3f(1, 0, 0, 0),
    v3f(0, 1, 0, 0),
    v3f(0, 0, 1, 0)
)

const QUAD_UVS = let (q_min, q_max) = Float32.(0.5, 0.8)
    Float32[
        q_min, q_min,
        q_min, q_max,
        q_max, q_max,
        q_max, q_min
    ]
end

const HALF_CIRCLE_SEGMENT_COUNT = 64

const SNAP_TENSION = Float32(0.5)


# ImGUI format strings.
# Pin them as globals, then on module initialization
#    convert each of those pinned globals to a C-string.

const TRANSLATION_INFO_MASK_SOURCES = tuple(
    "X : %5.3f", "Y : %5.3f", "Z : %5.3f",
    "Y : %5.3f Z : %5.3f",
    "X : %5.3f Z : %5.3f",
    "X : %5.3f Y : %5.3f",
    "X : %5.3f Y : %5.3f Z : %5.3f"
)
const SCALE_INFO_MASK_SOURCES = tuple(
    "X : %5.2f",
    "Y : %5.2f",
    "Z : %5.2f",
    "XYZ : %5.2f"
)
const ROTATION_INFO_MASK_SOURCES = tuple(
    "X : %5.2f deg %5.2f rad",
    "Y : %5.2f deg %5.2f rad",
    "Z : %5.2f deg %5.2f rad",
    "Screen : %5.2f deg %5.2f rad"
)

const TRANSLATION_INFO_MASK = Vector{Ptr{Cchar}}()
const ROTATION_INFO_MASK = Vector{Ptr{Cchar}}()
const SCALE_INFO_MASK = Vector{Ptr{Cchar}}()

push!(RUN_ON_INIT, () -> begin
    for source in TRANSLATION_INFO_MASK_SOURCES
        push!(TRANSLATION_INFO_MASK, Base.unsafe_convert(Ptr{Cchar}, source))
    end
    for source in ROTATION_INFO_MASK_SOURCES
        push!(ROTATION_INFO_MASK, Base.unsafe_convert(Ptr{Cchar}, source))
    end
    for source in SCALE_INFO_MASK_SOURCES
        push!(SCALE_INFO_MASK, Base.unsafe_convert(Ptr{Cchar}, source))
    end
end)