// -*- c++ -*-

// Tools
#include <MsgLogger.h>
// Storage
#include "ListIndex.h"

#undef DEBUG_LISTINDEX

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
        LOG_MSG("ListIndex '%s' Writing is not supported!\n",
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

// Close what's open. OK to call if nothing's open.
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
    {   // Try open index first
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
        printf("Checking '%s' for '%s'\n",
               subs->c_str(), channel.c_str());
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

static int sort_compare(const stdString &a,
                        const stdString &b)
{
    return a.compare(b);
}

void ListIndex::name_traverser(const stdString &name,
                               void *self)
{
    ListIndex *me = (ListIndex *)self;
    me->names.push_back(name);
}

// As soon as the _first_ channel is requested,
// we actually have to query every sub archive
// for every channel.
// That bites, but what else could one do?
// In the process, we might run into the same
// channel more than once
// -> need an efficient structure for checking
//    if we've already seen that channel
// Later, we want to iterate over the channels
// -> need a list
// Ideal would be e.g. an AVL tree that supports
// iteration. Don't have that at hand, so I
// use both Tools/AVLTree (temporary in getFirstChannel)
// and stdList.
bool ListIndex::getFirstChannel(NameIterator &iter)
{
    if (names.empty())
    {
        // Pull all names into AVLTree
        AVLTree<stdString> known_names;
        bool ok;
        close(); // in case there was an open index...
        stdList<stdString>::const_iterator subs;
        NameIterator sub_names;
        for (subs  = config.subarchives.begin();
             subs != config.subarchives.end();
             ++subs)
        {
#ifdef DEBUG_LISTINDEX
            printf("Getting names from '%s'\n", subs->c_str());
#endif
            if (!index.open(*subs, true))
                continue; // can't open
            ok = index.getFirstChannel(sub_names);
            while (ok)
            {
                known_names.add(sub_names.getName());
                ok = index.getNextChannel(sub_names);
            }
            index.close();
        }
        // Copy dump tree into list
        known_names.traverse(name_traverser, this);
    }
    if (names.empty())
        return false;
    current_name = names.begin();
    iter.entry.name = *current_name;
    iter.entry.ID = 0;
    iter.entry.next = 0;
    iter.entry.offset = 0;
    iter.hashvalue = 0;
    ++current_name;
    return true;
}

bool ListIndex::getNextChannel(NameIterator &iter)
{
    if (current_name != names.end())
    {
        iter.entry.name = *current_name;
        ++current_name;
        return true;
    }
    return false;
}

