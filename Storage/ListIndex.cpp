// -*- c++ -*-

// Tools
#include <MsgLogger.h>
#include <Filename.h>
#include <IndexConfig.h>
#include <AVLTree.h>
// Storage
#include "RTree.h"
#include "ListIndex.h"

#define USE_AUTO_INDEX

#ifdef USE_AUTO_INDEX
#include "AutoIndex.h"
#else
#include "IndexFile.h"
#endif

// #define DEBUG_LISTINDEX

Index *ListIndex::SubArchInfo::openIndex()
{
    if (!index)
    {   // Open individual index file
        try
        {
#ifdef USE_AUTO_INDEX
            index = new AutoIndex();
#else
            index = new IndexFile();
#endif
        }
        catch (...)
        {
            throw GenericException(__FILE__, __LINE__,
                "ListIndex: No mem for '%s'", name.c_str());
        }
        try
        {
            index->open(name, Index::ReadOnly);
        }
        catch (GenericException &e)
        {
            // can't open this one; ignore error, drop it from list
            LOG_MSG("Error opening '%s':\n%s",
                    name.c_str(), e.what());
            delete index;
            index = 0;
        }
    }
    return index;
}

void ListIndex::SubArchInfo::closeIndex()
{
    if (index)
    {
        index->close();
        index = 0;
    }
}

ListIndex::~ListIndex()
{
    close();
}

void ListIndex::open(const stdString &filename, ReadWrite readwrite)
{
    setFilename(filename);
    if (readwrite != ReadOnly)
        throw GenericException(__FILE__, __LINE__,
                               "ListIndex '%s' Writing is not supported!\n",
                               filename.c_str());
    IndexConfig config;
    stdList<stdString>::const_iterator subs;
    if (! config.parse(filename))
    {
#ifdef DEBUG_LISTINDEX
        printf("ListIndex::open(%s): No valid indexconfig\n", filename.c_str());
#endif
        throw GenericException(__FILE__, __LINE__,
                               "ListIndex cannot parse '%s'.\n",
                               filename.c_str());
    }
#ifdef DEBUG_LISTINDEX
    printf("ListIndex::open(%s): Parsed indexconfig\n", filename.c_str());
#endif
    for (subs  = config.subarchives.begin();
         subs != config.subarchives.end();    ++subs)
    {
        // Check if we need to resolve sub-index name relative to filename
        if (getDirectory().empty()  ||  Filename::containsFullPath(*subs))
            sub_archs.push_back(SubArchInfo(*subs));
        else
        {
            stdString subname;
            Filename::build(getDirectory(), *subs, subname);
            sub_archs.push_back(SubArchInfo(subname));
        }
    }
#ifdef DEBUG_LISTINDEX
    printf("ListIndex::open(%s)\n", filename.c_str());
    stdList<SubArchInfo>::const_iterator archs;
    for (archs  = sub_archs.begin();
         archs != sub_archs.end();   ++ archs)
    {
        printf("sub '%s'\n", archs->getName().c_str());
    }    
#endif
    this->filename = filename;
}

// Close what's open. OK to call if nothing's open.
void ListIndex::close()
{
    if (filename.length() > 0)
    {
        stdList<SubArchInfo>::iterator archs;
        for (archs  = sub_archs.begin();
             archs != sub_archs.end();   ++ archs)
        {
            archs->closeIndex();
        }
        sub_archs.clear();
#ifdef DEBUG_LISTINDEX
        printf("Closed ListIndex %s\n", filename.c_str());
#endif
        filename.assign(0, 0);
    }
}
    
Index::Result *ListIndex::addChannel(const stdString &channel)
{
    throw GenericException(__FILE__, __LINE__,
                           "ListIndex: Tried to add '%s'",
                           channel.c_str());
}

Index::Result *ListIndex::findChannel(const stdString &channel)
{
    stdList<SubArchInfo>::iterator archs = sub_archs.begin();
    while (archs != sub_archs.end())
    {
        Index *index = archs->openIndex();
        if (! index)
        {   // can't open this one; ignore error, drop it from list
            LOG_MSG("Listindex '%s': Error opening '%s'\n",
                filename.c_str(), archs->getName().c_str());
            archs = sub_archs.erase(archs);
            continue;
        }
        AutoPtr<Index::Result> result(index->findChannel(channel));
#       ifdef DEBUG_LISTINDEX
        printf("%s '%s' in '%s'\n",
               (result ? "Found" : "Didn't find"),
               channel.c_str(), archs->getName().c_str());
#       endif
        if (result)
            return result.release();
        // else: try next sub-archive
        ++archs;
    }
    // Channel not found anywhere.
    return 0;
}

int sort_compare(const stdString &a, const stdString &b)
{
    return b.compare(a);
}

void ListIndex::name_traverser(const stdString &name, void *self)
{
    ListIndex *me = (ListIndex *)self;
#ifdef DEBUG_LISTINDEX
    printf("Got '%s'\n", name.c_str());
#endif
    
    me->names.push_back(name);
}

/** A NameIterator for a list of channel names in a std. string list. */
class ListIndexNameIterator : public Index::NameIterator
{
public:
    ListIndexNameIterator(const stdList<stdString> &names);
    virtual bool isValid() const;
    virtual const stdString &getName() const;
    virtual void  next();
private:
    const stdList<stdString> &names;
    stdList<stdString>::const_iterator name;
};

ListIndexNameIterator::ListIndexNameIterator(const stdList<stdString> &names)
    : names(names), name(names.begin())
{}

bool ListIndexNameIterator::isValid() const
{   return name != names.end();   }

const stdString &ListIndexNameIterator::getName() const
{
    LOG_ASSERT(isValid());
    return *name;
}

void ListIndexNameIterator::next()
{
    LOG_ASSERT(isValid());
    ++name;
}

Index::NameIterator *ListIndex::iterator()
{
    if (names.empty())
        collectNames();
    if (names.empty())
        return 0;
    return new ListIndexNameIterator(names);
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
// use both (temporary) AVLTree and stdList.
void ListIndex::collectNames()
{
    // Pull all names into AVLTree
    AVLTree<stdString> known_names;
    stdList<SubArchInfo>::iterator archs = sub_archs.begin();
    while (archs != sub_archs.end())
    {
        Index *index = archs->openIndex();
        if (!index)
       {   // can't open this one; ignore error, drop it from list
            LOG_MSG("Listindex '%s':\n- Error opening '%s'\n",
                     filename.c_str(), archs->getName().c_str());
            archs = sub_archs.erase(archs);
            continue;
        }
#ifdef DEBUG_LISTINDEX
        printf("Getting names from '%s'\n", archs->getName().c_str());
#endif
        AutoPtr<NameIterator> sub_names(index->iterator());
        while (sub_names)
        {
            known_names.add(sub_names->getName());
            sub_names->next();
            if (!sub_names->isValid())
                sub_names = 0;
        }
        ++archs;
    }
#ifdef DEBUG_LISTINDEX
    printf("Converting to list\n");
#endif
    known_names.traverse(name_traverser, this);
}

