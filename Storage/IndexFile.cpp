// Tools
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

IndexFile::IndexFile() : f(0), names(fa, 4)
{}

bool IndexFile::open(const stdString &filename, bool readonly)
{
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
    long file_cookie;
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
    this->filename = filename;
    Filename::getDirname(filename, dirname);
    return true;
  open_error:
    fclose(f);
    f=0;
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

RTree *IndexFile::addChannel(const stdString &name)
{
    RTree *tree = getTree(name);
    if (tree)
        return tree;
    long tree_anchor;
    if (!(tree_anchor = fa.allocate(RTree::anchor_size)))
    {
        LOG_MSG("IndexFile::add_channel(%s): cannot allocate tree anchor\n",
                name.c_str());
        return false;
    }
    tree = new RTree(fa, tree_anchor);
    if (tree->init() && names.insert(name, tree_anchor))
        return tree;
    delete tree;
    return 0;
}

RTree *IndexFile::getTree(const stdString &name)
{
    long tree_anchor;
    if (!names.find(name, tree_anchor))
        return 0;
    RTree *tree = new RTree(fa, tree_anchor);
    if (tree->reattach())
        return tree;
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
