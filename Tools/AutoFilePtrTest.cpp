// System
#include <string.h>
// Tools
#include <AutoFilePtr.h>
#include <UnitTest.h>

TEST_CASE bogus_auto_file_ptr()
{
    AutoFilePtr bogus_file(fopen("notthere", "r"));
    TEST(! bogus_file);
    TEST_OK;
}

TEST_CASE auto_file_ptr()
{
    char line[100];
    FILE *stale_copy;
    {
        AutoFilePtr file_ok(fopen("ToolsTest.cpp", "r"));
        TEST(file_ok);
        TEST(fread(line, 1, 20, file_ok) == 20);
        line[17] = '\0';
        TEST(strcmp(line, "// ToolsTest.cpp\n") == 0);
        stale_copy = file_ok;
    }
    // Now the file should be closed
    // and this read should fail.
    TEST(fread(line, 1, 20, stale_copy) == 0);
    TEST_OK;
}


