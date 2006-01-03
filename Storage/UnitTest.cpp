// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!

// System
#include <stdio.h>
#include <string.h>
// Tools
#include <UnitTest.h>

// Unit DataFileTest:
extern TEST_CASE test_data_file();
// Unit FileAllocatorTest:
extern TEST_CASE file_allocator_create_new_file();
extern TEST_CASE file_allocator_open_existing();
// Unit NameHashTest:
extern TEST_CASE name_hash_test();
// Unit RTreeTest:
extern TEST_CASE fill_tests();
extern TEST_CASE dump_blocks();
extern TEST_CASE update_test();
// Unit RawValueTest:
extern TEST_CASE RawValue_format();
extern TEST_CASE RawValue_auto_ptr();

int main(int argc, const char *argv[])
{
    size_t units = 0, run = 0, passed = 0;
    const char *single_test = 0;

    if (argc == 2)
        single_test = argv[1];

    if (single_test==0  ||  strcmp(single_test, "DataFileTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit DataFileTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("test_data_file:\n");
        if (test_data_file())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "FileAllocatorTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit FileAllocatorTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("file_allocator_create_new_file:\n");
        if (file_allocator_create_new_file())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("file_allocator_open_existing:\n");
        if (file_allocator_open_existing())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "NameHashTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit NameHashTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("name_hash_test:\n");
        if (name_hash_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "RTreeTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit RTreeTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("fill_tests:\n");
        if (fill_tests())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("dump_blocks:\n");
        if (dump_blocks())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("update_test:\n");
        if (update_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "RawValueTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit RawValueTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("RawValue_format:\n");
        if (RawValue_format())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("RawValue_auto_ptr:\n");
        if (RawValue_auto_ptr())
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

