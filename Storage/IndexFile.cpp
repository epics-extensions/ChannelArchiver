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
{
    cache_hits = cache_misses = 0;
}

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
        tree_cache.clear();
        fa.detach();
        fclose(f);
        f = 0;
    }   
}

// Comparison routine for AVLTree<TreeCacheEntry>
int sort_compare(const TreeCacheEntry &a, const TreeCacheEntry &b)
{   return a.channel.compare(b.channel); }

RTree *IndexFile::addChannel(const stdString &channel)
{
    RTree *tree = getTree(channel);
    if (tree)
        return tree;
    long tree_anchor;
    if (!(tree_anchor = fa.allocate(RTree::anchor_size)))
    {
        LOG_MSG("IndexFile::add_channel(%s): cannot allocate tree anchor\n",
                channel.c_str());
        return false;
    }
    tree = new RTree(fa, tree_anchor);
    if (tree->init() && names.insert(channel, tree_anchor))
    {
        TreeCacheEntry entry;
        entry.channel = channel;
        entry.tree = tree;
        //printf("Adding to cache tree 0x%lX (%s)\n",
        //       (unsigned long)tree, channel.c_str());
        tree_cache.add(entry);
        entry.tree = 0;
       return tree;
    }
    delete tree;
    return 0;
}

RTree *IndexFile::getTree(const stdString &channel)
{
    // Note: TreeCacheEntry deletes its tree,
    // which is fine for those in the cache,
    // but we have to be careful with local
    // copies of the entry.
    RTree *tree;
    TreeCacheEntry entry;
    entry.channel = channel;
    if (tree_cache.find(entry))
    {
        ++cache_hits;
        tree = entry.tree;
        //printf("Got from cache tree 0x%lX (%s)\n",
        //       (unsigned long)tree, channel.c_str());
        entry.tree = 0;
        return tree;
    }
    ++cache_misses;
    long tree_anchor;
    if (!names.find(channel, tree_anchor))
        return 0;
    tree = new RTree(fa, tree_anchor);
    if (tree->reattach())
    {
        entry.tree = tree;
        //printf("Adding to cache tree 0x%lX (%s)\n",
        //       (unsigned long)tree, channel.c_str());
        tree_cache.add(entry);
        entry.tree = 0;
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

void IndexFile::showHashStats(FILE *f)
{
    fprintf(f, "IndexFile Tree Cache hits: %ld, misses: %ld\n",
            (long) cache_hits, (long) cache_misses);
    names.showStats(f);
}
