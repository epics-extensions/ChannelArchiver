

/*
TESTPROD_HOST += UnitTest
UnitTest_CXXFLAGS += -DUNIT_TEST
*/

typedef bool TEST_CASE;

#define TEST(t)                        \
       if (t)                          \
           printf("  OK  : %s\n", #t); \
       else                            \
       {                               \
           printf("  FAIL: %s\n", #t); \
           return false;               \
       }

#define TEST_MSG(t,msg)                 \
       if (t)                           \
           printf("  OK  : %s\n", msg); \
       else                             \
       {                                \
           printf("  FAIL: %s\n", msg); \
           return false;                \
       }

#define TEST_OK   return true

