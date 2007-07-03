// Tools
#include <AutoPtr.h>
#include <Filename.h>
#include <epicsTimeHelper.h>
#include <IndexFile.h>
// Local
#include "MasterIndex.h"

extern volatile bool run;

MasterIndex::~MasterIndex()
{}

void MasterIndex::create()
{
    IndexFile index(RTreeM);
    index.open(index_name, Index::ReadAndWrite);
    if (verbosity)
        printf("Opened master index '%s'.\n", index_name.c_str());
    stdList<stdString>::const_iterator subs;
    for (subs  = subarchives.begin();
         run && subs != subarchives.end(); ++subs)
    {
        try
        {
            addSubindex(index, *subs);
            if (verbosity > 2)
                index.showStats(stdout);
        }
        catch (GenericException &ex)
        {
            fprintf(stderr, "Error while processing sub-index '%s':\n%s\n",
                    (*subs).c_str(), ex.what());
        }
    }
    index.close();
    if (verbosity)
        printf("Closed master index '%s'.\n", index_name.c_str());
}

void MasterIndex::addSubindex(Index &index, const stdString &sub_name)
{
    IndexFile subindex(RTreeM);
    subindex.open(sub_name);
    if (verbosity)
    {
        printf("Adding sub-index '%s'\n", sub_name.c_str());
        fflush(stdout);
    }
    AutoPtr<IndexFile::NameIterator> name(subindex.iterator());
    for (/**/;
         run  &&  name  &&  name->isValid();
         name->next())
    {
        const stdString &channel = name->getName();
        try
        {
            if (verbosity > 1)
                printf("'%s'\n", channel.c_str());
            AutoPtr<Index::Result> src_info(subindex.findChannel(channel));
            if (!src_info)
            {
                fprintf(stderr, "Cannot get tree for '%s' from '%s'\n",
                        channel.c_str(), subindex.getFullName().c_str());
                continue;
            }
            AutoPtr<Index::Result> dest_info(index.addChannel(channel));
            if (!dest_info)
            {
                fprintf(stderr, "Cannot add '%s' to '%s'\n",
                        channel.c_str(), index.getFullName().c_str());
                continue;
            }
            addChannelInfo(channel, index, dest_info, subindex, src_info);
        }
        catch (GenericException &e)
        {
            fprintf(stderr,
                    "Error adding channel '%s' from '%s' to '%s':\n%s\n",
                    channel.c_str(),
                    subindex.getFullName().c_str(),
                    index.getFullName().c_str(), e.what());
        }
    }
    subindex.close();
}

void MasterIndex::addChannelInfo(const stdString &channel,
                                 Index &index, Index::Result *dest_info,
                                 Index &subindex, Index::Result *src_info)
{
    // Xfer data blocks from the end on, going back to the start.
    // Stop when finding a block that was already in the target index.
    AutoPtr<RTree::Datablock> block(src_info->getRTree()->getLastDatablock());
    for (/**/;    run  &&  block  &&  block->isValid();
         block->getPrevDatablock())
    {
        stdString datafile;
        // Data files in master need a full path ....
        if (Filename::containsFullPath(block->getDataFilename()))
            datafile = block->getDataFilename();
        else // or are written relative to the dir. of the index file
            Filename::build(src_info->getDirectory(),
                            block->getDataFilename(), datafile);
        if (verbosity > 3)
            printf("Inserting '%s' as '%s'\n",
                   block->getDataFilename().c_str(), datafile.c_str());
        if (verbosity > 2)
        {
            stdString start, end;
            printf("'%s' @ 0x%lX: %s - %s\n",
                   datafile.c_str(),
                   (unsigned long)block->getDataOffset(),
                   epicsTimeTxt(block->getInterval().getStart(), start),
                   epicsTimeTxt(block->getInterval().getEnd(), end));
        }
        // Note that there's no inner loop over the 'chained'
        // blocks, we only handle the main blocks of each sub-tree.
        if (!dest_info->getRTree()->updateLastDatablock(
                block->getInterval(), block->getDataOffset(), datafile))
        {
            if (verbosity > 2)
                 printf("Found existing block, skipping the rest\n");
            return;
        }
    }
}

void ShallowIndex::addChannelInfo(const stdString &channel,
                                  Index &index, Index::Result *dest_info,
                                  Index &subindex, Index::Result *src_info)
{
    // Add only the overall time range and the source index name to the
    // destination index, not every data block
    Interval range = src_info->getRTree()->getInterval();
    const stdString &index_name = subindex.getFullName();
    if (verbosity > 2)
        printf("'%s': %s\n", index_name.c_str(), range.toString().c_str());
    dest_info->getRTree()->updateLastDatablock(range, 0, index_name);
}

