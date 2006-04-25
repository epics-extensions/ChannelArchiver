#include "ToolsConfig.h"
#include "Guard.h"
#include "BenchTimer.h"
#include "UnitTest.h"

TEST_CASE guard_test()
{
    epicsMutex mutex1, mutex2;
    {
        Guard guard(mutex1);
        guard.check(__FILE__, __LINE__, mutex1);
        TEST("Guard::check is OK");
        TEST(guard.isLocked());
        mutex1.show(10);
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

TEST_CASE release_test()
{
    epicsMutex mutex;
    {
        Guard guard(mutex);
        guard.check(__FILE__, __LINE__, mutex);
        TEST(guard.isLocked());
        {
            GuardRelease release(guard);
            TEST(!guard.isLocked());
        }
        TEST(guard.isLocked());
    }
    TEST_OK;
}

TEST_CASE guard_performance()
{
    epicsMutex mutex;
    {
    	BenchTimer timer;
        Guard guard(mutex);
        size_t i, N=100000;
        for (i=0; i<N; ++i)
        {
        	guard.unlock();
        	guard.lock();
        }
        timer.stop();
        double t = timer.runtime();
        printf("Time for %zu unlock/locks: %g secs, i.e. %g per sec\n",
               N, t, (double)N/t);
    }
    TEST_OK;
}


