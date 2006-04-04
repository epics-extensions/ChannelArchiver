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
// Unit ProcessVariableTest:
extern TEST_CASE process_variable();
// Unit SampleMechanismTest:
extern TEST_CASE test_sample_get();
extern TEST_CASE test_sample_monitor();
// Unit ScanListTest:
extern TEST_CASE test_scan_list();
// Unit TimeFilterTest:
extern TEST_CASE test_time_filter();
// Unit TimeSlotFilterTest:
extern TEST_CASE test_time_slot_filter();

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
    if (single_test==0  ||  strcmp(single_test, "ProcessVariableTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ProcessVariableTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nprocess_variable:\n");
        if (process_variable())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "SampleMechanismTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit SampleMechanismTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_sample_get:\n");
        if (test_sample_get())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ntest_sample_monitor:\n");
        if (test_sample_monitor())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "ScanListTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ScanListTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_scan_list:\n");
        if (test_scan_list())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "TimeFilterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit TimeFilterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_time_filter:\n");
        if (test_time_filter())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "TimeSlotFilterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit TimeSlotFilterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_time_slot_filter:\n");
        if (test_time_slot_filter())
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

