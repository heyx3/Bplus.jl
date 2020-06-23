#include "BPAssert.h"

#include <assert.h>
#include <cstdio>


namespace
{
    Bplus::AssertFuncSignature currentAssertFunc = Bplus::DefaultAssertFunc;
}

void Bplus::SetAssertFunc(Bplus::AssertFuncSignature f)
{
    currentAssertFunc = f;
}
Bplus::AssertFuncSignature Bplus::GetAssertFunc()
{
    return currentAssertFunc;
}


void Bplus::DefaultAssertFunc(bool expr, const char* msg)
{
    if (!expr)
    {
        printf("BPAssert failed: ");
        printf(msg);
        printf("\n");

        assert(false);
    }
    else
    {
        //For completeness:
        assert(true);
    }
}