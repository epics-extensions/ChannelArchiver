// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!


// System
#include <stdio.h>
// Tools
#include <UnitTest.h>

// Unit ASCIIParserTest:
extern TEST_CASE test_ascii_parser();
// Unit AVLTreeTest:
extern TEST_CASE avl_test();
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
// Unit GuardTest:
extern TEST_CASE guard_test();
// Unit MsgLoggerTest:
extern TEST_CASE test_log();
// Unit ThreadTest:
extern TEST_CASE worker_thread();
extern TEST_CASE test_threads();
// Unit TimerTest:
extern TEST_CASE test_timer();
// Unit epicsTimeHelperTest:
extern TEST_CASE test_time();
// Unit stdStringTest:
extern TEST_CASE test_string();

int main(int argc, const char *argv[])
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
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit AVLTreeTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("avl_test:\n");
    if (avl_test())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit AutoFilePtrTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("bogus_auto_file_ptr:\n");
    if (bogus_auto_file_ptr())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    ++run;
    printf("auto_file_ptr:\n");
    if (auto_file_ptr())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit AutoPtrTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_autoptr:\n");
    if (test_autoptr())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit BinIOTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("bin_io_write:\n");
    if (bin_io_write())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    ++run;
    printf("bin_io_read:\n");
    if (bin_io_read())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit ConversionsTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_conversions:\n");
    if (test_conversions())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit GenericExceptionTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("how_new_fails:\n");
    if (how_new_fails())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    ++run;
    printf("various_exception_tests:\n");
    if (various_exception_tests())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit GuardTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("guard_test:\n");
    if (guard_test())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit MsgLoggerTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_log:\n");
    if (test_log())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit ThreadTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("worker_thread:\n");
    if (worker_thread())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    ++run;
    printf("test_threads:\n");
    if (test_threads())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit TimerTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_timer:\n");
    if (test_timer())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit epicsTimeHelperTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_time:\n");
    if (test_time())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");
    printf("==================================================\n");
    printf("Unit stdStringTest:\n");
    printf("--------------------------------------------------\n");
    ++units;
    ++run;
    printf("test_string:\n");
    if (test_string())
        ++passed;
    else
        printf("THERE WERE ERRORS!\n");

    printf("==================================================\n");
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

