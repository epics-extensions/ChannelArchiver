// -*- c++ -*-

// Tools
#include <MsgLogger.h>
#include <Filename.h>
#include <IndexConfig.h>
#include <AutoFilePtr.h>
// Storage
#include "AutoIndex.h"
#include "IndexFile.h"
#include "ListIndex.h"

// #define DEBUG_AUTOINDEX

AutoIndex::~AutoIndex()
{
    close();
}

void AutoIndex::open(const stdString &filename, ReadWrite readwrite)
{
#ifdef DEBUG_AUTOINDEX
    printf("AutoIndex::open(%s)\n", filename.c_str());
#endif
    setFilename(filename);
    if (readwrite != ReadOnly)
        throw GenericException(__FILE__, __LINE__,
                               "AutoIndex '%s' Writing is not supported!\n",
                               filename.c_str());
    // Try to open as ListIndex
    try
    {
        index = new ListIndex();
    }
    catch (...)
    {
        throw GenericException(__FILE__, __LINE__, "AutoIndex: No mem for");
    }
    try
    {
        index->open(filename, readwrite);
#ifdef DEBUG_AUTOINDEX
        printf("AutoIndex(%s) -> ListIndex\n", filename.c_str());
#endif
        return;
    }
    catch (GenericException &e)
    {   // can't open as list; ignore.
        index = 0;
    }

    // Try to open as IndexFile
    try
    {
        index = new IndexFile();
#ifdef DEBUG_AUTOINDEX
        printf("AutoIndex(%s) -> IndexFile\n", filename.c_str());
#endif
    }
    catch (...)
    {       
        throw GenericException(__FILE__, __LINE__, "AutoIndex: No mem");
    }
    index->open(filename, readwrite);
#ifdef DEBUG_AUTOINDEX
    printf("AutoIndex(%s) -> IndexFile\n", filename.c_str());
#endif
}

// Close what's open. OK to call if nothing's open.
void AutoIndex::close()
{
    if (index)
    {
        index = 0;
#ifdef DEBUG_AUTOINDEX
        printf("AutoIndex::close(%s)\n", getFilename().c_str());
#endif
        clearFilename();
    }
}
    
Index::Result *AutoIndex::addChannel(const stdString &channel)
{
    throw GenericException(__FILE__, __LINE__,
                           "AutoIndex: Tried to add '%s'",
                           channel.c_str());
}

Index::Result *AutoIndex::findChannel(const stdString &channel)
{
    return index->findChannel(channel);
}

Index::NameIterator *AutoIndex::iterator()
{
    return index->iterator();
}

