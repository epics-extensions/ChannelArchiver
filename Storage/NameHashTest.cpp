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
    LOG_ASSERT(names.init(7));
    LOG_ASSERT(names.insert("fred", 1));
    LOG_ASSERT(names.insert("freddy", 2));
    LOG_ASSERT(names.insert("james", 2));
    LOG_ASSERT(names.insert("james", 3));
    LOG_ASSERT(names.insert("James", 4));
    FileOffset ID;
    LOG_ASSERT(names.find("freddy", ID));
    LOG_ASSERT(ID == 2);

    NameHash::Entry entry;
    unsigned long hashvalue;
    bool valid = names.startIteration(hashvalue, entry);
    while (valid)
    {
        printf("%4ld - Name: %-30s ID: %ld\n",
               hashvalue, entry.name.c_str(), entry.ID);
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
