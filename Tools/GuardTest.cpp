#include "ToolsConfig.h"
#include "Guard.h"
#include "BenchTimer.h"
#include "UnitTest.h"

TEST_CASE guard_test()
{
    OrderedMutex mutex1("1", 1), mutex2("2", 2);
    {
        Guard guard(__FILE__, __LINE__, mutex1);
        guard.check(__FILE__, __LINE__, mutex1);
        TEST("Guard::check is OK");
        TEST(guard.isLocked());
        try
        {
            guard.check(__FILE__, __LINE__, mutex2);
            FAIL("We should not reach this point");
        }
        catch (GenericException &e)
        {
            printf("  OK  : Caught %s\n", e.what());
            TEST_OK;
        }
        FAIL("Didn't catch the guard failure");
    }
}

TEST_CASE release_test()
{
    OrderedMutex mutex("to_release", 1);
    {
        Guard guard(__FILE__, __LINE__, mutex);
        guard.check(__FILE__, __LINE__, mutex);
        TEST(guard.isLocked());
        {
            GuardRelease release(__FILE__, __LINE__, guard);
            TEST(!guard.isLocked());
        }
        TEST(guard.isLocked());
    }
    TEST_OK;
}

TEST_CASE guard_performance()
{
    OrderedMutex mutex("test", 1);
    {
    	BenchTimer timer;
        Guard guard(__FILE__, __LINE__, mutex);
        size_t i, N=10;
        for (i=0; i<N; ++i)
        {
        	guard.unlock();
        	guard.lock(__FILE__, __LINE__);
        }
        timer.stop();
        double t = timer.runtime();
        printf("Time for %zu unlock/locks: %g secs, i.e. %g per sec\n",
               N, t, (double)N/t);
    }
    TEST_OK;
}


