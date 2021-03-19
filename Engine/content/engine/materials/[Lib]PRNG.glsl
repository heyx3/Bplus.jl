//PRNG library takes inspiration from this:
//      http://burtleburtle.net/bob/rand/smallprng.html
//  and this:
//      https://stackoverflow.com/a/52207531


//The function H_PRNG_Hash() turns (u)int data into pseudo-random (u)int data.
//You can apply this successively to effectively get a PRNG object.
//  Example:
//    uint seed = ...;
//    seed = H_PRNG_Hash(seed);
//    [do something with 'seed']
//    seed = H_PRNG_Hash(seed);
//    [do something with 'seed']
//    etc.

uint  H_PRNG_Hash(uint);
uvec2 H_PRNG_Hash(uvec2);
uvec3 H_PRNG_Hash(uvec3);
uvec4 H_PRNG_Hash(uvec4);

int   H_PRNG_Hash(int);
ivec2 H_PRNG_Hash(ivec2);
ivec3 H_PRNG_Hash(ivec3);
ivec4 H_PRNG_Hash(ivec4);


//The function H_PRNG_Rand() turns N (u)ints into N random floats,
//    and advances the input values to the next pseudo-random values.
//    E.x. 
//      int seed = ...;
//      float r1 = H_PRNG_Rand(seed),
//            r2 = H_PRNG_Rand(seed),
//            r3 = H_PRNG_Rand(seed),
//            ... ;

float H_PRNG_Rand(inout uint);
vec2  H_PRNG_Rand(inout uvec2);
vec3  H_PRNG_Rand(inout uvec3);
vec4  H_PRNG_Rand(inout uvec4);

float H_PRNG_Rand(inout int);
vec2  H_PRNG_Rand(inout ivec2);
vec3  H_PRNG_Rand(inout ivec3);
vec4  H_PRNG_Rand(inout ivec4);


//There are other versions of the above functions which handle asymmetric input and output sizes.
//These versions put the size of the output at the end of their name.
//Hash uints:
uint H_PRNG_Hash1(uvec2 v) { return H_PRNG_Hash(v.x ^ v.y); }
uint H_PRNG_Hash1(uvec3 v) { return H_PRNG_Hash1(v.xy ^ v.z); }
uint H_PRNG_Hash1(uvec4 v) { return H_PRNG_Hash1(v.xy ^ v.zw); }
uvec2 H_PRNG_Hash2(uint u) { uint u1 = H_PRNG_Hash(u); return uvec2(u1, H_PRNG_Hash(u1)); }
uvec2 H_PRNG_Hash2(uvec3 v) { return H_PRNG_Hash(v.xy ^ v.z); }
uvec2 H_PRNG_Hash2(uvec4 v) { return H_PRNG_Hash(v.xy ^ v.zw); }
uvec3 H_PRNG_Hash3(uint u) { uint u1 = H_PRNG_Hash(u), u2 = H_PRNG_Hash(u1); return uvec3(u1, u2, H_PRNG_Hash(u2)); }
uvec3 H_PRNG_Hash3(uvec2 v) { return H_PRNG_Hash(uvec3(v.xy, v.x * v.y)); }
uvec3 H_PRNG_Hash3(uvec4 v) { return H_PRNG_Hash(v.xyz ^ v.w); }
uvec4 H_PRNG_Hash4(uint u) { uint u1 = H_PRNG_Hash(u), u2 = H_PRNG_Hash(u1), u3 = H_PRNG_Hash(u2); return uvec4(u1, u2, u3, H_PRNG_Hash(u3)); }
uvec4 H_PRNG_Hash4(uvec2 v) { return H_PRNG_Hash(uvec3(v.xy, v.x + v.y, v.x ^ (~v.y))); }
uvec4 H_PRNG_Hash4(uvec3 v) { return H_PRNG_Hash(v.xyz ^ v.w); }
//Hash ints:
int H_PRNG_Hash1(ivec2 v) { return H_PRNG_Hash(v.x ^ v.y); }
int H_PRNG_Hash1(ivec3 v) { return H_PRNG_Hash1(v.xy ^ v.z); }
int H_PRNG_Hash1(ivec4 v) { return H_PRNG_Hash1(v.xy ^ v.zw); }
ivec2 H_PRNG_Hash2(int i) { int i1 = H_PRNG_Hash(i); return ivec2(i1, H_PRNG_Hash(i1)); }
ivec2 H_PRNG_Hash2(ivec3 v) { return H_PRNG_Hash(v.xy ^ v.z); }
ivec2 H_PRNG_Hash2(ivec4 v) { return H_PRNG_Hash(v.xy ^ v.zw); }
ivec3 H_PRNG_Hash3(int i) { int i1 = H_PRNG_Hash(i), i2 = H_PRNG_Hash(i1); return ivec3(i1, i2, H_PRNG_Hash(i2)); }
ivec3 H_PRNG_Hash3(ivec2 v) { return H_PRNG_Hash(ivec3(v.xy, v.x * v.y)); }
ivec3 H_PRNG_Hash3(ivec4 v) { return H_PRNG_Hash(v.xyz ^ v.w); }
ivec4 H_PRNG_Hash4(int i) { int i1 = H_PRNG_Hash(i), i2 = H_PRNG_Hash(i1), i3 = H_PRNG_Hash(i2); return ivec4(i1, i2, i3, H_PRNG_Hash(i3)); }
ivec4 H_PRNG_Hash4(ivec2 v) { return H_PRNG_Hash(ivec3(v.xy, v.x + v.y, v.x ^ (~v.y))); }
ivec4 H_PRNG_Hash4(ivec3 v) { return H_PRNG_Hash(v.xyz ^ v.w); }
//Rand:
float H_PRNG_Rand1(uvec2 v) { return H_PRNG_Rand(H_PRNG_Hash1(v)); }
float H_PRNG_Rand1(uvec3 v) { return H_PRNG_Rand(H_PRNG_Hash1(v)); }
float H_PRNG_Rand1(uvec4 v) { return H_PRNG_Rand(H_PRNG_Hash1(v)); }
vec2 H_PRNG_Rand2(uint u) { return H_PRNG_Rand(H_PRNG_Hash2(u)); }
vec2 H_PRNG_Rand2(uvec3 u) { return H_PRNG_Rand(H_PRNG_Hash2(v)); }
vec2 H_PRNG_Rand2(uvec4 u) { return H_PRNG_Rand(H_PRNG_Hash2(v)); }
vec3 H_PRNG_Rand3(uint u) { return H_PRNG_Rand(H_PRNG_Hash3(u)); }
vec3 H_PRNG_Rand3(uvec2 u) { return H_PRNG_Rand(H_PRNG_Hash3(v)); }
vec3 H_PRNG_Rand3(uvec4 u) { return H_PRNG_Rand(H_PRNG_Hash3(v)); }
vec4 H_PRNG_Rand4(uint u) { return H_PRNG_Rand(H_PRNG_Hash4(u)); }
vec4 H_PRNG_Rand4(uvec2 u) { return H_PRNG_Rand(H_PRNG_Hash4(v)); }
vec4 H_PRNG_Rand4(uvec3 u) { return H_PRNG_Rand(H_PRNG_Hash4(v)); }
float H_PRNG_Rand1(ivec2 v) { return H_PRNG_Rand(H_PRNG_Hash1(v)); }
float H_PRNG_Rand1(ivec3 v) { return H_PRNG_Rand(H_PRNG_Hash1(v)); }
float H_PRNG_Rand1(ivec4 v) { return H_PRNG_Rand(H_PRNG_Hash1(v)); }
vec2 H_PRNG_Rand2(int i) { return H_PRNG_Rand(H_PRNG_Hash2(i)); }
vec2 H_PRNG_Rand2(ivec3 i) { return H_PRNG_Rand(H_PRNG_Hash2(v)); }
vec2 H_PRNG_Rand2(ivec4 i) { return H_PRNG_Rand(H_PRNG_Hash2(v)); }
vec3 H_PRNG_Rand3(int i) { return H_PRNG_Rand(H_PRNG_Hash3(i)); }
vec3 H_PRNG_Rand3(ivec2 i) { return H_PRNG_Rand(H_PRNG_Hash3(v)); }
vec3 H_PRNG_Rand3(ivec4 i) { return H_PRNG_Rand(H_PRNG_Hash3(v)); }
vec4 H_PRNG_Rand4(int i) { return H_PRNG_Rand(H_PRNG_Hash4(i)); }
vec4 H_PRNG_Rand4(ivec2 i) { return H_PRNG_Rand(H_PRNG_Hash4(v)); }
vec4 H_PRNG_Rand4(ivec3 i) { return H_PRNG_Rand(H_PRNG_Hash4(v)); }



//========================================
//====       PRNG implementation      ====
//========================================


//Helper functions for mixing an integer's bits:

const uvec3 C_PRNG_ROTu = uvec3(1103515245U,
                                0020170906U,
                                0001664525U,
                                0134775813U);
const ivec3 C_PRNG_ROTi = ivec3(1103515245,
                                0020170906,
                                0001664525,
                                0134775813);

const uint C_PRNG_ROT1u = 27U;
const int C_PRNG_ROT1i = 27;

uint H_PRNG_ROT(uint u) { return H_PRNG_ROT(u, C_PRNG_ROT1u); }
uint H_PRNG_Rot(uint u, uint amount)
{
    return (u << amount) |
           (u >> (32 - amount));
}
uvec2 H_PRNG_Rot(uvec2 u)
{
    const uvec2 K = uvec2(1103515245U,
                          0020170906U);

    u = ((u >> 8U) ^ u.yx) * K;
}
uvec3 H_PRNG_Rot(uvec3 u)
{
    const uvec3 K = uvec3(1103515245U,
                          0020170906U,
                          0134775813U);

    u = ((u >> 8U) ^ u.yzx) * K;
}
uvec4 H_PRNG_Rot(uvec4 u)
{
    const uvec4 K = uvec4(1103515245U,
                          0020170906U,
                          0001664525U,
                          0134775813U);

    u = ((u >> 8U) ^ u.yzwx) * K;
}

int H_PRNG_ROT(int i) { return H_PRNG_ROT(i, C_PRNG_ROT1i); }
int H_PRNG_Rot(int i, int amount)
{
    return (i << amount) |
           (i >> (32 - amount));
}
ivec2 H_PRNG_Rot(ivec2 i)
{
    const ivec2 K = ivec2(1103515245,
                          0020170906);

    i = ((i >> 8U) ^ i.yx) * K;
}
ivec3 H_PRNG_Rot(ivec3 i)
{
    const ivec3 K = ivec3(1103515245,
                          0020170906,
                          0134775813);

    i = ((i >> 8U) ^ i.yzx) * K;
}
ivec4 H_PRNG_Rot(ivec4 i)
{
    const ivec4 K = ivec4(1103515245,
                          0020170906,
                          0001664525,
                          0134775813);

    i = ((i >> 8U) ^ i.yzwx) * K;
}


//Define H_PRNG_Hash overloads with a macro.
#define H_PRNG_DEF_HASH(T) \
    T H_PRNG_Hash(T ints) { \
        for (int i = 0; i < 3; ++i) \
            ints = H_PRNG_Rot(ints); \
        return ints; \
    }
H_PRNG_DEF_HASH(uint)
H_PRNG_DEF_HASH(uvec2)
H_PRNG_DEF_HASH(uvec3)
H_PRNG_DEF_HASH(uvec4)
H_PRNG_DEF_HASH(iint)
H_PRNG_DEF_HASH(ivec2)
H_PRNG_DEF_HASH(ivec3)
H_PRNG_DEF_HASH(ivec4)
#undef H_PRNG_DEF_HASH



//Define H_PRNG_Rand overloads with a macro.
#define H_PRNG_DEF_RAND(TIn, TOut) \
    TOut H_PRNG_Rand(inout TIn rngState) \
    { \
        /* Use floating-point tricks to turn the bits into a float between 1 and 2. */ \
        uint header =  0x3f800000U, \
             rngMask = 0x007fffffU; \
        rngState = H_PRNG_Hash(rngState); \
        float f_1_2 = uintBitsToFloat(header | (rngState & rngMask)); \
        /* Subtract 1 to get a [0, 1) range of values. */ \
        return f_1_2 - 1.0; \
    }
H_PRNG_DEF_RAND(uint, float)
H_PRNG_DEF_RAND(uvec2, vec2)
H_PRNG_DEF_RAND(uvec3, vec3)
H_PRNG_DEF_RAND(uvec4, vec4)
#undef H_PRNG_DEF_RAND
#define H_PRNG_DEF_RAND(TIn, TOut) \
    TOut H_PRNG_Rand(inout TIn rngState) \
    { \
        /* Use floating-point tricks to turn the bits into a float between 1 and 2. */ \
        int header =  0x3f800000, \
            rngMask = 0x007fffff; \
        rngState = H_PRNG_Hash(rngState); \
        float f_1_2 = intBitsToFloat(header | (rngState & rngMask)); \
        /* Subtract 1 to get a [0, 1) range of values. */ \
        return f_1_2 - 1.0; \
    }
H_PRNG_DEF_RAND(int, float)
H_PRNG_DEF_RAND(vec2, vec2)
H_PRNG_DEF_RAND(vec3, vec3)
H_PRNG_DEF_RAND(vec4, vec4)
#undef H_PRNG_DEF_RAND


//======================================
//======================================