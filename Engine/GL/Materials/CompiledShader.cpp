#include "CompiledShader.h"

#include "../Uniforms/Storage.h"
#include "Factory.h"
#include "ShaderDefinition.h"


using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Materials;


CompiledShader::CompiledShader(OglPtr::ShaderProgram&& compiledProgramHandle,
                               const Uniforms::Definitions& uniforms)
    : programHandle(compiledProgramHandle)
{
    compiledProgramHandle = OglPtr::ShaderProgram::Null();

    //Build the map of uniforms and their current values.
    uniforms.VisitAllUniforms(false,
    [&](const std::string& uName, const Uniforms::Type& uType)
    {
        BP_ASSERT_STR(uniformPtrs.find(uName) == uniformPtrs.end(),
                      "Uniform '" + uName +
                          "' has already been defined. New definition is of type " +
                          GetDescription(uType));

        auto uShaderName = ShaderDefinition::Prefix_Uniforms() + uName;
        OglPtr::ShaderUniform ptr(glGetUniformLocation(programHandle.Get(), uShaderName.c_str()));
        uniformPtrs[uName] = ptr;
        
        //Set up the default value for this uniform.
        if (std::holds_alternative<Uniforms::Vector>(uType.ElementType))
        {
            #pragma region Vector uniforms

            const auto& vData = std::get<Uniforms::Vector>(uType.ElementType);

            switch (vData.Type)
            {
                #define VDATA_TYPE(scalarType, glmInLetter, glmOutLetter) \
                    case Uniforms::ScalarTypes::scalarType: { \
                        auto vec = glm::glmOutLetter##vec4(std::get<glm::glmInLetter##vec4>(vData.DefaultValue)); \
                        switch (vData.D) \
                        { \
                            case Uniforms::VectorSizes::One: \
                                SetUniform(ptr, vec.x); \
                            break; \
                            case Uniforms::VectorSizes::Two: \
                                SetUniform(ptr, Math::Resize<2>(vec)); \
                            break; \
                            case Uniforms::VectorSizes::Three: \
                                SetUniform(ptr, Math::Resize<3>(vec)); \
                            break; \
                            case Uniforms::VectorSizes::Four: \
                                SetUniform(ptr, vec); \
                            break; \
                            default: \
                                BP_ASSERT_STR(false, \
                                              "Unknown Bplus::GL::Uniforms::VectorSizes::" + \
                                                  vData.D._to_string()); \
                                uniformPtrs[uName] = OglPtr::ShaderUniform::Null(); \
                            break; \
                        } \
                    } \
                    break

                VDATA_TYPE(Float, d, f);
                VDATA_TYPE(Double, d, d);
                VDATA_TYPE(Int, i64, i);
                VDATA_TYPE(UInt, i64, u);
                VDATA_TYPE(Bool, i64, b);
                #undef VDATA_TYPE
                
                default:
                    BP_ASSERT_STR(false,
                                    "Unknown Bplus::GL::Uniforms::ScalarTypes::" +
                                        vData.Type._to_string());
                    uniformPtrs[uName] = OglPtr::ShaderUniform::Null();
                break;
            }

            #pragma endregion
        }
        else if (std::holds_alternative<Uniforms::Matrix>(uType.ElementType))
        {
            #pragma region Matrix uniforms

            const auto& mData = std::get<Uniforms::Matrix>(uType.ElementType);

            //Switch statements are used below; this macro helps with them.
            #define MDATA_TYPE(code, sizeC, sizeR, oglMatType) \
                case code: { \
                    auto mSized = Math::Resize<sizeC, sizeR>(m); \
                    SetUniform(ptr, mSized); \
                }; break

            if (mData.IsDouble)
            {
                glm::dmat4x4 m(1.0);

                //Nested switch statements with proper "default: BP_ASSERT(false)" clauses
                //    are verbose and annoying, but I don't like doing it the "improper" way.
                //Turn a group of nested switch statements into a single switch statement
                //    by combining the row and column numbers.
                //E.x. "43" means a 4x3 matrix.
                switch ((int)(mData.Columns._to_integral() + (10 * mData.Rows._to_integral())))
                {
                    MDATA_TYPE(22, 2, 2, 2d);
                    MDATA_TYPE(23, 2, 3, 2x3d);
                    MDATA_TYPE(24, 2, 4, 2x4d);
                    MDATA_TYPE(32, 3, 2, 3x2d);
                    MDATA_TYPE(33, 3, 3, 3d);
                    MDATA_TYPE(34, 3, 4, 3x4d);
                    MDATA_TYPE(42, 4, 2, 4x2d);
                    MDATA_TYPE(43, 4, 3, 4x3d);
                    MDATA_TYPE(44, 4, 4, 4d);

                    default:
                        BP_ASSERT_STR(false,
                                      "Unknown matrix size: " + mData.Columns._to_string() +
                                          " columns and " + mData.Rows._to_string() + " rows");
                        uniformPtrs[uName] = OglPtr::ShaderUniform::Null();
                    break;
                }
            }
            else
            {
                glm::fmat4x4 m(1.0f);

                //Nested switch statements with proper "default: BP_ASSERT(false)" clauses
                //    are verbose and annoying, but I don't like doing it the "improper" way.
                //Turn a group of nested switch statements into a single switch statement
                //    by combining the row and column numbers.
                //E.x. "43" means a 4x3 matrix.
                switch ((int)(mData.Columns._to_integral() + (10 * mData.Rows._to_integral())))
                {
                    MDATA_TYPE(22, 2, 2, 2f);
                    MDATA_TYPE(23, 2, 3, 2x3f);
                    MDATA_TYPE(24, 2, 4, 2x4f);
                    MDATA_TYPE(32, 3, 2, 3x2f);
                    MDATA_TYPE(33, 3, 3, 3f);
                    MDATA_TYPE(34, 3, 4, 3x4f);
                    MDATA_TYPE(42, 4, 2, 4x2f);
                    MDATA_TYPE(43, 4, 3, 4x3f);
                    MDATA_TYPE(44, 4, 4, 4f);

                    default:
                        BP_ASSERT_STR(false,
                                      "Unknown matrix size: " + mData.Columns._to_string() +
                                          " columns and " + mData.Rows._to_string() + " rows");
                        uniformPtrs[uName] = OglPtr::ShaderUniform::Null();
                    break;
                }
            }

            #undef MDATA_TYPE

            #pragma endregion
        }
        else if (std::holds_alternative<Uniforms::Color>(uType.ElementType))
        {
            const auto& cData = std::get<Uniforms::Color>(uType.ElementType);
            uniformValues[ptr] = cData.Default;
            
            switch (cData.Channels)
            {
                case Textures::SimpleFormatComponents::R:
                    SetUniform(ptr, cData.Default.r);
                break;
                case Textures::SimpleFormatComponents::RG:
                    SetUniform(ptr, Math::Resize<2>(cData.Default));
                break;
                case Textures::SimpleFormatComponents::RGB:
                    SetUniform(ptr, Math::Resize<3>(cData.Default));
                break;
                case Textures::SimpleFormatComponents::RGBA:
                    SetUniform(ptr, cData.Default);
                break;

                default:
                    BP_ASSERT_STR(false,
                                  "Unknown color channels format " + cData.Channels._to_string());
                    uniformPtrs[uName] = OglPtr::ShaderUniform::Null();
                break;
            }
        }
        else if (std::holds_alternative<Uniforms::Gradient>(uType.ElementType))
        {
            const auto& gData = std::get<Uniforms::Gradient>(uType.ElementType);

            //TODO: Once we have a singleton for common textures, default to using a texture from there.
            SetUniform(ptr, OglPtr::View::Null());
        }
        else if (std::holds_alternative<Uniforms::TexSampler>(uType.ElementType))
        {
            const auto& tData = std::get<Uniforms::TexSampler>(uType.ElementType);
            
            //TODO: Once we have a singleton for common textures, default to using a texture from there.
            SetUniform(ptr, OglPtr::View::Null());
        }
        //TODO: Once we support buffers, we should provide default buffers filled with 0 values.
        else
        {
            BP_ASSERT_STR(false,
                          "Unexpected uniform type, index " +
                              std::to_string(uType.ElementType.index()));
            uniformPtrs[uName] = OglPtr::ShaderUniform::Null();
        }
    });
}
CompiledShader::~CompiledShader()
{
    if (!programHandle.IsNull())
    {
        glDeleteProgram(programHandle.Get());
        programHandle = OglPtr::ShaderProgram::Null();
    }
}

CompiledShader::CompiledShader(CompiledShader&& src)
    : programHandle(src.programHandle),
      uniformPtrs(std::move(src.uniformPtrs)),
      uniformValues(std::move(src.uniformValues))
{
    src.programHandle = OglPtr::ShaderProgram::Null();
}

CompiledShader::UniformAndStatus CompiledShader::CheckUniform(const std::string& name) const
{
    //Check whether the name exists.
    auto foundPtr = uniformPtrs.find(name);
    if (foundPtr == uniformPtrs.end())
        return { OglPtr::ShaderUniform::Null(), UniformStates::Missing };
    
    //Check whether the uniform actually exists in the shader program.
    auto ptr = foundPtr->second;
    if (ptr.IsNull())
        return { ptr, UniformStates::OptimizedOut };

    //Everything checks out!
    return { ptr, UniformStates::Exists };
}


bool CompiledShader::SetUniform_Dynamic(const std::string& name, const UniformSetData_t& value)
{
    std::visit(
        [&](auto&& val) { SetUniform(name, val); },
        value
    );
}
bool CompiledShader::SetUniformArrayElement_Dynamic(const std::string& name, int index,
                                                    const UniformSetData_t& value)
{
    std::visit(
        [&](auto&& val) { SetUniformArrayElement(name, index, value); },
        value
    );
}
bool CompiledShader::SetUniformArray_Dynamic(const std::string& name,
                                             const span<UniformSetData_t>& newValues,
                                             size_t uOffset)
{
    //Edge-case: an empty array.
    if (newValues.size() < 1)
        return true;

    //Learn the type of the elements.
    bool result;
    std::visit(
        [&](auto&& val) {
            //Copy the elements into a buffer of the exact type.
            thread_local std::vector<decltype(val)> dataBuffer;
            dataBuffer.reserve(newValues.size());
            for (const auto& dataVariant : newValues)
            {
                BP_ASSERT(std::holds_alternative<decltype(val)>(dataVariant),
                          "Trying to set a uniform array, using "
                              "an array of values of different types");
                dataBuffer.push_back(std::get<decltype(val)>(dataVariant));
            }
            //Pass the buffer into the type-safe version of this function.
            result = SetUniformArray(name, span(dataBuffer), uOffset);
        },
        newValues[0]
    );

    return result;
}