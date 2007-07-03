
// Tools
#include <BinIO.h>
#include <AutoPtr.h>
#include <AutoFilePtr.h>
#include <GenericException.h>
#include <UnitTest.h>
// Index
#include "NameHash.h"

TEST_CASE name_hash_test()
{
    AutoFilePtr f("test/names.ht", "w+b");
    TEST_MSG(f, "Created file");

    try
    {
        FileAllocator fa;
        TEST_MSG(fa.attach(f, NameHash::anchor_size, true) == true, "FileAllocator initialized");
        NameHash names(fa, 0);
        stdString ID_txt;
        ID_txt = "ID";
        names.init(7);
        TEST(names.insert("fred",   ID_txt, 1) == true);
        TEST(names.insert("freddy", ID_txt, 2) == true);
        TEST(names.insert("james",  ID_txt, 2) == true);
        TEST(names.insert("james",  ID_txt, 3) == false);
        TEST(names.insert("James",  ID_txt, 4) == true);

        {
            AutoPtr<NameHash::Entry> entry(names.find("freddy"));
            TEST(entry);
            TEST(entry->getId() == 2);
        }

        AutoPtr<NameHash::Iterator> iter(names.iterator());
	    size_t name_count = 0;
        while (iter->isValid())
        {
            ++name_count;
            printf("        Name: %-30s ID: %ld\n",
                   iter->getName().c_str(),
                   (long)iter->getId());
            TEST(iter->getName() != "fred"    ||  iter->getId() == 1);
            TEST(iter->getName() != "freddy"  ||  iter->getId() == 2);
            TEST(iter->getName() != "james"   ||  iter->getId() == 3);
            TEST(iter->getName() != "James"   ||  iter->getId() == 4);
            iter->next();
        }
        TEST(name_count == 4);
        names.showStats(stdout);
        fa.detach();
    }
    catch (GenericException &e)
    {
        printf("Error: %s", e.what());
        FAIL("I didn't expect the Spanish Inquisition nor this exception");
    }
    TEST_OK;
}

