#pragma once

#include "../Platform.h"

#include "BPAssert.h"
#include <unordered_map>


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


//Provide hashing for std::array<>.
namespace std {
    template<typename T, size_t N>
    struct hash<std::array<T, N>> {
        size_t operator()(const std::array<T, N>& a) const {
            using std::hash;
            hash<T> hasher;

            size_t h = 0;
            for (size_t i = 0; i < N; ++i)
                h = Bplus::CombineHash(h, hasher(a[i]));
            return h;
        }
    };
}