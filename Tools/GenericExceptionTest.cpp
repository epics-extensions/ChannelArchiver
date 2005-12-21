
#include "ArchiveException.h"
#include "UnitTest.h"

class Huge
{
public:
    Huge()
    {
        mem = new char[0x7FFFFFFFL];
    }
    ~Huge()
    {
        delete [] mem;
    }
    char *mem;
};

TEST_CASE how_new_fails()
{
    size_t num = 0x7FFFFFFFL;
    Huge *mem = 0;

    // Assume that IN_VALGRIND is set by <somebody>
    // when running under valgrind.
    if (getenv("IN_VALGRIND"))
    {
        TEST("Under valgrind, we can't run the memory exhaustion test");
    }
    else
    {
        try
        {
            mem = new Huge[num];
            // We should not reach this point
            TEST(mem != 0);
            delete [] mem;
        }
        // In theory, this should happen
        //    catch (std::bad_alloc)
        // But with the gcc 4.0 on Mac OS X 10.4,
        // we get a 'malloc' error message
        // and then end up here:
        catch (...)
        {
            TEST_MSG(1, "Caught an exception from new");
            mem = (Huge *)1;
        }
        TEST_MSG(mem == (Huge *)1, "Handled the 'new' failure");
    }
    TEST_OK;
}

TEST_CASE various_exception_tests()
{
    int exception_count = 0;

    try
    {
        throw GenericException(__FILE__, __LINE__);
    }
    catch (GenericException &e)
    {
        printf("     %s", e.what());
        ++exception_count;
    }

    try
    {
        throw GenericException(__FILE__, __LINE__, "Hello %s", "World");
    }
    catch (GenericException &e)
    {
        printf("     %s", e.what());
        ++exception_count;
    }
    try
    {
        throwArchiveException(Invalid);
    }
    catch (GenericException &e)
    {
        printf("     %s", e.what());
        ++exception_count;
    }

    try
    {
        throwDetailedArchiveException(Invalid, "in a test");
    }
    catch (GenericException &e)
    {
        printf("     %s", e.what());
        ++exception_count;
    }
    TEST(exception_count == 4);
    TEST_OK;
}


