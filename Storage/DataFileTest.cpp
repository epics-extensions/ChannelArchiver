

// Tools
#include <UnitTest.h>
// Storage
#include "DataFile.h"

TEST_CASE test_data_file()
{
    // Get read-only reference.
    DataFile *df1 = DataFile::reference("../DemoData", "20040305", false);
    TEST_MSG(df1, "Opened file");

    // Get another one, basically the same file.
    DataFile *df1a = DataFile::reference("", "../DemoData/20040305", false);
    TEST_MSG(df1a, "Opened again");
    TEST(df1 == df1a);

    DataFile *df1b = DataFile::reference("../DemoData/", "20040305", false);
    TEST_MSG(df1b, "Opened again");
    TEST(df1 == df1b);

    // Get a new ref. this time for writing.
    DataFile *df2 = DataFile::reference("", "../DemoData/20040305", true);
    TEST_MSG(df2, "Opened for writing");
    TEST(df1 != df2);

    df2->release();
    df1b->release();
    df1a->release();
    df1->release();

    DataFile::close_all();

    TEST_OK;
}
