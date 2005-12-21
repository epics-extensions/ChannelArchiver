#include "ToolsConfig.h"
#include "Guard.h"
#include "UnitTest.h"

TEST_CASE guard_test()
{
    epicsMutex mutex1, mutex2;
    {
        Guard guard(mutex1);
        guard.check(__FILE__, __LINE__, mutex1);
        TEST("Guard::check is OK");
        try
        {
            guard.check(__FILE__, __LINE__, mutex2);
            FAIL("We should not reach this point");
        }
        catch (GenericException &e)
        {
            printf("  OK  : Caught %s", e.what());
            TEST_OK;
        }
        FAIL("Didn't catch the guard failure");
    }
}

