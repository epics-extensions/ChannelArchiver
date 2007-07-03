// System
#include <stdlib.h>
// Tools
#include <AutoPtr.h>
#include <BinIO.h>
#include <UnitTest.h>
// Storage
#include "IndexFile.h"
#include "ListIndex.h"

TEST_CASE list_index_test()
{
    try
    {
        IndexFile file;
        ListIndex list;
        
        file.open("../DemoData/index");
        list.open("../DemoData/index.xml");
        
        AutoPtr<Index::Result> file_result(file.findChannel("fred"));
        AutoPtr<Index::Result> list_result(list.findChannel("fred"));
        
        TEST(file_result);
        TEST(list_result);
        
        RTree *file_tree = file_result->getRTree();
        RTree *list_tree = list_result->getRTree();
        TEST(file_tree->getInterval() == list_tree->getInterval());
        
        size_t expect = 0;
        AutoPtr<Index::NameIterator> names(file.iterator());
        while (names && names->isValid())
        {
            ++expect;
            names->next();
        }
        
        size_t found = 0;
        names = list.iterator();
        while (names && names->isValid())
        {
            ++found;
            names->next();
        }
    
        TEST(found == expect);
    }
    catch (GenericException &e)
    {
        printf("Exception:\n%s\n", e.what());
        FAIL("Exception");
    }

    TEST_OK;
}
