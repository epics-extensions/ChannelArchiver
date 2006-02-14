// UnitTest Suite,
// created by makeUnitTestMain.pl.
// Do NOT modify!

// System
#include <stdio.h>
#include <string.h>
// Tools
#include <UnitTest.h>

// Unit AverageReaderTest:
extern TEST_CASE AverageReaderTest();
// Unit DataFileTest:
extern TEST_CASE test_data_file();
// Unit DataWriterTest:
extern TEST_CASE data_writer_test();
extern TEST_CASE data_writer_readback();
// Unit FileAllocatorTest:
extern TEST_CASE file_allocator_create_new_file();
extern TEST_CASE file_allocator_open_existing();
// Unit HashTableTest:
extern TEST_CASE hash_table_test();
// Unit LinearReaderTest:
extern TEST_CASE LinearReaderTest();
// Unit NameHashTest:
extern TEST_CASE name_hash_test();
// Unit PlotReaderTest:
extern TEST_CASE PlotReaderTest();
// Unit RTreeTest:
extern TEST_CASE fill_tests();
extern TEST_CASE dump_blocks();
extern TEST_CASE update_test();
// Unit RawDataReaderTest:
extern TEST_CASE RawDataReaderTest();
extern TEST_CASE DualRawDataReaderTest();
// Unit RawValueTest:
extern TEST_CASE RawValue_format();
extern TEST_CASE RawValue_auto_ptr();
// Unit SpreadsheetReaderTest:
extern TEST_CASE spreadsheet_dump();
extern TEST_CASE spreadsheet_values();

int main(int argc, const char *argv[])
{
    size_t units = 0, run = 0, passed = 0;
    const char *single_test = 0;

    if (argc == 2)
        single_test = argv[1];

    if (single_test==0  ||  strcmp(single_test, "AverageReaderTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit AverageReaderTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nAverageReaderTest:\n");
        if (AverageReaderTest())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "DataFileTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit DataFileTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ntest_data_file:\n");
        if (test_data_file())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "DataWriterTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit DataWriterTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\ndata_writer_test:\n");
        if (data_writer_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ndata_writer_readback:\n");
        if (data_writer_readback())
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
        printf("\nfile_allocator_create_new_file:\n");
        if (file_allocator_create_new_file())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nfile_allocator_open_existing:\n");
        if (file_allocator_open_existing())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "HashTableTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit HashTableTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nhash_table_test:\n");
        if (hash_table_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "LinearReaderTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit LinearReaderTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nLinearReaderTest:\n");
        if (LinearReaderTest())
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
        printf("\nname_hash_test:\n");
        if (name_hash_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "PlotReaderTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit PlotReaderTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nPlotReaderTest:\n");
        if (PlotReaderTest())
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
        printf("\nfill_tests:\n");
        if (fill_tests())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\ndump_blocks:\n");
        if (dump_blocks())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nupdate_test:\n");
        if (update_test())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "RawDataReaderTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit RawDataReaderTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nRawDataReaderTest:\n");
        if (RawDataReaderTest())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nDualRawDataReaderTest:\n");
        if (DualRawDataReaderTest())
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
        printf("\nRawValue_format:\n");
        if (RawValue_format())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nRawValue_auto_ptr:\n");
        if (RawValue_auto_ptr())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
    }
    if (single_test==0  ||  strcmp(single_test, "SpreadsheetReaderTest")==0)
    {
        printf("======================================================================\n");
        printf("Unit SpreadsheetReaderTest:\n");
        printf("----------------------------------------------------------------------\n");
        ++units;
        ++run;
        printf("\nspreadsheet_dump:\n");
        if (spreadsheet_dump())
            ++passed;
        else
            printf("THERE WERE ERRORS!\n");
        ++run;
        printf("\nspreadsheet_values:\n");
        if (spreadsheet_values())
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

