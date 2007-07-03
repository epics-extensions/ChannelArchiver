
// Tools
#include <AutoPtr.h>

// Local
#include "IndexInfo.h"

extern volatile bool run;

void IndexInfo::show(const stdString &channel)
{
    try
    {
        printf("Index '%s':\n", index_name.c_str());
        IndexFile index;
        index.open(index_name);
        if (channel.length() > 0)
            showChannel(index, channel);
        else
        {
            size_t name_count = 0;
            AutoPtr<Index::NameIterator> names(index.iterator());
            while (run && names && names->isValid())
            {
                ++name_count;
                showChannel(index, names->getName());
                names->next();
            }
            printf("%zd Channels\n", name_count);
        }
    }
    catch (GenericException &ex)
    {
        fprintf(stderr, "Exception: %s\n", ex.what());
    }
}

void IndexInfo::showChannel(Index &index, const stdString &channel)
{
    if (verbosity > 0)
        printf("Channel '%s'\n", channel.c_str());
    if (verbosity > 1)
        showBlocks(index, channel);
}

void IndexInfo::showBlocks(Index &index, const stdString &channel)
{
    AutoPtr<Index::Result> result(index.findChannel(channel));
    if (! result)
    {
        printf("    No RTree\n");
        return;
    }
    printf("    RTree 'M': %d\n", result->getRTree()->getM());
    
    AutoPtr<RTree::Datablock> block(result->getRTree()->getFirstDatablock());
    while (run &&  block  &&  block->isValid())
    {
        printf("    [ '%s' @ 0x%lX ]",
               block->getDataFilename().c_str(),
               (unsigned long) block->getDataOffset());
        if (verbosity > 2)
            printf("    %s\n",
                   block->getInterval().toString().c_str());
        else
            printf("\n");
        if (verbosity > 3)
        {
            while (run &&  block->getNextChainedBlock())
            {
                printf("    +--[ '%s' @ 0x%lX ]\n",
                       block->getDataFilename().c_str(),
                       (unsigned long) block->getDataOffset());
            }
        }
        block->getNextDatablock();
    }
}
