
// Tools
#include "MsgLogger.h"
#include "GenericException.h"
#include "UnitTest.h"


TEST_CASE test_list()
{
    printf("------------------------------------------\n");
    printf("List Test\n");
    printf("------------------------------------------\n");

    stdList<stdString> list;
    
    list.push_back(stdString("A"));
    list.push_back(stdString("B"));
    list.push_back(stdString("C"));

    printf("Dump\n");
    TEST(list.size() == 3);
    stdList<stdString>::iterator i;
    for (i = list.begin(); i!=list.end(); ++i)
    {
        printf("        %s\n", i->c_str());
    }
    
    printf("Append while traversing\n");
    i = list.begin();
    printf("        %s\n", i->c_str());
    ++i;
    list.push_back(stdString("D added while traversing"));
    for (/**/; i!=list.end(); ++i)
    {
        printf("        %s\n", i->c_str());
    }

#if 0
    printf("Delete 'A' while traversing\n");
    i = list.begin();
    // This results in crash, since we remove an element
    list.pop_front();
    // ... and then try to access the deleted element:
    printf("        %s\n", i->c_str());
    ++i;
    for (/**/; i!=list.end(); ++i)
    {
        printf("        %s\n", i->c_str());
    }
#endif

    TEST_OK;
}

