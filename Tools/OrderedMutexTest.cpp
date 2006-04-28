// Tools
#include "OrderedMutex.h"
#include "UnitTest.h"

TEST_CASE deadlock_test()
{
    OrderedMutex a("a", 1);
    OrderedMutex b("b", 2);
    
    try
    {
        a.lock(__FILE__, __LINE__);
        b.lock(__FILE__, __LINE__);
        
        b.unlock();
        a.unlock();
    }
    catch (GenericException &e)
    {
        FAIL("Caught exception");
    }
    try
    {
        b.lock(__FILE__, __LINE__);
        a.lock(__FILE__, __LINE__);
        FAIL("I reversed the lock order without problems?!");
        b.unlock();
        a.unlock();
    }
    catch (GenericException &e)
    {
        PASS("Caught exception:");
        printf("        %s\n", e.what());
    }
    
    TEST_OK;
}
