// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!

// System
#include <stdio.h>
#include <string.h>
// Tools
#include <UnitTest.h>

// Unit CircularBufferTest:
extern TEST_CASE test_circular_buffer();
// Unit HTTPServerTest:
extern TEST_CASE test_http_server();

int main(int argc, const char *argv[])
{
    size_t units = 0, run = 0, passed = 0;
    const char *single_test = 0;

    if (argc == 2)
        single_test = argv[1];

    if (single_test==0  ||  strcmp(single_test, "CircularBufferTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit CircularBufferTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_circular_buffer:\n");
        if (test_circular_buffer())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "HTTPServerTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit HTTPServerTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_http_server:\n");
        if (test_http_server())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }

    printf("======================================================================\n");
    size_t failed = run - passed;
    printf("Tested %zu unit%s, ran %zu test%s, %zu passed, %zu failed.\n",
           units,
           (units > 1 ? "s" : ""),
           run,
           (run > 1 ? "s" : ""),
           passed, failed);
    printf("Success rate: %.1f%%\n", 100.0*passed/run);

    printf("==================================================\n");
    printf("--------------------------------------------------\n");
    if (failed != 0)
    {
        printf("THERE WERE ERRORS!\n");
        return -1;
    }
    printf("All is OK\n");
    printf("--------------------------------------------------\n");
    printf("==================================================\n");

    return 0;
}

