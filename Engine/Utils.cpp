#include "Utils.h"

#include <iostream>
#include <assert.h>


Bplus::AssertFuncSignature Bplus::assertFunc = Bplus::DefaultAssertFunc;

void Bplus::DefaultAssertFunc(bool expr, const char* msg)
{
    if (!expr)
        std::cout << "BPAssert failed: " << msg << "\n";

    assert(expr);
}