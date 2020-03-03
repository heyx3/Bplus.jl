#pragma once

#define TEST_NO_MAIN 
#include <acutest.h>

#include <B+/TomlIO.h>


BETTER_ENUM(TestEnum, int32_t,
               A =  1,  B =  2,  C =  3,
              _A = -1, _B = -2, _C = -3,
              Zero = 0);

void TomlEnums()
{
    auto a = +TestEnum::A;
    auto aStr = a._to_string();
    auto aInt = a._to_integral();

    TEST_CHECK_(a._to_string() == std::string("A"),
                "TestEnum::A as a string isn't 'A'; it's %s",
                aStr);
    TEST_CHECK_(aInt == 1,
                "TestEnum::A isn't equal to 1; it's %i",
                aInt);

    //TODO: TOML.
    //TODO: Redirect Robocopy output to a temp build file
}

TEST_LIST = {
    { "Toml <=> BETTER_ENUM", TomlEnums },
    { NULL, NULL }
};