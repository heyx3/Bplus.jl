#pragma once

#include "../SimpleApp.hpp"
#include <B+/Utils.h>

#define TEST_NO_MAIN
#include <acutest.h>

using namespace Bplus;


void TestToBaseString()
{
    TEST_CHECK_(Strings::ToBinaryString(55) == "110111",
                "55 to binary");
    TEST_CHECK_(Strings::ToBinaryString(0, true, "abc") == "abc0",
                "0 with prefix 'abc' to base 2");

    TEST_CHECK_(Strings::ToBaseString(55, Strings::NumberBases::Binary)
                    == "110111",
                "55 to base 2");
    TEST_CHECK_(Strings::ToBaseString(0, Strings::NumberBases::Binary,
                                      "abc")
                 == "abc0",
                "0 with prefix 'abc' to base 2");

    TEST_CHECK_(Strings::ToBaseString(1234567890,
                                      Strings::NumberBases::Decimal)
                   == "1234567890",
                "Large number to decimal");
    TEST_CHECK_(Strings::ToBaseString(1234567890,
                                      Strings::NumberBases::Decimal,
                                      "0.")
                   == "0.1234567890",
                "Large number to decimal, with prefix");
    
    TEST_CHECK_(Strings::ToBaseString(1999999999,
                                      Strings::NumberBases::Octal)
                  == "16715311777",
                "Large number to Octal");
    TEST_CHECK_(Strings::ToBaseString(0, Strings::NumberBases::Octal, "o")
                  == "o0",
                "Zero to Octal with prefix");
    
    TEST_CHECK_(Strings::ToBaseString(1989503886,
                                      Strings::NumberBases::Hex)
                  == "76956B8E",
                "Large number to Hex");
    TEST_CHECK_(Strings::ToBaseString(15,
                                      Strings::NumberBases::Hex,
                                      "0 X ")
                  == "0 X F",
                "Fifteen to Hex with Prefix");
}

void TestStringStartsWith()
{
    TEST_CHECK_(Strings::StartsWith("abc123", "a"),
                "'abc123' starts with 'a'");
    TEST_CHECK_(!Strings::StartsWith("abc123", "b"),
                "'abc123' doesn't start with 'b'");
    TEST_CHECK_(Strings::StartsWith("abc123", "abc1"),
                "'abc123' starts with 'abc1'");
    TEST_CHECK_(!Strings::StartsWith("abc123", "123"),
                "'abc123' doesn't start with '123'");
}
void TestStringEndsWith()
{
    TEST_CHECK_(Strings::EndsWith("abc123", "3"),
                "'abc123' ends with '3'");
    TEST_CHECK_(!Strings::EndsWith("abc123", "2"),
                "'abc123' doesn't end with '2'");
    TEST_CHECK_(Strings::EndsWith("abc123", "c123"),
                "'abc123' ends with 'c123'");
    TEST_CHECK_(!Strings::EndsWith("abc123", "abc"),
                "'abc123' doesn't end with 'abc'");
}

namespace
{
    //Runs a given test for both versions of Strings::Replace().
    void _TestStringReplaces(const std::string& _testName,
                             const std::string& src,
                             const std::string& snippet,
                             const std::string& replaceWith,
                             const std::string& expected)
    {
        std::string testName = _testName + " (inline)";
        TEST_CASE(testName.c_str());
        std::string result = src;
        Strings::Replace(result, snippet, replaceWith);
        if (!TEST_CHECK(result == expected))
            TEST_MSG("Expected \"%s\" but got \"%s\"", expected.c_str(), result.c_str());

        testName = _testName + " (New-ed)";
        TEST_CASE(testName.c_str());
        result = Strings::ReplaceNew(src, snippet, replaceWith);
        if (!TEST_CHECK(result == expected))
            TEST_MSG("Expected \"%s\" but got \"%s\"", expected.c_str(), result.c_str());
    }
}
void TestStringReplace()
{
    _TestStringReplaces("src is Empty String",
                        "",
                        "abc", "def",
                        "");
    _TestStringReplaces("snippet is Empty String",
                        "abc",
                        "", "def",
                        "abc");
    _TestStringReplaces("replacedWith is Empty String",
                        "abc123abc",
                        "abc", "",
                        "123");
    _TestStringReplaces("basic",
                        "Hello, world",
                        "l", "[the letter L]",
                        "He[the letter L][the letter L]o, wor[the letter L]d");
    _TestStringReplaces("large replacement",
                        "abc123abc",
                        "c123a", "[]",
                        "ab[]bc");
}

//TODO: Test Strings::FindDifference()