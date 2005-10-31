// -*- c++ -*-
// Tools
#include <BinIO.h>
#include <MsgLogger.h>
// Index
#include "NameHash.h"

bool name_hash_test()
{
    FILE *f = fopen("test/names.ht", "w+b");
    LOG_ASSERT(f);
    FileAllocator fa;
    LOG_ASSERT(fa.attach(f, NameHash::anchor_size));
    NameHash names(fa, 0);
    stdString ID_txt;
    ID_txt = "ID";
    LOG_ASSERT(names.init(7));
    LOG_ASSERT(names.insert("fred", ID_txt, 1));
    LOG_ASSERT(names.insert("freddy", ID_txt, 2));
    LOG_ASSERT(names.insert("james", ID_txt, 2));
    LOG_ASSERT(names.insert("james", ID_txt, 3));
    LOG_ASSERT(names.insert("James", ID_txt, 4));
    FileOffset ID;
    LOG_ASSERT(names.find("freddy", ID_txt, ID));
    LOG_ASSERT(ID == 2);

    NameHash::Entry entry;
    uint32_t hashvalue;
    bool valid = names.startIteration(hashvalue, entry);
    while (valid)
    {
        printf("%4ld - Name: %-30s ID: %ld\n",
               (long)hashvalue, entry.name.c_str(), (long)entry.ID);
        valid = names.nextIteration(hashvalue, entry);
    }
    names.showStats(stdout);
    fa.detach();
    return true;
}

int main()
{
    if (!name_hash_test())
	return -1;
    return 0;
}
