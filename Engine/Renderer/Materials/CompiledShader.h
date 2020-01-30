#pragma once

#include "../Context.h"


namespace Bplus::GL
{
    //Various OpenGL handles, wrapped in type-safety.
    namespace Ptr
    {
        //An OpenGL handle to a compiled shader program.
        #pragma region ShaderProgram

        strong_typedef_start(ShaderProgram, GLuint, BP_API)

            static const ShaderProgram Null;

            strong_typedef_equatable(ShaderProgram, GLuint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        const ShaderProgram ShaderProgram::Null = ShaderProgram(0);

        #pragma endregion
        
        //An OpenGL handle to a specific uniform within a shader.
        //If it points to an array of uniforms,
        //    you can get element 'i' in the array by adding 'i' to this value.
        //The byte-size of the uniform has no bearing on this numbering scheme.
        #pragma region ShaderUniform

        strong_typedef_start(ShaderUniform, GLint, BP_API)

            static const ShaderUniform Null;

            strong_typedef_equatable(ShaderUniform, GLint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        const ShaderUniform ShaderUniform::Null = ShaderUniform(-1);

        #pragma endregion

        //An OpenGL handle to a specific sampleable texture.
        //The difference between this and "Image" is that Samplers
        //    have sampling settings (linear, wrap, etc)
        //    while Images are basically plain 2D arrays.
        #pragma region Sampler

        strong_typedef_start(Sampler, GLuint, BP_API)

            static const Sampler Null;

            strong_typedef_equatable(Sampler, GLuint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        const Sampler Sampler::Null = Sampler(0);

        #pragma endregion

        //An OpenGL handle to a specific image buffer.
        //The difference between this and "Sampler" is that Samplers
        //    have sampling settings (linear, wrap, etc)
        //    while Images are basically just 2D arrays.
        #pragma region Image

        strong_typedef_start(Image, GLuint, BP_API)

            //TODO: Check whether 0 is actually the "invalid" handle for an Image.
            //static const Image Null;

            strong_typedef_equatable(Image, GLuint);
            //strong_typedef_hashable is invoked at the bottom of the file.

        strong_typedef_end;

        //const Image Image::Null = Image(0);

        #pragma endregion
    }


    //A specific compiled shader, plus its "uniforms" (a.k.a. parameters).
    class BP_API CompiledShader
    {
    public:
        
        //Gets the currently-active shader program.
        //Returns nullptr if none is active.
        static CompiledShader* GetCurrentActive();

        
        //Compiles and returns an OpenGL shader program with a vertex and fragment shader.
        //If compilation failed, 0 is returned and an error message is written.
        //Otherwise, the result should eventually be cleaned up with glDeleteProgram().
        static Ptr::ShaderProgram Compile(const char* vertexShader, const char* fragmentShader,
                                          std::string& outErrMsg);
        //Compiles and returns an OpenGL shader program with a vertex, geometry,
        //    and fragment shader.
        //If compilation failed, 0 is returned and an error message is written.
        //Otherwise, the result should be cleaned up with glDeleteProgram().
        static Ptr::ShaderProgram Compile(const char* vertexShader, const char* geometryShader,
                                          const char* fragmentShader,
                                          std::string& outErrMsg);


        //Creates a new instance that manages the given shader program with RAII.
        //Also pre-calculates all shader uniform pointers if they're not set already.
        CompiledShader(Ptr::ShaderProgram compiledProgramHandle,
                       const std::vector<std::string>& uniformNames);

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

        //TODO: Uniform functionality.


    private:

        Ptr::ShaderProgram programHandle = Ptr::ShaderProgram::Null;
        std::unordered_map<std::string, Ptr::ShaderUniform> uniformPtrs;
    };
}


//Add hashing support for the various OpenGL handles.
strong_typedef_hashable(Bplus::GL::Ptr::ShaderProgram, GLuint);
strong_typedef_hashable(Bplus::GL::Ptr::ShaderUniform, GLint, BP_API);
strong_typedef_hashable(Bplus::GL::Ptr::Sampler, GLuint, BP_API);
strong_typedef_hashable(Bplus::GL::Ptr::Image, GLuint, BP_API);