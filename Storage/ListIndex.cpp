// -*- c++ -*-

// Tools
#include <MsgLogger.h>
// Storage
#include "ListIndex.h"

#undef DEBUG_LISTINDEX

ListIndex::ListIndex()
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
    IndexConfig config;
    SubArchInfo info;
    info.index = 0;
    if (config.parse(filename))
    {
        stdList<stdString>::const_iterator subs;
        for (subs  = config.subarchives.begin();
             subs != config.subarchives.end();    ++subs)
        {      
            info.name = *subs;
            sub_archs.push_back(info);
        }
    }
    else
    {   // Assume a single index, no list of indices.
        info.name = filename;
        sub_archs.push_back(info);
    }
#ifdef DEBUG_LISTINDEX
    printf("ListIndex::open(%s)\n", filename.c_str());
    stdList<SubArchInfo>::const_iterator archs;
    for (archs  = sub_archs.begin();
         archs != sub_archs.end();   ++ archs)
    {
        printf("sub '%s'\n", archs->name.c_str());
    }    
#endif
    this->filename = filename;
    return true;
}

// Close what's open. OK to call if nothing's open.
void ListIndex::close()
{
    stdList<SubArchInfo>::iterator archs;
    for (archs  = sub_archs.begin();
         archs != sub_archs.end();   ++ archs)
    {
        if (archs->index)
        {
            archs->index->close();
            delete archs->index;
            archs->index = 0;
#ifdef DEBUG_LISTINDEX
            printf("Closed sub %s\n", archs->name.c_str());
#endif
        }
    }
#ifdef DEBUG_LISTINDEX
    printf("Closed ListIndex %s\n", filename.c_str());
#endif
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
    stdList<SubArchInfo>::iterator archs = sub_archs.begin();
    while (archs != sub_archs.end())
    {
        if (archs->index == 0)
        {   // Open individual index file
            if (!(archs->index = new IndexFile(50)))
            {                
                LOG_MSG("ListIndex::getTree out of mem\n");
                return 0;
            }
            if (!archs->index->open(archs->name, true))
            {   // can't open this one; drop it from list
                delete archs->index;
                archs = sub_archs.erase(archs);
                continue;
            }
        }
        if ((tree = archs->index->getTree(channel, directory)))
        {
#ifdef DEBUG_LISTINDEX
            printf("Found '%s' in '%s'\n",
                   channel.c_str(), archs->name.c_str());
#endif
            return tree;
        }
#ifdef DEBUG_LISTINDEX
        printf("Didn't find '%s' in '%s'\n",
               channel.c_str(), archs->name.c_str());
#endif
        ++archs;
    }
    // Channel not found anywhere.
    return 0;
}

int sort_compare(const stdString &a,
                 const stdString &b)
{
    return b.compare(a);
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
        stdList<SubArchInfo>::iterator archs = sub_archs.begin();
        while (archs != sub_archs.end())
        {
            if (archs->index == 0)
            {   // Open individual index file
                if (!(archs->index = new IndexFile(50)))
                {                
                    LOG_MSG("ListIndex::getFirstChannel out of mem\n");
                    return 0;
                }
                if (!archs->index->open(archs->name, true))
                {   // can't open this one; drop it from list
                    delete archs->index;
                    archs = sub_archs.erase(archs);
                    continue;
                }
            }
#ifdef DEBUG_LISTINDEX
            printf("Getting names from '%s'\n", archs->name.c_str());
#endif
            NameIterator sub_names;
            bool ok = archs->index->getFirstChannel(sub_names);
            while (ok)
            {
                known_names.add(sub_names.getName());
                ok = archs->index->getNextChannel(sub_names);
            }
            ++archs;
        }
#ifdef DEBUG_LISTINDEX
        printf("Converting to list\n");
#endif
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
#ifdef DEBUG_LISTINDEX
    printf("getFirstChannel: '%s'\n", iter.entry.name.c_str());
#endif
    return true;
}

bool ListIndex::getNextChannel(NameIterator &iter)
{
    if (current_name != names.end())
    {
        iter.entry.name = *current_name;
        ++current_name;
#ifdef DEBUG_LISTINDEX
        printf("getNextChannel: '%s'\n", iter.entry.name.c_str());
#endif
        return true;
    }
    return false;
}

