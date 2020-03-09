//Runs unit tests for BPlus.
//Uses the "acutest" single-header unit test library: https://github.com/mity/acutest

#include <acutest.h>

//AllTests.h is an auto-generated code file.
//It is generated as a pre-build step in Visual Studio,
//    so if you're getting an error about it not existing,
//    just hit "Compile" to fix.
#include "AllTests.h"

void TesterTest()
{
    std::cout << "Test\n";
    char d;
    std::cin >> d;

    std::cout << "You entered " << d << "\n";
}