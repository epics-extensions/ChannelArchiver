// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!


// System
#include <stdio.h>
// Tools
#include <UnitTest.h>

// Unit AutoFilePtrTest:
extern TEST_CASE bogus_auto_file_ptr();
extern TEST_CASE auto_file_ptr();

int main()
{
    size_t run = 0, passed = 0;

    printf("Unit AutoFilePtrTest:\n");
    printf("--------------------------------------------------\n");
    ++run;
    printf("* bogus_auto_file_ptr:\n");
    if (bogus_auto_file_ptr())
        ++passed;
    ++run;
    printf("* auto_file_ptr:\n");
    if (auto_file_ptr())
        ++passed;

    printf("--------------------------------------------------\n");
    printf("--------------------------------------------------\n");
    size_t failed = run - passed;
    printf("Ran %zu test%s, %zu passed, %zu failed.\n",
           run,
           (run > 1 ? "s" : ""),
           passed, failed);
    printf("Success rate: %.1f%%\n", 100.0*passed/run);

    if (failed > 0)
        return -1;

    return 0;
}

