#pragma once

#define TEST_NO_MAIN 
#include <acutest.h>

#include <B+/TomlIO.h>

#include <random>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;


BETTER_ENUM(TestEnum, int32_t,
               A =  1,  B =  2,  C =  3,
              _A = -1, _B = -2, _C = -3,
              Zero = 0);


void TomlBasic()
{
    //Try to parse the TOML test file.
    auto tomlPath = fs::path(BPLUS_CONTENT_FOLDER) / "TestToml.toml";
    auto tomlParseResult = toml::parseFile(tomlPath.string());
    TEST_CHECK_(tomlParseResult.valid(),
                "Error parsing %s: %s",
                tomlPath.c_str(), tomlParseResult.errorReason.c_str());
    auto tomlData = tomlParseResult.value;

    //Test the root values inside the file.
    auto rootString = Bplus::IO::TomlTryGet<std::string>(tomlData, "root_string", "NO");
    TEST_CHECK_(Bplus::IO::TomlTryGet<std::string>(tomlData, "root_string", "NO")
                  == "hi",
                "'root_string' key wasn't correct");
    TEST_CHECK_(Bplus::IO::TomlTryGet<std::string>(tomlData, "nonexistent string./?", "NO")
                  == "NO",
                "key should not exist in the TOML, resulting in the default value being returned");

    //Test the table inside the file.
    TEST_CHECK_(tomlData.as<toml::Table>().find("my table")
                  != tomlData.as<toml::Table>().end(),
                "Can't find 'my table' in the TOML file %s",
                tomlPath.c_str());
    const auto& rootTable = tomlData["my table"];
    TEST_CHECK_(rootTable.is<toml::Table>(),
                "'my table' TOML item isn't actually a table, but a %s. %s",
                toml::Value::typeToString(rootTable.type()), tomlPath.c_str());
    TEST_CHECK_(Bplus::IO::TomlTryGet<std::string>(rootTable, "table_str", "NO")
                  == "hello",
                "'my table'/table_str doesn't exist or has the wrong value");
    TEST_CHECK_(Bplus::IO::TomlTryGet<int64_t>(rootTable, "table_int", -5647)
                  == 5,
                "'my table'/table_int doesn't exist or has the wrong value");
    TEST_CHECK_(Bplus::IO::TomlTryGet<bool>(rootTable, "table_bool", false)
                  == true,
                "'my table'/table_bool doesn't exist or has the wrong value");

    //Test the array inside the file.
    TEST_CHECK_(tomlData.as<toml::Table>().find("my_array")
                  != tomlData.as<toml::Table>().end(),
                "Can't find 'my_array' in the TOML file %s",
                tomlPath.c_str());
    const auto& rootArray = tomlData["my_array"];
    TEST_CHECK_(rootArray.is<toml::Array>(),
                "'my_array' TOML item isn't actually an array, but a %s. %s",
                toml::Value::typeToString(rootArray.type()), tomlPath.c_str());
    TEST_CHECK_(rootArray.size() == 3,
                "'my_array' in TOML file should have 3 elements, but has %i",
                rootArray.size());
    for (size_t i = 0; i < rootArray.size(); ++i)
    {
        auto element = rootArray.as<toml::Array>()[i];
        TEST_CHECK_(element.type() == toml::Value::INT_TYPE,
                    "'my_array'[%i] isn't an int, but a %s",
                    i, toml::Value::typeToString(element.type()));
        TEST_CHECK_(element.as<int64_t>() == (7 + i),
                    "'my_array'[%i]: expected %i, got %i",
                    i, (7 + i), element.as<int64_t>());
    }
}


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
    _TomlWrapping(Bplus::Bool{ false });

    //Test wrapping of TOML tables.
    //Note that TOML uses std::map for its tables,
    //    and both toml::Value and std::map implements operator==,
    //    so this should work just fine.
    toml::Table tTab;
    tTab["a"] = 5;
    tTab["c"] = "hi";
    tTab["..."] = false;
    _TomlWrapping(tTab);

    //Similarly, TOML uses std::vector for its arrays,
    //    and those implement operator== as well,
    //    so we can test the wrapping of a TOML array.
    toml::Array tArr;
    tArr.push_back(5);
    tArr.push_back("Hello there. General Kenobiiiii");
    tArr.push_back(true);
    _TomlWrapping(tArr);
}

void TomlPrimitives()
{
    using namespace Bplus::IO;

    //Run tests by wrapping integers into TOML, then unwrapping them again.
    //Try every combination of input number type and output number type.

#define TOML_TEST(v, tIn, tOut) \
    TEST_CHECK_(TomlUnwrap<tOut>(TomlWrap((tIn)v)) == v, \
                "TOML (" #tIn ")" #v " => " #tOut)
#define TOML_TESTS_UNSIGNED(v8s, tIn) \
    TOML_TEST(v8s,  tIn, uint8_t ); \
    TOML_TEST(v8s,  tIn, int8_t  ); \
    TOML_TEST(v8s, tIn, uint16_t); \
    TOML_TEST(v8s, tIn, int16_t ); \
    TOML_TEST(v8s, tIn, uint32_t); \
    TOML_TEST(v8s, tIn, int32_t ); \
    TOML_TEST(v8s, tIn, uint64_t); \
    TOML_TEST(v8s, tIn, int64_t )
#define TOML_TESTS_ALL(v8s, tIn) \
    TOML_TESTS_UNSIGNED(v8s, tIn); \
    TOML_TEST(-v8s, tIn, int8_t ); \
    TOML_TEST(-v8s, tIn, int16_t); \
    TOML_TEST(-v8s, tIn, int32_t); \
    TOML_TEST(-v8s, tIn, int64_t)

    TOML_TESTS_ALL     (83,  int8_t);
    TOML_TESTS_UNSIGNED(101, uint8_t);
    TOML_TESTS_ALL     (90,  int16_t);
    TOML_TESTS_UNSIGNED(91,  uint16_t);
    TOML_TESTS_ALL     (93,  int32_t);
    TOML_TESTS_UNSIGNED(93,  uint32_t);
    TOML_TESTS_ALL     (98,  int64_t);
    TOML_TESTS_UNSIGNED(95,  uint64_t);

#undef TOML_TESTS_ALL
#undef TOML_TESTS_UNSIGNED

    //Run similar tests for floats.
#define TOML_TEST_EPSILON(v, tIn, tOut, eps) \
    TEST_CHECK_(abs(TomlUnwrap<tOut>(TomlWrap((tIn)v)) - \
                    v) \
                  <= eps, \
                "TOML (" #tIn ")" #v " => " #tOut)
    TOML_TEST_EPSILON(2.5151132932, float,  float,  0.0001);
    TOML_TEST_EPSILON(34.345231230, double, float,  0.001);
    TOML_TEST_EPSILON(-3.134122552, float,  double, 0.0001);
    TOML_TEST_EPSILON(-51.90243923, double, double, 0.000001);

#undef TOML_TEST_EPSILON

    //Finally, run the test for booleans and the Bool struct.
    TOML_TEST(false, bool, bool);
    TOML_TEST(true,  bool, bool);
    TOML_TEST(false, bool, Bplus::Bool);
    TOML_TEST(true,  bool, Bplus::Bool);
    TOML_TEST(false, Bplus::Bool, bool);
    TOML_TEST(true,  Bplus::Bool, bool);
    TOML_TEST(false, Bplus::Bool, Bplus::Bool);
    TOML_TEST(true,  Bplus::Bool, Bplus::Bool);

#undef TOML_TEST
}

void TomlGLM()
{
    //Use a deterministic RNG to give values to the vectors/matrices.
    std::mt19937 rngE;
    rngE.seed(9743932);
    auto rngD = std::uniform_real_distribution(0.0, 1.0);
    auto rng = [&]() { return rngD(rngE); };

    #pragma region Vectors

#define TOML_TEST_VEC(L, T, rngToType, v1_v2_el_i_equality) { \
        TEST_CASE_("glm::vec<%i, %s>", L, #T); \
        glm::vec<L, T> v1; \
        for (int i = 0; i < L; ++i) \
            v1[i] = rngToType; \
        auto v1Toml = Bplus::IO::TomlWrap(v1); \
        auto v2 = Bplus::IO::TomlUnwrap<glm::vec<L, T>>(v1Toml); \
        for (int i = 0; i < L; ++i) \
            TEST_CHECK_(v1_v2_el_i_equality, \
                        "glm::vec<" #L ", " #T "> deserialization fail at i=%i", i); \
    }

#define TOML_TEST_VECS(T, rngToType, v1_v2_el_i_equality) \
    TOML_TEST_VEC(1, T, rngToType, v1_v2_el_i_equality); \
    TOML_TEST_VEC(2, T, rngToType, v1_v2_el_i_equality); \
    TOML_TEST_VEC(3, T, rngToType, v1_v2_el_i_equality); \
    TOML_TEST_VEC(4, T, rngToType, v1_v2_el_i_equality)

#define TOML_TEST_VECS_EXACT(T, rngToType) \
    TOML_TEST_VECS(T, rngToType, v1[i] == v2[i])
#define TOML_TEST_VECS_EPSILON(T, rngToType, eps) \
    TOML_TEST_VECS(T, rngToType, abs(v1[i] - v2[i]) <= eps)


    TOML_TEST_VECS_EXACT(int32_t,  (int32_t)floor(rng() * 10000) - 5000);
    TOML_TEST_VECS_EXACT(uint32_t, (uint32_t)floor(rng() * 20000));
    TOML_TEST_VECS_EXACT(bool,     (rng() > 0.5));
    TOML_TEST_VECS_EPSILON(float,  (float)rng(),    0.0001);
    TOML_TEST_VECS_EPSILON(double, rng(),           0.0000001);

#undef TOML_TEST_VEC
#undef TOML_TEST_VECS
#undef TOML_TEST_VECS_EXACT
#undef TOML_TEST_VECS_EPSILON

    #pragma endregion

    #pragma region Matrices

#define TOML_TEST_MAT(C, R, T, epsilon) { \
        glm::mat<C, R, T> m1; \
        TEST_CASE_("glm::mat<%i, %i, %s>", C, R, #T); \
        for (int c = 0; c < C; ++c) \
            for (int r = 0; r < R; ++r) \
                m1[c][r] = (T)rng(); \
        auto m1Toml = Bplus::IO::TomlWrap(m1); \
        auto m2 = Bplus::IO::TomlUnwrap<glm::mat<C, R, T>>(m1Toml); \
        for (int c = 0; c < C; ++c) \
            for (int r = 0; r < R; ++r) \
                TEST_CHECK_(abs(m1[c][r] - m2[c][r]) <= epsilon, \
                            "glm::mat<" #C ", " #R ", " #T "> deserialization fail at c=%i;r=%i " \
                              ": expected %f, got %f", \
                            c, r, m1[c][r], m2[c][r]); \
    }

#define TOML_TEST_MATS(R, T, epsilon) \
    TOML_TEST_MAT(2, R, T, epsilon); \
    TOML_TEST_MAT(3, R, T, epsilon); \
    TOML_TEST_MAT(4, R, T, epsilon)

#define TOML_TEST_MATS_BOTH(R, epsF, epsD) \
    TOML_TEST_MATS(R, float, epsF); \
    TOML_TEST_MATS(R, double, epsD)

    const double epsF = 0.0001,
                 epsD = 0.0000001;
    TOML_TEST_MATS_BOTH(2, epsF, epsD);
    TOML_TEST_MATS_BOTH(3, epsF, epsD);
    TOML_TEST_MATS_BOTH(4, epsF, epsD);

#undef TOML_TEST_MAT
#undef TOML_TEST_MATS
#undef TOML_TEST_MATS_BOTH

    #pragma endregion
}

void TomlEnums()
{
    auto a = +TestEnum::A;
    auto aStr = a._to_string();
    auto aInt = a._to_integral();

    using namespace Bplus::IO;

    //Test basic BETTER_ENUMS functionality.
    TEST_CHECK_(a._to_string() == std::string("A"),
                "TestEnum::A as a string isn't 'A'; it's %s",
                aStr);
    TEST_CHECK_(aInt == 1,
                "TestEnum::A isn't equal to 1; it's %i",
                aInt);

    //Test "wrapping" and "unwrapping" TOML data.
    auto tomlA = TomlWrap<TestEnum>(a);
    TEST_CHECK_(TomlUnwrap<TestEnum>(tomlA) == a,
                "Casting 'A' to TOML and back: \n\t%s",
                TomlToString(tomlA, toml::FORMAT_NONE));
    TEST_CHECK_(ToToml(a).as<TestEnum>() == a,
                "TestEnum::A conversion to TOML");

    //Test unwrapping an integer to an enum value.
    TEST_CHECK_(TomlUnwrap<TestEnum>(TomlWrap((int8_t)(+TestEnum::_A))) == +TestEnum::_A,
                "TOML-wrap enum::_A's integer value, then unwrap to the enum type");
}