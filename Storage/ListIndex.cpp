// -*- c++ -*-

// Tools
#include <MsgLogger.h>
// Storage
#include "ListIndex.h"

#define DEBUG_LISTINDEX

ListIndex::ListIndex()  :  index(50), is_open(false)
{
#ifdef DEBUG_LISTINDEX
    printf("new ListIndex\n");
#endif
}

bool ListIndex::open(const stdString &filename, bool readonly)
{
    if (!readonly)
    {
        LOG_MSG("ListIndex '%s': Only read-only support\n",
                filename.c_str());
        return false;
    }
#ifdef DEBUG_LISTINDEX
    printf("ListIndex::open(%s)\n", filename.c_str());
#endif
    if (config.parse(filename))
    {
#ifdef DEBUG_LISTINDEX
        stdList<stdString>::const_iterator subs;
        for (subs  = config.subarchives.begin();
             subs != config.subarchives.end(); ++subs)            
            printf("sub '%s'\n", subs->c_str());
#endif  
        return true;
    }
#ifdef DEBUG_LISTINDEX
    printf("sub '%s'\n", filename.c_str());
#endif
    config.subarchives.push_back(filename);
    return true;
}

void ListIndex::close()
{
    if (!is_open)
        return;
#ifdef DEBUG_LISTINDEX
    printf("Closing %s\n", filename.c_str());
#endif
    index.close();
    is_open = false;
    filename.assign(0, 0);
}
    
class RTree *ListIndex::addChannel(const stdString &channel,
                                   stdString &directory)
{
    LOG_MSG("ListIndex: Tried to add '%s'\n", channel.c_str());
    return 0;
}

class RTree *ListIndex::getTree(const stdString &channel,
                                stdString &directory)
{
    RTree *tree;
    if (is_open)
    {
        tree = index.getTree(channel, directory);
        if (tree)
        {
#ifdef DEBUG_LISTINDEX
            printf("Found '%s' in '%s' (already open)\n",
                   channel.c_str(), filename.c_str());
#endif
            return tree;
        }   
    }
    close(); // in case there was an open index...
    // Find suitable sub-archive
    stdList<stdString>::const_iterator subs;
    for (subs  = config.subarchives.begin();
         subs != config.subarchives.end();
         ++subs)
    {
#ifdef DEBUG_LISTINDEX
        printf("Checking '%s' for '%s'\n", subs->c_str(), channel.c_str());
#endif
        if (!index.open(*subs, true))
            continue; // can't open
        tree = index.getTree(channel, directory);
        if (tree)
        {
#ifdef DEBUG_LISTINDEX
            printf("Bingo!\n");
#endif
            is_open = true;
            filename = *subs;
            return tree;
        }
        // Channel not in this one; close & search on
        index.close();
    }
    // Channel not found anywhere.
    return 0;
}

bool ListIndex::getFirstChannel(NameIterator &iter)
{
    return false;
}

bool ListIndex::getNextChannel(NameIterator &iter)
{
    return false;
}

