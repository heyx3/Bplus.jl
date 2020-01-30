#pragma once

#include "../Context.h"

//"Uniforms" are shader parameters.


namespace Bplus::GL
{
    //I spent a long time working on the CPU-side representation of uniforms,
    //    mainly because I wanted to postpone actually giving them to OpenGL until
    //    the relevant shader program gets bound for rendering (to greatly reduce driver overhead).
    //However, I later found out that this is unnecessary with OpenGL 4.5,
    //    thanks to the glProgramUniform[X] functions.

    #pragma region Old Uniform data representation
    #if 0

    //All the different types of uniforms.
    BETTER_ENUM(UniformTypes, uint_fast8_t,
        //Vector types:
        Bool1,   Bool2,   Bool3,   Bool4,
        Uint1,   Uint2,   Uint3,   Uint4,
        Int1,    Int2,    Int3,    Int4,
        Float1,  Float2,  Float3,  Float4,
        Double1, Double2, Double3, Double4,

        //Matrices (based on OpenGL -- first number is columns, second rows):
        Float2x2, Double2x2, Float2x3, Double2x3, Float2x4, Double2x4,
        Float3x2, Double3x2, Float3x3, Double3x3, Float3x4, Double3x4,
        Float4x2, Double4x2, Float4x3, Double4x3, Float4x4, Double4x4,

        //A texture plus sampling settings.
        Sampler,
        //A plain texture; you can read (and sometimes write) pixels but
        //    can't "sample" from it
        Image
    );

    //Gets the enum value for the corresponding uniform data type.
    template<typename T>
    UniformTypes GetUniformType()
    {
        //This "if" statement exists so that all subsequent lines can use "else",
        //    to simplify the macros.
        if (false) return UniformTypes::_from_index(-1);

    #define CASE(T2, val) \
            else if constexpr (std::is_same_v<T, T2>) return UniformTypes::val;

    #define CASE_VECTORS(T2, val) \
            CASE(T2, val##1) \
            CASE(glm::vec<1 BP_COMMA T2>, val##1) \
            CASE(glm::vec<2 BP_COMMA T2>, val##2) \
            CASE(glm::vec<3 BP_COMMA T2>, val##3) \
            CASE(glm::vec<4 BP_COMMA T2>, val##4)
        CASE_VECTORS(bool, Bool)
        CASE_VECTORS(int32_t, Int)
        CASE_VECTORS(uint32_t, Uint)
        CASE_VECTORS(float, Float)
        CASE_VECTORS(double, Double)
    #undef CASE_VECTORS


    #define CASE_MATRIX(nCols, nRows) \
            CASE(glm::mat<nCols BP_COMMA nRows BP_COMMA float>, Float##nCols##x##nRows) \
            CASE(glm::mat<nCols BP_COMMA nRows BP_COMMA double>, Double##nCols##x##nRows)
    #define CASE_MATRICES(nCols) \
            CASE_MATRIX(nCols, 2) CASE_MATRIX(nCols, 3) CASE_MATRIX(nCols, 4)
        CASE_MATRICES(2) CASE_MATRICES(3) CASE_MATRICES(4)
    #undef CASE_MATRICES
    #undef CASE_MATRIX

        CASE(UniformDataStructures::SamplerUniformHandle, Sampler)
        CASE(UniformDataStructures::ImageUniformHandle, Image)

    #undef CASE

        //If none of the previous cases apply, then an invalid type was passed.
        else { static_assert(false); return UniformTypes::_from_index(-1); }
    }

    //Gets the size of the given uniform type.
    //NOTE: booleans are stored as integers for OpenGL purposes,
    //    so they're generally more than 1 byte.
    uint32_t GetUniformByteSize(UniformTypes type)
    {
        switch (type)
        {
            //Bool vectors require special handling.
            case UniformTypes::Bool1:
                return sizeof(int32_t);
            case UniformTypes::Bool2:
                return 2 * sizeof(int32_t);
            case UniformTypes::Bool3:
                return 3 * sizeof(int32_t);
            case UniformTypes::Bool4:
                return 4 * sizeof(int32_t);

             //Handle the non-boolean vector types.
        #define CASE_VECTOR(Type, data) \
            case UniformTypes::Type##1: \
                return sizeof(glm::data##1); \
            case UniformTypes::Type##2: \
                return sizeof(glm::data##2); \
            case UniformTypes::Type##3: \
                return sizeof(glm::data##3); \
            case UniformTypes::Type##4: \
                return sizeof(glm::data##4)

            CASE_VECTOR(Int, ivec);
            CASE_VECTOR(Uint, uvec);
            CASE_VECTOR(Float, fvec);
            CASE_VECTOR(Double, dvec);
        #undef CASE_VECTOR

            //Handle the matrix types.
        #define CASE_MATRIX(nRows, nCols) \
            case UniformTypes::Float##nCols##x##nRows: \
                return sizeof(glm::fmat##nCols##x##nRows); \
            case UniformTypes::Double##nCols##x##nRows: \
                return sizeof(glm::dmat##nCols##x##nRows)
        #define CASE_MATRICES(nRows) \
            CASE_MATRIX(nRows, 2); \
            CASE_MATRIX(nRows, 3); \
            CASE_MATRIX(nRows, 4)

            CASE_MATRICES(2);
            CASE_MATRICES(3);
            CASE_MATRICES(4);

        #undef CASE_MATRICES
        #undef CASE_MATRIX

            //Handle the texture/image types.
            case UniformTypes::Sampler:
                return sizeof(Ptr::Sampler);
            case UniformTypes::Image:
                return sizeof(Ptr::Image);

            //Error for any other types we forgot to handle.
            default:
                BPAssert(false,
                         (std::string("Unknown uniform type: ") + type._to_string()).c_str());
        }
    }


    //A single uniform value of any type.
    struct BP_API UniformValue
    {
        #pragma region The union
        union {
            bool B;
            uint32_t U;
            int32_t I;
            float F;
            double D;

            //Macro to define the vector types.
            //Note that there are 1D vectors, which represent the exact same data
            //    as the above primitive types.
        #define MAKE_OF_SIZE(n) \
            glm::vec<n, bool> B##n; \
            glm::vec<n, uint32_t> U##n; \
            glm::vec<n, int32_t> I##n; \
            glm::vec<n, float> F##n; \
            glm::vec<n, double> D##n

            MAKE_OF_SIZE(1);
            MAKE_OF_SIZE(2);
            MAKE_OF_SIZE(3);
            MAKE_OF_SIZE(4);
        #undef MAKE_OF_SIZE

            //Macro to define the matrix types.
            //Names are in the form "F[C]x[R]" and "D[C]x[R]",
            //    where C is the column count and R is the row count.
        #define GLM_MAT(n, m) \
            glm::mat<n, m, glm::f32> F##n##x##m; \
            glm::mat<n, m, glm::f64> D##n##x##m
        #define GLM_MATS(n) \
            GLM_MAT(n, 2); GLM_MAT(n, 3); GLM_MAT(n, 4)

            GLM_MATS(2);
            GLM_MATS(3);
            GLM_MATS(4);
        #undef GLM_MATS
        #undef GLM_MAT
               
            Ptr::Sampler Sampler;
            Ptr::Image Image;

        } Value;
        #pragma endregion
        
        #pragma region Casting/getting, assignment, and constructors

        template<typename Value_t>
        const Value_t& As() const { static_assert(false); } //Specialized for applicable types.

        template<typename Value_t>
        Value_t& As()
        {
            //Call the const version, and then just cast.
            const auto& constThis = *this;
            const auto& constVal = constThis.As<Value_t>();
            return const_cast<Value_t&>(constVal);
        }


        //Define cast operator, constructor, assignment operator, and "As()" for each type.
    #define SUPPORT(t, field, enumType) \
        operator t() const { return Value.field; } \
        UniformValue(const t& val): type(UniformTypes::enumType) { Value.field = val; } \
        UniformValue& operator=(const t& val) { Value.field = val; type = UniformTypes::enumType; return *this; } \
        template<> const t& As<t>() const { BPAssert(type == +UniformTypes::enumType, \
                                                     (std::string("Expected ") + (+UniformTypes::enumType)._to_string() + \
                                                        " but got " + type._to_string()).c_str()); \
                                            return Value.field; }

        //Define the basic data types.
        SUPPORT(bool, B, Bool1)
        SUPPORT(int32_t, I, Int1)
        SUPPORT(uint32_t, U, Uint1)
        SUPPORT(float, F, Float1)
        SUPPORT(double, D, Double1)

        //Define the vector types.
    #define SUPPORT_VECTORS(t, field, typePrefix) \
        SUPPORT(glm::vec<1 BP_COMMA t>, field##1, typePrefix##1) \
        SUPPORT(glm::vec<2 BP_COMMA t>, field##2, typePrefix##2) \
        SUPPORT(glm::vec<3 BP_COMMA t>, field##3, typePrefix##3) \
        SUPPORT(glm::vec<4 BP_COMMA t>, field##4, typePrefix##4)

        SUPPORT_VECTORS(bool, B, Bool)
        SUPPORT_VECTORS(int32_t, I, Int)
        SUPPORT_VECTORS(uint32_t, U, Uint)
        SUPPORT_VECTORS(float, F, Float)
        SUPPORT_VECTORS(double, D, Double)
    #undef SUPPORT_VECTORS

        //Define the matrix types.
    #define SUPPORT_ALL_FTYPES(nCols, nRows) \
        SUPPORT(glm::mat<nCols BP_COMMA nRows BP_COMMA float>, \
                F##nCols##x##nRows, Float##nCols##x##nRows) \
        SUPPORT(glm::mat<nCols BP_COMMA nRows BP_COMMA double>, \
                D##nCols##x##nRows, Double##nCols##x##nRows)
    #define SUPPORT_ALL_ROWS(nCols) \
        SUPPORT_ALL_FTYPES(nCols, 2) \
        SUPPORT_ALL_FTYPES(nCols, 3) \
        SUPPORT_ALL_FTYPES(nCols, 4)

        SUPPORT_ALL_ROWS(2)
        SUPPORT_ALL_ROWS(3)
        SUPPORT_ALL_ROWS(4)
    #undef SUPPORT_ALL_ROWS
    #undef SUPPORT_ALL_FTYPES
    
        SUPPORT(Ptr::Sampler, Sampler, Sampler);
        SUPPORT(Ptr::Image, Image, Image);

    #undef SUPPORT

        #pragma endregion

        UniformValue() = default;

    private:

        UniformTypes type;
    };


    //An array of some kind of uniform value.
    class BP_API UniformArray
    {
    public:

        UniformArray(UniformTypes _type) : type(_type) { }

        //Constructor with a single value.
        template<typename Value_t>
        UniformArray(const Value_t& singleValue)
            : this(GetUniformType<Value_t>()), count(1)
        {
            arrayBytes.resize(GetUniformByteSize(type));
            *(Value_t*)arrayBytes.data() = singleValue;
        }

        //Constructor with an array of values.
        template<typename Value_t>
        UniformArray(const Value_t* arrayValues, size_t _count)
            : this(GetUniformType<Value_t>()), count(_count)
        {
            arrayBytes.resize(count * GetUniformByteSize(type));
            memcpy(arrayBytes.data(), arrayValues, arrayBytes.size());
        }


        #pragma region Getters

        UniformTypes GetType() const { return type; }
        size_t GetCount() const { return count; }

        const std::byte* GetData() const { return arrayBytes.data(); }
              std::byte* GetData()       { return arrayBytes.data(); }

        template<typename Value_t>
        const Value_t* GetData() const { AssertType<Value_t>(); return (Value_t*)GetData(); }
        template<typename Value_t>
              Value_t* GetData()       { AssertType<Value_t>(); return (Value_t*)GetData(); }

        template<typename Value_t>
        const Value_t& Get(size_t index) const { AssertType<Value_t>(); AssertIndex(index); return GetData<Value_t>()[index]; }
        template<typename Value_t>
              Value_t& Get(size_t index)       { AssertType<Value_t>(); AssertIndex(index); return GetData<Value_t>()[index]; }
        
        #pragma endregion

        #pragma region Setters

        template<typename Value_t>
        void Set(size_t index, const Value_t& newValue)
        {
            AssertType<Value_t>();
            AssertIndex(index);
            GetData<Value_t>()[index] = newValue;
        }

        template<typename Value_t>
        void Add(const Value_t& val)
        {
            AssertType<Value_t>();
            
            arrayBytes.resize(arrayBytes.size() + GetUniformByteSize(type));
            Get<Value_t>()[count] = val;

            count += 1;
        }

        void Remove(size_t index);

        void Clear(UniformTypes newType);
        void Clear() { Clear(type); }

        //Clears this list AND changes its type to the given one.
        template<typename Value_t>
        void Clear() { Clear(GetUniformType<Value_t>()); }

        #pragma endregion

        //TODO: A non-template version of Get, Set, and Add which use the UniformValue class


    private:

        UniformTypes type;
        size_t count;
        std::vector<std::byte> arrayBytes;


        void AssertIndex(size_t i) const;

        void AssertType(UniformTypes expected) const;

        template<typename Value_t>
        void AssertType() const { AssertType(GetUniformType<Value_t>(); }
    };
    
    //void SetUniform(Ptr::ShaderUniform ptr, const VectorUniform& value);
    //void SetUniform(Ptr::ShaderUniform ptr, const MatrixUniform& value);
    //void SetUniform(Ptr::ShaderUniform ptr, const TextureUniform& value);

    #endif
    #pragma endregion
}