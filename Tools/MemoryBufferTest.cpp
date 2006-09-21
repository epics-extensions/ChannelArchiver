#include "MemoryBuffer.h"
#include "UnitTest.h"

TEST_CASE test_mem_buf()
{
    printf("\nMemoryBuffer Test\n");
    MemoryBuffer<char> buf;
    
    buf.reserve(100);
    TEST(buf.capacity() >= 100);
    strcpy(buf.mem(), "Hi");
    TEST(strcmp(buf.mem(), "Hi") == 0);
    TEST_OK;
}

