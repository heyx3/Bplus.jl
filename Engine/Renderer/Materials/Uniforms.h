#pragma once

#include "../Context.h"

//"Uniforms" are shader parameters.


namespace Bplus::GL
{
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


    #pragma region Uniform unions
    //Hide as many of the data structures as possible in an internal namespace
    //    to simplify the outer namespace.
    namespace UniformDataStructures
    {
        //Samplers and images have the same handle type, so use strong_typedef to differentiate them.
        strong_typedef(SamplerUniformHandle, GLuint, BP_API);
        strong_typedef(ImageUniformHandle, GLuint, BP_API);


        //A union of the various 1D - 4D vector types.
    //Wrapped in a struct to provide implicit casting and easy assignment.
        struct BP_API UniformUnion_Vector
        {
            #pragma region The union
            union {
                //The 1-dimensional types are given a second, simpler mode of access;
                //    GLM doesn't have implicit casting from vec1<T> to T.
                bool B;
                uint32_t U;
                int32_t I;
                float F;
                double D;

                //Macro to define the higher-dimensional data types.
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
        #define SUPPORT(t, field) \
                operator t() const { return Value.field; } \
                UniformUnion_Vector(const t& val) { Value.field = val; } \
                UniformUnion_Vector& operator=(const t& val) { Value.field = val; return *this; } \
                template<> const t& As<t>() const { return Value.field; }

            //Define the basic data types.
            SUPPORT(bool, B)
            SUPPORT(int32_t, I)
            SUPPORT(uint32_t, U)
            SUPPORT(float, F)
            SUPPORT(double, D)

        #define SUPPORT_VECTORS(t, field) \
                SUPPORT(glm::vec<1 BP_COMMA t>, field##1) \
                SUPPORT(glm::vec<2 BP_COMMA t>, field##2) \
                SUPPORT(glm::vec<3 BP_COMMA t>, field##3) \
                SUPPORT(glm::vec<4 BP_COMMA t>, field##4)

            SUPPORT_VECTORS(bool, B)
            SUPPORT_VECTORS(int32_t, I)
            SUPPORT_VECTORS(uint32_t, U)
            SUPPORT_VECTORS(float, F)
            SUPPORT_VECTORS(double, D)

            #undef SUPPORT_VECTORS
            #undef SUPPORT

    #pragma endregion

            UniformUnion_Vector() = default;
            static bool IsValidType(UniformTypes type);
        };

    
        //A union of the various 2x2 to 4x4 matrix types
        //    (including rectangular sizes).
        //Wrapped in a struct to provide implicit casting and easy assignment.
        struct BP_API UniformUnion_Matrix
        {
            #pragma region The union
            union {
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
        #define SUPPORT(t, field) \
                operator t() const { return Value.field; } \
                UniformUnion_Matrix(const t& val) { Value.field = val; } \
                UniformUnion_Matrix& operator=(const t& val) { Value.field = val; return *this; } \
                template<> const t& As<t>() const { return Value.field; }

        #define SUPPORT_ALL_FTYPES(nCols, nRows) \
                SUPPORT(glm::mat<nCols BP_COMMA nRows BP_COMMA float>, \
                        F##nCols##x##nRows) \
                SUPPORT(glm::mat<nCols BP_COMMA nRows BP_COMMA double>, \
                        D##nCols##x##nRows)

        #define SUPPORT_ALL_ROWS(nCols) \
                SUPPORT_ALL_FTYPES(nCols, 2) \
                SUPPORT_ALL_FTYPES(nCols, 3) \
                SUPPORT_ALL_FTYPES(nCols, 4)

            SUPPORT_ALL_ROWS(2)
                SUPPORT_ALL_ROWS(3)
                SUPPORT_ALL_ROWS(4)

    #undef SUPPORT_ALL_ROWS
    #undef SUPPORT_ALL_FTYPES
    #undef SUPPORT

    #pragma endregion

            UniformUnion_Matrix() = default;
            static bool IsValidType(UniformTypes type);
        };

        //A union of the various texture types.
        //Wrapped in a struct to provide implicit casting and easy assignment.
        struct BP_API UniformUnion_Texture
        {
            #pragma region The union
            union Types{
                SamplerUniformHandle Sampler;
                ImageUniformHandle Image;
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
        #define SUPPORT(t, field) \
                operator t() const { return Value.field; } \
                UniformUnion_Texture(const t& val) { Value.field = val; } \
                UniformUnion_Texture& operator=(const t& val) { Value.field = val; return *this; } \
                template<> const t& As<t>() const { return Value.field; }
            SUPPORT(SamplerUniformHandle, Sampler)
            SUPPORT(ImageUniformHandle, Image)
        #undef SUPPORT

    #pragma endregion
            
            UniformUnion_Texture() = default;
            static bool IsValidType(UniformTypes type);
        };
    }

    #pragma endregion


    //A value, or array of values, to be passed into a shader.
    //If there's only one value, no heap allocations will be made.
    template<typename ValueUnion_t,
             UniformTypes::_integral _InvalidType>
    class Uniform
    {
    public:

        static const UniformTypes InvalidType;
        using This_t = Uniform<ValueUnion_t, _InvalidType>;


        Uniform() : type(InvalidType), count(0)
        {
            //Technically the C++ standard doesn't guarantee that std::vector's default constructor
            //    makes no heap allocations, but from what I understand, it never will in practice.
            //If this assert fails, you'll need to switch over to a raw array.
            assert(arrayValue.capacity() < 1);
        }

        //Constructor with the uniform's value.
        template<typename Value_t>
        Uniform(const Value_t& _singleValue)
            : Uniform() //Make sure the assert() gets called.
        {
            type = GetUniformType<Value_t>();
            singleValue = _singleValue;
            count = 1;
        }

        //Assignment operator with the uniform's value.
        template<typename Value_t>
        Uniform& operator=(const Value_t& _singleValue)
        {
            if (arrayValue.capacity() > 0)
                arrayValue.clear();

            type = GetUniformType<Value_t>();
            singleValue = _singleValue;
            count = 1;

            return *this;
        }


        #pragma region Size/index getters and setters

        size_t GetCount() const { return count; }

        const ValueUnion_t& Get(size_t index = 0) const
        {
            assert(index < count);
            if ((index == 0) & (arrayValue.size() < 1))
                return singleValue;
            else
                return arrayValue[index];
        }
        ValueUnion_t& Get(size_t index = 0)
        {
            //Just call the const version and cast it.
            const auto& constThis = *this;
            const auto& constVal = constThis.Get(index);
            return const_cast<ValueUnion_t&>(constVal);
        }

        template<typename Value_t>
        const Value_t& Get(size_t index = 0) const
        {
            assert(type == GetUniformType<Value_t>());
            return Get(index).As<Value_t>();
        }
        template<typename Value_t>
        Value_t& Get(size_t index = 0)
        {
            //Just call the const version and cast it.
            const auto& constThis = *this;
            const auto& constVal = constThis.Get<Value_t>(index);
            return const_cast<Value_t&>(constVal);
        }

        void Set(size_t index, const ValueUnion_t& newValue)
        {
            assert(index < GetCount());
            if (GetCount() == 1 && arrayValue.size() == 0)
                singleValue = newValue;
            else
                arrayValue[index] = newValue;
        }

        template<typename Value_t>
        void Set(size_t index, const Value_t& newValue)
        {
            assert(type == GetUniformType<Value_t>());
            Set(index, ValueUnion_t{ newValue });
        }

        #pragma endregion

        #pragma region Type data getters/setters

        bool HasAType() const { return type != InvalidType; }

        UniformTypes GetType() const { return type; }
        void SetType(UniformTypes t)
        {
            //Changing the type is only allowed if no items are in the array yet.
            assert(type == InvalidType ||
                   type == t ||
                   GetCount() == 0);

            type = t;
        }

        template<typename Value_t>
        void SetType() { SetType(GetUniformType<Value_t>()); }

        #pragma endregion

        #pragma region Add/Insert/Remove/Clear

        void Add(const ValueUnion_t& val)
        {
            if (count == 0)
            {
                count = 1;
                singleValue = val;
            }
            else if (count == 1)
            {
                //If we're still caching the single value,
                //    move it into the heap.
                if (arrayValue.size() == 0)
                    arrayValue.push_back(singleValue);

                arrayValue.push_back(val);
                count = 2;
            }
            else
            {
                arrayValue.push_back(val);
                count += 1;
            }

            assert((count == arrayValue.size()) ||
                   (count < 2));
        }
        void Remove(size_t index)
        {
            assert(index < count); //Both are unsigned, so this implies count > 0 as well.

            if (count == 1)
                count = 0;
            else
            {
                arrayValue.erase(arrayValue.begin() + index);
                count -= 1;
            }
        }

        template<typename Value_t>
        void Add(const Value_t& val)
        {
            SetType<Value_t>();
            Add(ValueUnion_t{ val });
        }

        void Clear()
        {
            count = 0;
            if (arrayValue.size() > 0)
                arrayValue.clear();
        }

        //Clears this list AND changes its type to the given one.
        template<typename Value_t>
        void Clear()
        {
            Clear();
            SetType<Value_t>();
        }

        #pragma endregion

    private:

        UniformTypes type = InvalidType;
        size_t count = 0;

        ValueUnion_t singleValue;
        std::vector<ValueUnion_t> arrayValue;
    };

    template<typename ValueUnion_t,
             UniformTypes::_integral _InvalidType>
    const UniformTypes Uniform<ValueUnion_t, _InvalidType>::InvalidType =
        UniformTypes::_from_integral(_InvalidType);


    using VectorUniform = Uniform<UniformDataStructures::UniformUnion_Vector,
                                  UniformTypes::Image>;
    using MatrixUniform = Uniform<UniformDataStructures::UniformUnion_Matrix,
                                  UniformTypes::Image>;
    using TextureUniform = Uniform<UniformDataStructures::UniformUnion_Texture,
                                   UniformTypes::Bool1>;
}