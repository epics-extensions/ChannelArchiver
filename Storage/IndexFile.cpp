// Tools
#include <AutoPtr.h>
#include <BinIO.h>
#include <MsgLogger.h>
#include <Filename.h>
// Index
#include "IndexFile.h"
#include "RTree.h"

// File Layout:
// long cookie;
// NameHash::anchor_size anchor;
// Rest handled by FileAllocator, NameHash and RTree

long IndexFile::ht_size = 1009;

IndexFile::IndexFile(int RTreeM) : RTreeM(RTreeM), f(0), names(fa, 4)
{}

bool IndexFile::open(const stdString &filename, bool readonly)
{
    Filename::getDirname(filename, dirname);
    bool new_file = false;
    if (readonly)
        f = fopen(filename.c_str(), "rb");
    else
    {   // Try existing file
        f = fopen(filename.c_str(), "r+b");
        if (!f)
        {   // Create new file
            f = fopen(filename.c_str(), "w+b");
            new_file = true;
        }   
    }
    if (!f)
    {
        LOG_MSG("IndexFile::open(%s) cannot %s file.\n",
                filename.c_str(), (new_file ? "create" : "open"));
        return false;
    }
    // TODO: Tune these two. All 0 seems best?!
    FileAllocator::minimum_size = 0;
    FileAllocator::file_size_increment = 0;
    fa.attach(f, 4+NameHash::anchor_size);
    if (new_file)
    {
        if (fseek(f, 0, SEEK_SET)  ||  !writeLong(f, cookie))
        {
            LOG_MSG("IndexFile::open(%s) cannot write cookie.\n",
                    filename.c_str());
            goto open_error;
        }
        if (names.init(ht_size))
            return true;
        goto open_error;
    }
    // Check existing file
    unsigned long file_cookie;
    if (fseek(f, 0, SEEK_SET)  ||  !readLong(f, &file_cookie))
    {
        LOG_MSG("IndexFile::open(%s) cannot read cookie.\n",
                filename.c_str());
        goto open_error;
    }
    if (file_cookie != cookie)
    {
        LOG_MSG("IndexFile::open(%s) doesn't have valid cookie.\n",
                filename.c_str());
        goto open_error;
    }
    if (!names.reattach())
        goto open_error;
    return true;
  open_error:
    close();
    return false;
}

void IndexFile::close()
{
    if (f)
    {
        fa.detach();
        fclose(f);
        f = 0;
    }   
}

RTree *IndexFile::addChannel(const stdString &channel, stdString &directory)
{
    RTree *tree = getTree(channel, directory);
    if (tree)
        return tree;
    stdString tree_filename;
    long tree_anchor;
    if (!(tree_anchor = fa.allocate(RTree::anchor_size)))
    {
        LOG_MSG("IndexFile::add_channel(%s): cannot allocate tree anchor\n",
                channel.c_str());
        return false;
    }
    tree = new RTree(fa, tree_anchor);
    if (tree->init(RTreeM) &&
        names.insert(channel, tree_filename, tree_anchor))
    {
        directory = dirname;
        return tree;
    }
    delete tree;
    return 0;
}

RTree *IndexFile::getTree(const stdString &channel, stdString &directory)
{
    stdString  tree_filename;
    FileOffset tree_anchor;
    if (!names.find(channel, tree_filename, tree_anchor))
        return 0;
    RTree *tree = new RTree(fa, tree_anchor);
    if (tree->reattach())
    {
        directory = dirname;
        return tree;
    }
    delete tree;
    return 0;
}

bool IndexFile::getFirstChannel(NameIterator &iter)
{
    return names.startIteration(iter.hashvalue, iter.entry);
}

bool IndexFile::getNextChannel(NameIterator &iter)
{
    return names.nextIteration(iter.hashvalue, iter.entry);
}

void IndexFile::showStats(FILE *f)
{
    names.showStats(f);
}

bool IndexFile::check(int level)
{
    printf("Checking FileAllocator...\n");
    if (fa.dump(level))
        printf("FileAllocator OK\n");
    else
    {
        printf("FileAllocator ERROR\n");
        return false;
    }
    NameIterator names;
    bool have_name;
    unsigned long channels = 0;
    unsigned long total_nodes=0, total_used_records=0, total_records=0;
    unsigned long nodes, records;
    stdString dir;
    for (have_name = getFirstChannel(names);
         have_name;
         have_name = getNextChannel(names))
    {
        ++channels;
        AutoPtr<RTree> tree(getTree(names.getName(), dir));
        if (!tree)
        {
            printf("Cannot get tree for channel '%s'\n",
                   names.getName().c_str());
            return false;
        }
        printf(".");
        fflush(stdout);
        if (!tree->selfTest(nodes, records))
        {
            printf("RTree for channel '%s' is broken\n",
                   names.getName().c_str());
            return false;
        }
        total_nodes += nodes;
        total_used_records += records;
        total_records += nodes * tree->getM();
    }
    printf("\nAll RTree self-tests check out fine\n");
    printf("%ld channels\n", channels);
    printf("Total: %ld nodes, %ld records out of %ld are used (%.1lf %%)\n",
           total_nodes, total_used_records, total_records,
           total_used_records*100.0/total_records);
    return true;
}
