# Provides an interface into the more modern OpenGL 'Debug Output'.
# Reference: https://www.khronos.org/opengl/wiki/Debug_Output

##################
##     Data     ##
##################

"Provides a string description of the given debug enum value"
function debug_string(enum)::String end
export debug_string


@bp_gl_enum(DebugEventSources::GLenum,
    api = GL_DEBUG_SOURCE_API,
    window_system = GL_DEBUG_SOURCE_WINDOW_SYSTEM,
    shader_compiler = GL_DEBUG_SOURCE_SHADER_COMPILER,
    user = GL_DEBUG_SOURCE_APPLICATION,
    library = GL_DEBUG_SOURCE_THIRD_PARTY,
    other = GL_DEBUG_SOURCE_OTHER
)
function debug_string(source::E_DebugEventSources)
    if source == DebugEventSources.api
        return "calling a 'gl' method"
    elseif source == DebugEventSources.window_system
        return "calling an SDL-related method"
    elseif source == DebugEventSources.shader_compiler
        return "compiling a shader"
    elseif source == DebugEventSources.third_party
        return "within some internal OpenGL app"
    elseif source == DebugEventSources.user
        return "raised by the user (`glDebugMessageInsert()`)"
    elseif source == DebugEventSources.other
        return "some unspecified context"
    else
        return "INTERNAL_B+_ERROR(Unhandled source: $source)"
    end
end
export DebugEventSources, E_DebugEventSources


@bp_gl_enum(DebugEventTopics::GLenum,
    deprecation = GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,
    undefined_behavior = GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,
    portability = GL_DEBUG_TYPE_PORTABILITY,
    performance = GL_DEBUG_TYPE_PERFORMANCE,

    custom = GL_DEBUG_TYPE_MARKER,
    begin_group = GL_DEBUG_TYPE_PUSH_GROUP,
    end_group = GL_DEBUG_TYPE_POP_GROUP,
    other = GL_DEBUG_TYPE_OTHER
)
function debug_string(topic::E_DebugEventTopics)
    if topic == DebugEventTopics.deprecation
        return "deprecated usage"
    elseif topic == DebugEventTopics.undefined_behavior
        return "undefined behavior"
    elseif topic == DebugEventTopics.portability
        return "non-portable behavior"
    elseif topic == DebugEventTopics.performance
        return "inefficiency"
    elseif topic == DebugEventTopics.custom
        return "command-stream annotation"
    elseif topic == DebugEventTopics.begin_group
        return "BEGIN group"
    elseif topic == DebugEventTopics.end_group
        return "END group"
    elseif topic == DebugEventTopics.other
        return "other"
    else
        return "INTERNAL_B+_ERROR(Unhandled topic: $topic)"
    end
end
export DebugEventTopics, E_DebugEventTopics


@bp_gl_enum(DebugEventSeverities::GLenum,
    none = GL_DEBUG_SEVERITY_NOTIFICATION,
    low = GL_DEBUG_SEVERITY_LOW,
    medium = GL_DEBUG_SEVERITY_MEDIUM,
    high = GL_DEBUG_SEVERITY_HIGH
)
function debug_string(severity::E_DebugEventSeverities)
    if severity == DebugEventSeverities.none
        return "information"
    elseif severity == DebugEventSeverities.low
        return "mild"
    elseif severity == DebugEventSeverities.medium
        return "concerning"
    elseif severity == DebugEventSeverities.high
        return "fatal"
    else
        return "INTERNAL_B+_ERROR(Unhandled severity: $severity)"
    end
end
export DebugEventSeverities, E_DebugEventSeverities


#####################
##    Interface    ##
#####################

struct OpenGLEvent
    source::E_DebugEventSources
    topic::E_DebugEventTopics
    severity::E_DebugEventSeverities
    msg::String
    msg_id::Int
end

Base.show(io::IO, event::OpenGLEvent) = print(io,
    "OpenGL (", event.msg_id, ") | ",
    uppercasefirst(debug_string(event.severity)), " event about ",
    debug_string(event.topic), ", from ",
    debug_string(event.source), ": \"", event.msg, "\""
)

"
Pulls all unread event logs from the current OpenGL context,
    in chronological order.
Only works if the context was started in debug mode.
"
function pull_gl_logs()::Vector{OpenGLEvent}
    PULL_INCREMENT::Int = 32
    max_msg_length::GLint = get_from_ogl(GLint, glGetIntegerv, GL_MAX_DEBUG_MESSAGE_LENGTH)
    output = OpenGLEvent[ ]

    lock(GL_EVENT_LOCKER) do
        # Make sure the buffers are large enough.
        resize!(BUFFER_EVENT_SOURCES, PULL_INCREMENT)
        resize!(BUFFER_EVENT_TOPICS, PULL_INCREMENT)
        resize!(BUFFER_EVENT_SEVERITIES, PULL_INCREMENT)
        resize!(BUFFER_EVENT_IDS, PULL_INCREMENT)
        resize!(BUFFER_EVENT_LENGTHS, PULL_INCREMENT)
        resize!(BUFFER_EVENT_MSGS, PULL_INCREMENT * max_msg_length)

        # Read messages.
        pull_amount::GLuint = GLuint(1)
        while pull_amount > 0
            # Load the data from OpenGL.
            pull_amount = glGetDebugMessageLog(
                PULL_INCREMENT,
                length(BUFFER_EVENT_MSGS),
                Ref(BUFFER_EVENT_SOURCES, 1),
                Ref(BUFFER_EVENT_TOPICS, 1),
                Ref(BUFFER_EVENT_IDS, 1),
                Ref(BUFFER_EVENT_SEVERITIES, 1),
                Ref(BUFFER_EVENT_LENGTHS, 1),
                Ref(BUFFER_EVENT_MSGS, 1)
            )

            # Copy the data into the output array.
            msg_start_idx::Int = 1
            for log_i::Int in 1:Int(pull_amount)
                msg_range = msg_start_idx : (msg_start_idx + BUFFER_EVENT_LENGTHS[log_i] - 1)
                msg_start_idx += BUFFER_EVENT_LENGTHS[log_i] # The string lengths include
                                                             #    the null-terminator
                push!(output, OpenGLEvent(
                    BUFFER_EVENT_SOURCES[log_i],
                    BUFFER_EVENT_TOPICS[log_i],
                    BUFFER_EVENT_SEVERITIES[log_i],
                    String(@view(BUFFER_EVENT_MSGS[msg_range])),
                    BUFFER_EVENT_IDS[log_i]
                ))
            end
        end
    end

    return output
end

export OpenGLEvent, pull_gl_logs


##########################
##    Implementation    ##
##########################

const GL_EVENT_LOCKER = ReentrantLock()

const BUFFER_EVENT_SOURCES = Vector{E_DebugEventSources}()
const BUFFER_EVENT_TOPICS = Vector{E_DebugEventTopics}()
const BUFFER_EVENT_SEVERITIES = Vector{E_DebugEventSeverities}()
const BUFFER_EVENT_IDS = Vector{GLuint}()
const BUFFER_EVENT_LENGTHS = Vector{GLsizei}()
const BUFFER_EVENT_MSGS = Vector{GLchar}()

###########################################################################



###########################################################################

# Unfortunately, the nicer callback-based interface doesn't work.
# As far as I can tell, it's due to a limitation with Julia's feature
#    of turning Julia functions into C-friendly functions --
#    the calling convention expected by OpenGL can't be chosen from the @cfunction macro.

###########################
##     Old Interface     ##
###########################

"
The global callback for OpenGL errors and log messages.
Only raised if the context is created in debug mode.
You can freely set this to point to a different function.
"
ON_OGL_MSG = (source::E_DebugEventSources,
              topic::E_DebugEventTopics,
              severity::E_DebugEventSeverities,
              msg_id::Int,
              msg::String) ->
begin
    println(stderr,
            "OpenGL (", msg_id, ") | ",
            uppercasefirst(debug_string(severity)), " event about ",
            debug_string(topic), ", from ",
            debug_string(source), ": \"", msg, "\"")
end

"
Raises the global OpenGL message-handling callback with some message.
This must be called from a thread with an active OpenGL context.
"
function raise_ogl_msg(topic::E_DebugEventTopics,
                       severity::E_DebugEventSeverities,
                       message::String
                       ;
                       source::E_DebugEventSources = DebugEventSources.user,
                       id::GLuint = 0)
    glDebugMessageInsert(GLenum(source), GLenum(topic), id, GLenum(severity),
                         length(message), Ref(message))
end


############################
##     Implementation     ##
############################

"The internal C callback for OpenGL errors/messages."
function ON_OGL_MSG_impl(source::GLenum, topic::GLenum, msg_id::GLuint,
                         severity::GLenum,
                         msg_len::GLsizei, msg::Ptr{GLchar},
                         ::Ptr{Cvoid})::Cvoid
    println("RECEIVED")
    ON_OGL_MSG(E_DebugEventSources(source),
               E_DebugEventTopics(topic),
               E_DebugEventSeverities(severity),
               Int(msg_id),
               unsafe_string(msg, msg_len))
    return nothing
end
"A C-friendly function pointer to the OpenGL error/message callback."
const ON_OGL_MSG_ptr = @cfunction(ON_OGL_MSG_impl, Cvoid,
                                  (GLenum, GLenum, GLuint, GLenum,
                                   GLsizei, Ptr{GLchar}, Ptr{Cvoid}))