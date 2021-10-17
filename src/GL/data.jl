# Defines simple data types used by GL

# SDL Vsync settings
@bp_enum(VsyncModes,
    Off, On, Adaptive
)
export VsyncModes, E_VsyncModes

#=
Whether to ignore polygon faces that are pointing away from the camera
    (or towards the camera, in "Backwards" mode).
=#
@bp_gl_enum(FaceCullModes::GLenum,
    Off       = GL_INVALID_ENUM,
    On        = GL_BACK,
    Backwards = GL_FRONT,
    All       = GL_FRONT_AND_BACK
)
export FaceCullModes, E_FaceCullModes