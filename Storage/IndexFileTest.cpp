// Index
#include "IndexFile.h"
#include "RTree.h"

bool index_test()
{
    IndexFile index;

    if (!index.open("index", false))
        return false;
    if (!index.addChannel("fred"))
        return false;
    if (!index.addChannel("freddy"))
        return false;
    if (!index.addChannel("jane"))
        return false;
    if (!index.addChannel("janet"))
        return false;
    index.close();
    
    if (!index.open("index", false))
        return false;
    if (!index.addChannel("Alfred"))
        return false;
    index.close();

    if (!index.open("index", false))
        return false;
    IndexFile::NameIterator iter;
    bool valid = index.getFirstChannel(iter);
    while (valid)
    {
        printf("'%s'\n", iter.getName().c_str());
        valid = index.getNextChannel(iter);
    }
    index.close();

    if (!index.open("index", false))
        return false;
    RTree *tree = index.getTree("fred");
    if (!tree)
        return false;
    epicsTime start, end;
    string2epicsTime("12/24/2003 12:00:00", start);
    end = start + 90;
    tree->insertDatablock(start, end, 0x1200, "20031214");
    tree->makeDot("i1.dot", true);
    start += 120;
    end = start + 90;    
    tree->insertDatablock(start, end, 0x1202, "20031214");
    tree->makeDot("i2.dot", true);
    start += 120;
    end = start + 90;
    tree->insertDatablock(start, end, 0x1204, "20031214");
    tree->makeDot("i3.dot", true);
    start += 120;
    end = start + 90;
    tree->insertDatablock(start, end, 0x1206, "20031214");
    tree->makeDot("i4.dot", true);
    delete tree;
    index.close();
    
    if (!index.open("index", false))
        return false;
    tree = index.getTree("fred");
    if (!tree)
        return false;
    tree->makeDot("fred.dot", true);
    delete tree;
    index.close();
    
    return true;
}

int main()
{
    initEpicsTimeHelper();
    if (!index_test())
        return -1;
    return 0;
}
