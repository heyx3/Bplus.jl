#pragma once

#define TEST_NO_MAIN 
#include <acutest.h>

#include <B+/TomlIO.h>


BETTER_ENUM(TestEnum, int32_t,
               A =  1,  B =  2,  C =  3,
              _A = -1, _B = -2, _C = -3,
              Zero = 0);


template<typename T>
void _TomlWrapping(const T& val)
{
    auto tomlVal = Bplus::IO::TomlWrap(val);
    T unpackedVal = Bplus::IO::TomlUnwrap<T>(tomlVal);

    TEST_CHECK_(val == unpackedVal,
                "TomlUnwrap(TomlWrap(a)) == b");
}
void TomlWrapping()
{
    _TomlWrapping(std::string("Hi there"));
    _TomlWrapping(12345);
    _TomlWrapping((int8_t)-50);
    _TomlWrapping(false);
    _TomlWrapping(Bool{ false });

    //Test wrapping of TOML tables.
    //Note that TOML uses std::map for its tables,
    //    and both toml::Value and std::map implements operator==,
    //    so this should work just fine.
    toml::Value tVal;
    tVal["a"] = 5;
    tVal["c"] = "hi";
    tVal["..."] = false;
    _TomlWrapping(tVal);

    //Similarly, TOML uses std::vector for its arrays,
    //    and those implement operator== as well,
    //    so we can test the wrapping of a TOML array.
    tVal = toml::Array();
    tVal.push(5);
    tVal.push("Hello there. General Kenobiiiii");
    tVal.push(true);
    _TomlWrapping(tVal);
}


void TomlEnums()
{
    auto a = +TestEnum::A;
    auto aStr = a._to_string();
    auto aInt = a._to_integral();

    using namespace Bplus::IO;

    TEST_CHECK_(a._to_string() == std::string("A"),
                "TestEnum::A as a string isn't 'A'; it's %s",
                aStr);
    TEST_CHECK_(aInt == 1,
                "TestEnum::A isn't equal to 1; it's %i",
                aInt);

    auto tomlA = TomlWrap<TestEnum>(a);
    TEST_CHECK_(TomlUnwrap<TestEnum>(tomlA) == a,
                "Casting 'A' to TOML and back: \n\t%s",
                ToTomlString(tomlA, toml::FORMAT_NONE));

    TEST_CHECK_(ToToml(a).as<TestEnum>() == a,
                "TestEnum::A conversion to TOML");
}

TEST_LIST = {
    { "Toml wrapping/unwrapping", TomlWrapping },
    { "Toml <=> BETTER_ENUM",     TomlEnums },
    { NULL, NULL }
};