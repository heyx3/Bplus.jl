# Functions for executing render commands

"
Clears the given render target (or the screen, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       color::vRGBA{T},
                       attachment_idx::Optional{Int} = nothing
                     ) where {T<:Union{GLint, GLuint, GLfloat}}
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    if color isa vRGBA{GLint}
        glClearNamedFramebufferiv(target, GL_COLOR, draw_buffer, Ref(color.data))
    elseif color isa vRGBA{GLuint}
        glClearNamedFramebufferuv(target, GL_COLOR, draw_buffer, Ref(color.data))
    elseif color isa vRGBA{GLfloat}
        glClearNamedFramebufferfv(target, GL_COLOR, draw_buffer, Ref(color.data))
    else
        error("Unexpected color component type: ", T)
    end
end
"
Clears the given depth render target (or the screen's depth buffer, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       depth::GLfloat,
                       attachment_idx::Optional{Int} = nothing
                     )
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    glClearNamedFramebufferfv(target, GL_DEPTH, draw_buffer, Ref(depth))
end
"
Clears the given stencil render target (or the screen's stencil buffer, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       stencil::GLint,  #TODO: Should this take a GLuint instead, since that's more intuitive? Same question for the below overload.
                       attachment_idx::Optional{Int} = nothing
                     )
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    glClearNamedFramebufferiv(target, GL_STENCIL, draw_buffer, Ref(stencil))
end
"
Clears the given depth/stencil render target (or the screen's depth/stencil buffer, if you pass a default/null handle).
Optionally clears only a single attachment of the target.
"
function render_clear( context::Context,
                       target::Ptr_Target,
                       depth::GLfloat,
                       stencil::GLint,
                       attachment_idx::Optional{Int} = nothing
                     )
    draw_buffer::GLint = exists(attachment_idx) ? attachment_idx : GL_NONE
    glClearNamedFramebufferfi(target, GL_DEPTH_STENCIL, draw_buffer, depth, stencil)
end
export render_clear


#TODO: Drawing