#pragma once

#include "../Context.h"
#include "UniformSet.h"


namespace Bplus::GL
{
    namespace Ptr
    {
        //An OpenGL handle to a compiled shader program.
        strong_typedef_start(ShaderProgram, GLuint, BP_API)
            static const ShaderProgram Null;
            strong_typedef_equatable(ShaderProgram, GLuint);
            //strong_typedef_hashable is invoked at the bottom of the file.
        strong_typedef_end;

        const ShaderProgram ShaderProgram::Null = ShaderProgram(0);
    }


    //A specific compiled shader, plus its "uniforms" (a.k.a. parameters).
    class BP_API CompiledShader
    {
    public:
        
        //A unique identifier for this shader within some grouping.
        using ID_t = uint64_t;
        static const ID_t INVALID_ID = 0;

        static CompiledShader* GetCurrentActive();
        
        //Compiles and returns an OpenGL shader program with a vertex and fragment shader.
        //If compilation failed, 0 is returned and an error message is written.
        //Otherwise, the result should eventually be cleaned up with glDeleteProgram().
        static GLuint Compile(const char* vertexShader, const char* fragmentShader,
                              std::string& outErrMsg);
        //Compiles and returns an OpenGL shader program with a vertex, geometry,
        //    and fragment shader.
        //If compilation failed, 0 is returned and an error message is written.
        //Otherwise, the result should be cleaned up with glDeleteProgram().
        static GLuint Compile(const char* vertexShader, const char* geometryShader,
                              const char* fragmentShader,
                              std::string& outErrMsg);


        UniformSet Uniforms;


        //Creates a new instance that manages the given shader program with RAII.
        //Also pre-calculates all shader uniform pointers if they're not set already.
        CompiledShader(GLuint compiledProgramHandle,
                       const UniformSet& uniforms, ID_t _id);

        ~CompiledShader();

        //No copying -- there should only be one of each compiled shader object.
        CompiledShader(const CompiledShader& cpy) = delete;
        CompiledShader& operator=(const CompiledShader& cpy) = delete;

        //Support move constructor/assignment.
        CompiledShader(CompiledShader&& src);
        CompiledShader& operator=(CompiledShader&& src);


        //Sets this shader as the active one, meaning that
        //    all future rendering operations are done with it.
        void Activate();


    private:

        Ptr::ShaderProgram programHandle = Ptr::ShaderProgram::Null;
        ID_t id = INVALID_ID;
    };
}

strong_typedef_hashable(Bplus::GL::Ptr::ShaderProgram, GLuint);