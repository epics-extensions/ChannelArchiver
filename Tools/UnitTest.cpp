// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!


// System
#include <stdio.h>
// Tools
#include <UnitTest.h>

// Unit ASCIIParserTest:
extern TEST_CASE test_ascii_parser();
// Unit AutoFilePtrTest:
extern TEST_CASE bogus_auto_file_ptr();
extern TEST_CASE auto_file_ptr();
// Unit AutoPtrTest:
extern TEST_CASE test_autoptr();
// Unit BinIOTest:
extern TEST_CASE bin_io_write();
extern TEST_CASE bin_io_read();
// Unit ConversionsTest:
extern TEST_CASE test_conversions();
// Unit GenericExceptionTest:
extern TEST_CASE how_new_fails();
extern TEST_CASE various_exception_tests();

int main()
{
    size_t units = 0, run = 0, passed = 0;

    printf("==================================================\n");
    printf("Unit ASCIIParserTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_ascii_parser:\n");
    if (test_ascii_parser())
        ++passed;
    printf("==================================================\n");
    printf("Unit AutoFilePtrTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("bogus_auto_file_ptr:\n");
    if (bogus_auto_file_ptr())
        ++passed;
    ++run;
    printf("auto_file_ptr:\n");
    if (auto_file_ptr())
        ++passed;
    printf("==================================================\n");
    printf("Unit AutoPtrTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_autoptr:\n");
    if (test_autoptr())
        ++passed;
    printf("==================================================\n");
    printf("Unit BinIOTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("bin_io_write:\n");
    if (bin_io_write())
        ++passed;
    ++run;
    printf("bin_io_read:\n");
    if (bin_io_read())
        ++passed;
    printf("==================================================\n");
    printf("Unit ConversionsTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_conversions:\n");
    if (test_conversions())
        ++passed;
    printf("==================================================\n");
    printf("Unit GenericExceptionTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("how_new_fails:\n");
    if (how_new_fails())
        ++passed;
    ++run;
    printf("various_exception_tests:\n");
    if (various_exception_tests())
        ++passed;

    printf("==================================================\n");
    size_t failed = run - passed;
    printf("Tested %zu unit%s, ran %zu test%s, %zu passed, %zu failed.\n",
           units,
           (units > 1 ? "s" : ""),
           run,
           (run > 1 ? "s" : ""),
           passed, failed);
    printf("Success rate: %.1f%%\n", 100.0*passed/run);

    if (failed > 0)
        return -1;

    return 0;
}

