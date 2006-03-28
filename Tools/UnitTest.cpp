// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!

// System
#include <stdio.h>
#include <string.h>
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
extern TEST_CASE test_autoarrayptr();
extern TEST_CASE test_autoarray_autoptr();
extern TEST_CASE test_autoarray_ordptr();
// Unit BinIOTest:
extern TEST_CASE bin_io_write();
extern TEST_CASE bin_io_read();
// Unit BitsetTest:
extern TEST_CASE test_bitset();
// Unit CATest:
extern TEST_CASE test_ca();
// Unit ConversionsTest:
extern TEST_CASE test_conversions();
// Unit FUXTest:
extern TEST_CASE test_fux();
// Unit FilenameTest:
extern TEST_CASE Filename_Test();
// Unit GenericExceptionTest:
extern TEST_CASE how_new_fails();
extern TEST_CASE various_exception_tests();
extern TEST_CASE double_throw();
// Unit GuardTest:
extern TEST_CASE guard_test();
extern TEST_CASE release_test();
extern TEST_CASE guard_performance();
// Unit IndexConfigTest:
extern TEST_CASE index_config_test();
// Unit LockfileTest:
extern TEST_CASE test_lockfile();
// Unit MsgLoggerTest:
extern TEST_CASE test_log();
// Unit ThreadTest:
extern TEST_CASE worker_thread();
extern TEST_CASE test_threads();
// Unit ThrottleTest:
extern TEST_CASE test_throttle();
// Unit ThrottledMsgLoggerTest:
extern TEST_CASE test_msg_throttle();
// Unit TimerTest:
extern TEST_CASE test_timer();
// Unit epicsTimeHelperTest:
extern TEST_CASE test_time();
// Unit stdStringTest:
extern TEST_CASE test_string();

int main(int argc, const char *argv[])
{
    size_t units = 0, run = 0, passed = 0;
    const char *single_test = 0;

    if (argc == 2)
        single_test = argv[1];

    if (single_test==0  ||  strcmp(single_test, "ASCIIParserTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ASCIIParserTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_ascii_parser:\n");
        if (test_ascii_parser())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "AVLTreeTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit AVLTreeTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\navl_test:\n");
        if (avl_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "AutoFilePtrTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit AutoFilePtrTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nbogus_auto_file_ptr:\n");
        if (bogus_auto_file_ptr())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nauto_file_ptr:\n");
        if (auto_file_ptr())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "AutoPtrTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit AutoPtrTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_autoptr:\n");
        if (test_autoptr())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ntest_autoarrayptr:\n");
        if (test_autoarrayptr())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ntest_autoarray_autoptr:\n");
        if (test_autoarray_autoptr())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ntest_autoarray_ordptr:\n");
        if (test_autoarray_ordptr())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "BinIOTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit BinIOTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nbin_io_write:\n");
        if (bin_io_write())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nbin_io_read:\n");
        if (bin_io_read())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "BitsetTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit BitsetTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_bitset:\n");
        if (test_bitset())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "CATest")==0)
    {
        printf("======================================================================\n");
        printf("Unit CATest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_ca:\n");
        if (test_ca())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "ConversionsTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ConversionsTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_conversions:\n");
        if (test_conversions())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "FUXTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit FUXTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_fux:\n");
        if (test_fux())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "FilenameTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit FilenameTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nFilename_Test:\n");
        if (Filename_Test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "GenericExceptionTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit GenericExceptionTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nhow_new_fails:\n");
        if (how_new_fails())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nvarious_exception_tests:\n");
        if (various_exception_tests())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ndouble_throw:\n");
        if (double_throw())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "GuardTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit GuardTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nguard_test:\n");
        if (guard_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nrelease_test:\n");
        if (release_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nguard_performance:\n");
        if (guard_performance())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "IndexConfigTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit IndexConfigTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nindex_config_test:\n");
        if (index_config_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "LockfileTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit LockfileTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_lockfile:\n");
        if (test_lockfile())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "MsgLoggerTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit MsgLoggerTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_log:\n");
        if (test_log())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "ThreadTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ThreadTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nworker_thread:\n");
        if (worker_thread())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ntest_threads:\n");
        if (test_threads())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "ThrottleTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ThrottleTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_throttle:\n");
        if (test_throttle())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "ThrottledMsgLoggerTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit ThrottledMsgLoggerTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_msg_throttle:\n");
        if (test_msg_throttle())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "TimerTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit TimerTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_timer:\n");
        if (test_timer())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "epicsTimeHelperTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit epicsTimeHelperTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_time:\n");
        if (test_time())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "stdStringTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit stdStringTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_string:\n");
        if (test_string())
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

