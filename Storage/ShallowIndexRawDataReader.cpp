// RawDataReader.cpp

// Tools
#include "MsgLogger.h"
#include "Filename.h"
// Storage
#include "ShallowIndexRawDataReader.h"
#include "RawDataReader.h"
#include "IndexFile.h"

#define DEBUG_DATAREADER

ShallowIndexRawDataReader::ShallowIndexRawDataReader(Index &index)
        : top_index(index)
{
    LOG_ASSERT(top_index.getFilename().length() > 0);
}

ShallowIndexRawDataReader::~ShallowIndexRawDataReader()
{
}

const RawValue::Data *ShallowIndexRawDataReader::find(
             const stdString &channel_name, const epicsTime *start)
{
    // Start at the top index
    return find(top_index, top_result, top_datablock, channel_name, start);
}

const RawValue::Data *ShallowIndexRawDataReader::find(
                       Index &index,
                       AutoPtr<Index::Result> &index_result,
                       AutoPtr<RTree::Datablock> &datablock,
                       const stdString &channel_name, const epicsTime *start)
{
    // Lookup channel in index
    index_result = index.findChannel(channel_name);
    if (! index_result)
        return 0; // Channel not found
    try
    {
        // Get 1st data block
        if (start)
            datablock = index_result->getRTree()->search(*start);
        else
            datablock = index_result->getRTree()->getFirstDatablock();
        if (! datablock)  // No values for this time in index
            return 0;
        LOG_ASSERT(datablock->isValid());
        if (datablock->getDataOffset() > 0)
        {   // name and offset: Must be data file and block offset.
#           ifdef DEBUG_DATAREADER
            stdString range = datablock->getInterval().toString();
            printf("- Index '%s' has 'full' entry: %s @ 0x%lX: %s\n",
                   index.getFullName().c_str(),
                   datablock->getDataFilename().c_str(),
                   (unsigned long)datablock->getDataOffset(),
                   range.c_str());
#           endif
            reader = new RawDataReader(index);
            return reader->find(channel_name, start);
        }
        else
        {   // Zero offset: Got name of sub-index file.
            const stdString &sub_index_name = datablock->getDataFilename();
#           ifdef DEBUG_DATAREADER
            stdString range = datablock->getInterval().toString();
            printf("- Index '%s' has 'shallow' entry: %s @ 0x%lX: %s\n",
                   index.getFullName().c_str(),
                   sub_index_name.c_str(),
                   (unsigned long)datablock->getDataOffset(),
                   range.c_str());
#           endif
            // Recursion keeps re-using the sub_index, sub_result, ...,
            // but the last sub_index, the one that'll be used by the
            // RawDataReader, stays open this way.
            if (! sub_index)
                sub_index = new IndexFile();
            // Check if the sub index name is relative...
            if (sub_index_name[0] == '/')
            {   // Full path name
#           ifdef DEBUG_DATAREADER
                printf("- opening as plain '%s'\n", sub_index_name.c_str());
#           endif
                sub_index->open(sub_index_name);
            }
            else
            {   // prepend directory of this index
                stdString full_name;
                Filename::build(index_result->getDirectory(), sub_index_name,
                                full_name);
#           ifdef DEBUG_DATAREADER
                printf("- opening as full '%s'\n", full_name.c_str());
#           endif
                sub_index->open(full_name);
            }
            return find(*sub_index, sub_result, sub_datablock,
                        channel_name, start);
        }
    }
    catch (GenericException &e)
    {  // Add channel name to the message
        throw GenericException(__FILE__, __LINE__,
                               "Index '%s', Channel '%s':\n%s",
                               index.getFilename().c_str(),
                               channel_name.c_str(), e.what());
    }
    return 0;
}

const RawValue::Data *ShallowIndexRawDataReader::next()
{
    LOG_ASSERT(reader);
    // If we're in a 'full' index, just let the underlying reader
    // handle everything.
    const RawValue::Data *sample = reader->next();
    if (!sub_index)
        return sample;

    // With a 'shallow' top-level index, we need to search again
    // as soon as we leave the time range of the current data block,
    // since the next data block might point to a different sub-archive.
    if (sample  &&
        RawValue::getTime(sample)  <=  top_datablock->getInterval().getEnd())
        return sample;

    // Either the underlying reader is 'done' (sample==0),
    // or it ran beyond what we intended to get from that sub-archive.
#   ifdef DEBUG_DATAREADER
    printf("- ShallowIndexRawDataReader reached end of block in '%s': %s\n",
           top_datablock->getDataFilename().c_str(),
           top_datablock->getInterval().toString().c_str());
#   endif
    // See if the top index has a next data block
    if (top_datablock->getNextDatablock())
    {
        const stdString &index_name = top_datablock->getDataFilename();
#       ifdef DEBUG_DATAREADER
        printf("- next sub-archive data in '%s': %s\n",
               index_name.c_str(),
               top_datablock->getInterval().toString().c_str());
#       endif

        // Get copy of name since 'reader' gets updated in find()
        stdString channel_name = reader->getName();
        epicsTime start = top_datablock->getInterval().getStart();
        // Search again from the top, but now with a new 'start' time.
        sample = find(top_index, top_result, top_datablock,
                      channel_name, &start);
        {
            stdString t;
            printf("- Got sample stamped %s...\n",
                   epicsTimeTxt(RawValue::getTime(sample), t));
        }
        while (sample  &&  RawValue::getTime(sample) < start)
        {
#           ifdef DEBUG_DATAREADER
            stdString t;
            printf("- Skipping sample stamped %s...\n",
                   epicsTimeTxt(RawValue::getTime(sample), t));
#           endif
            sample = next();
        }
    }
    // else: No more data. Just continue in current reader.
    
    return sample;
}   

// Pass all the other interfaces through to underlying reader
const stdString &ShallowIndexRawDataReader::getName() const
{
    LOG_ASSERT(reader);
    return reader->getName();
}

const RawValue::Data *ShallowIndexRawDataReader::get() const
{
    LOG_ASSERT(reader);
    return reader->get();
}

DbrType ShallowIndexRawDataReader::getType() const
{
    LOG_ASSERT(reader);
    return reader->getType();
}
    
DbrCount ShallowIndexRawDataReader::getCount() const
{
    LOG_ASSERT(reader);
    return reader->getCount();
}
    
const CtrlInfo &ShallowIndexRawDataReader::getInfo() const
{
    LOG_ASSERT(reader);
    return reader->getInfo();
}

bool ShallowIndexRawDataReader::changedType()
{
    LOG_ASSERT(reader);
    return reader->changedType();
}

bool ShallowIndexRawDataReader::changedInfo()
{
    LOG_ASSERT(reader);
    return reader->changedInfo();
}
    
