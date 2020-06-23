#pragma once

#include "../Platform.h"

#include <unordered_map>


//Bool is a struct that acts like a boolean,
//    but doesn't have any weird specialization in an std::vector<>.

namespace Bplus
{
    //Mimics the standard bool type.
    struct BP_API Bool
    {
        Bool() : data(false) { }
        Bool(bool b) : data(b) { }

        operator const bool&() const { return data; }
        operator bool&() { return data; }

        Bool& operator=(bool b) { data = b; return *this; }

        bool operator!() const { return !data; }
        bool operator|(bool b) const { return data | b; }
        bool operator&(bool b) const { return data & b; }

        bool operator==(bool b) const { return b == data; }
        bool operator!=(bool b) const { return b != data; }

    private:
        bool data;
    };
}

//Provide hashing for Bool.
namespace std {
    template<> struct BP_API hash<Bplus::Bool> {
        size_t operator()(Bplus::Bool key) const {
            using std::hash;
            return hash<bool>()(key);
        }
    };
}