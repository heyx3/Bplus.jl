#pragma once

#include "../Platform.h"

#include "BPAssert.h"
#include <unordered_map>


//This helper macro escapes commas inside other macro calls.
#define BP_COMMA ,


namespace Bplus
{
    //is_std_hashable_v : does a type T implement std::hash<T>?
    //Taken from https://stackoverflow.com/questions/12753997/check-if-type-is-hashable

    template<typename T, typename = std::void_t<>>
    struct is_std_hashable : std::false_type { };

    template<typename T>
    struct is_std_hashable<T, std::void_t<decltype(std::declval<std::hash<T>>()(std::declval<T>()))>> : std::true_type { };

    template<typename T>
    constexpr bool is_std_hashable_v = is_std_hashable<T>::value;


    //MultiHash: hash any number of variables together,
    //    as long as they each implement std::hash<T>.
    
    inline size_t CombineHash(size_t in1, size_t in2)
    {
        //Taken from: https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
        return in1 ^ (in2 + 0x9e3779b9 + (in1 << 6) + (in1 >> 2));
    }

    //Recursive function with one parameter:
    template<typename LastItem>
    size_t MultiHash(const LastItem& item)
    {
        static_assert(is_std_hashable_v<LastItem>, "Data type isn't hashable");

        using std::hash;
        return hash<LastItem>{}(item);
    }

    //Recursive function with multiple parameters:
    template<typename T, typename... Rest>
    size_t MultiHash(const T& v, const Rest&... rest)
    {
        static_assert(is_std_hashable_v<T>, "Data type isn't hashable");

        using std::hash;
        return CombineHash(hash<T>{}(v),
                           MultiHash(rest...));
    }
}


//TODO: Change the naming convention "..._START_FULL" to "..._TEMPL_START" (e.x. BP_HASHABLE_START_FULL to BP_HASHABLE_TEMPL_START).

#pragma region Macros for easy std::hash specialization

//A helper macro for adding std::hash<> for some templated type.
//The type can have partial or full specialization.
//NOTE: this macro must be invoked in the global namespace!
#define BP_HASHABLE_START_FULL(OuterTemplate, Type) \
    namespace std { \
        template<OuterTemplate> \
        struct hash<Type> { \
            size_t operator()(const Type& d) const {

#define BP_HASHABLE_END \
            } \
        }; \
    }

//A helper macro for adding std::hash<> for some concrete type.
//NOTE: this macro must be invoked in the global namespace!
#define BP_HASHABLE_START(Type) BP_HASHABLE_START_FULL(, Type)


//Defines the hashing of a templated type, by
//    hashing all its given fields.
//This type can have partial or full specialization.
#define BP_HASHABLE_SIMPLE_FULL(OuterTemplate, Type, ...) \
    BP_HASHABLE_START_FULL(OuterTemplate, Type) \
        return Bplus::MultiHash(__VA_ARGS__); \
    BP_HASHABLE_END

//Defines the hashing of a concrete type, by
//    hashing all its given fields.
#define BP_HASHABLE_SIMPLE(Type, ...) BP_HASHABLE_SIMPLE_FULL(, Type, __VA_ARGS__)

#pragma endregion

#pragma region Macros for easy equality operators

//Defines operator!= for the given templated type, then starts operator== for you to implement.
//The two instances to compare are named 'a' and 'b'.
#define BP_EQUATABLE_TEMPL_START(OuterTemplate, Type) \
        /* Forward-declare operator== so it can be used for operator!= */ \
        template<OuterTemplate> bool operator==(const Type& a, const Type& b); \
        template<OuterTemplate> static inline bool operator!=(const Type& a, const Type& b) { return !(a == b); } \
        template<OuterTemplate> static inline bool operator==(const Type& a, const Type& b) {

//Defines operator!= for the given concrete type, then starts operator== for you to implement.
//The two instances to compare are named 'a' and 'b'.
#define BP_EQUATABLE_START(Type) \
        /*Forward-declare operator== so it can be used for operator!= */ \
        bool operator==(const Type& a, const Type& b); \
        static inline bool operator!=(const Type& a, const Type& b) { return !(a == b); } \
        static inline bool operator==(const Type& a, const Type& b) {


//Closes the code that started with BP_EQUATABLE[_TEMPL]_START().
#define BP_EQUATABLE_END }

#pragma endregion

#pragma region Macros that provide both hashing and equality

//Provides simple hashing and equality for the given templated type.
//The hashing is done with the given set of fields (of the form "d.MyField1, d.MyField2, [etc]").
//The equality must be implemented below this macro invocation,
//    using the two instances 'a' and 'b'.
//NOTE: this macro must be invoked in the global namespace!
#define BP_HASH_EQ_TEMPL_START(NamespacePath, OuterTemplate, Type, ...) \
    BP_HASHABLE_SIMPLE_FULL(OuterTemplate, NamespacePath::Type, __VA_ARGS__) \
    namespace NamespacePath { \
        BP_EQUATABLE_TEMPL_START(OuterTemplate, NamespacePath::Type)

//Provides simple hashing and equality for the given concrete type.
//The hashing is done with the given set of fields (of the form "d.MyField1, d.MyProperty2(), [etc]").
//The equality must be implemented below this macro invocation,
//    using the two instances 'a' and 'b'.
//NOTE: this macro must be invoked in the global namespace!
#define BP_HASH_EQ_START(NamespacePath, Type, ...) \
    BP_HASHABLE_SIMPLE(NamespacePath::Type, __VA_ARGS__) \
    namespace NamespacePath { \
        BP_EQUATABLE_START(NamespacePath::Type)

//Closes the code that started with BP_HASH_EQ_START() or BP_HASH_EQ_TEMPL_START().
#define BP_HASH_EQ_END  } BP_EQUATABLE_END

#pragma endregion

//TODO: Switch to a custom hash function, since implementing std::hash for built-in types like array and tuple is UB. This would then call for a custom macro to declare dictionaries.

//Provide hashing for std::array<>.
BP_HASHABLE_START_FULL(typename T BP_COMMA size_t N,
                       std::array<T BP_COMMA N>)
    using std::hash;
    hash<T> hasher;

    size_t h = 987654321;
    for (size_t i = 0; i < N; ++i)
        h = Bplus::CombineHash(h, hasher(d[i]));

    return h;
BP_HASHABLE_END

//Provide hashing for std::pair<>.
BP_HASHABLE_START_FULL(typename K BP_COMMA typename V,
                       std::pair<K BP_COMMA V>)
    return Bplus::CombineHash(d.first, d.second);
BP_HASHABLE_END

//Provide hashing for std::tuple<>.
//This requires some painful template code, so I found an article that implements it for us:
//  https://www.variadic.xyz/2018/01/15/hashing-stdpair-and-stdtuple/
#pragma region Hash STD tuples

template<class... TupleArgs>
struct std::hash<std::tuple<TupleArgs...>>
{
private:
    //  this is a termination condition
    //  N == sizeof...(TupleTypes)
    //
    template<size_t Idx, typename... TupleTypes>
    inline typename std::enable_if<Idx == sizeof...(TupleTypes), void>::type
    hash_combine_tup(size_t& seed, const std::tuple<TupleTypes...>& tup) const
    {
    }

    //  this is the computation function
    //  continues till condition N < sizeof...(TupleTypes) holds
    //
    template<size_t Idx, typename... TupleTypes>
    inline typename std::enable_if <Idx < sizeof...(TupleTypes), void>::type
    hash_combine_tup(size_t& seed, const std::tuple<TupleTypes...>& tup) const
    {
        Bplus::MultiHash(seed, std::get<Idx>(tup));

        //  on to next element
        hash_combine_tup<Idx + 1>(seed, tup);
    }

public:
    size_t operator()(const std::tuple<TupleArgs...>& tupleValue) const
    {
        size_t seed = 0;
        //  begin with the first iteration
        hash_combine_tup<0>(seed, tupleValue);
        return seed;
    }
};

#pragma endregion


//Note that std::variant already has a hash specalization.