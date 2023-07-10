# Defines simple data types used by GL

# GLFW Vsync settings
@bp_enum(VsyncModes,
    off = 0,
    on = 1,
    adaptive = -1
)
export VsyncModes, E_VsyncModes

#=
Whether to ignore polygon faces that are pointing away from the camera
    (or towards the camera, in "Backwards" mode).
=#
@bp_gl_enum(FaceCullModes::GLenum,
    off       = GL_INVALID_ENUM,
    on        = GL_BACK,
    backwards = GL_FRONT,
    all       = GL_FRONT_AND_BACK
)
export FaceCullModes, E_FaceCullModes