// System
#include <stdio.h>
#include <string.h>
// Base
#include <epicsVersion.h>
// Tools
#include <AutoPtr.h>
#include <BenchTimer.h>
#include <stdString.h>
#include <Filename.h>
#include <ArchiverConfig.h>
#include <ArgParser.h>
#include <epicsTimeHelper.h>
#include <FUX.h>
// Index
#include <IndexFile.h>

int verbose;

bool add_tree_to_master(const stdString &index_name,
                        const stdString &dirname,
                        const stdString &channel,
                        const RTree *subtree,
                        RTree *index)
{
    RTree::Datablock block;
    RTree::Node node(subtree->getM(), true);
    int idx;
    bool ok;
    stdString datafile, start, end;
    // Xfer data blocks from the end on, going back to the start.
    // Stop when find a block that was already in the master index.
    for (ok = subtree->getLastDatablock(node, idx, block);
         ok;
         ok = subtree->getPrevDatablock(node, idx, block))
    {
        // Data files in master need full path
        if (Filename::containsPath(block.data_filename))
            datafile = block.data_filename;
        else
            Filename::build(dirname, block.data_filename, datafile);
        if (verbose > 2)
            printf("'%s' @ 0x%lX: %s - %s\n",
                   datafile.c_str(), block.data_offset,
                   epicsTimeTxt(node.record[idx].start, start),
                   epicsTimeTxt(node.record[idx].end, end));
        // Note that there's no inner loop over the 'chained'
        // blocks, we only handle the main blocks of each sub-tree.
        switch (index->updateLastDatablock(node.record[idx].start,
                                           node.record[idx].end,
                                           block.data_offset, datafile))
        {
            case RTree::YNE_Error:
                fprintf(stderr,
                        "Error inserting datablock for '%s' into '%s'\n",
                        channel.c_str(), index_name.c_str());
                return false;
            case RTree::YNE_No:
                if (verbose > 2)
                    printf("Block already existed, skipping the rest\n");
                return true;
            case RTree::YNE_Yes:
                continue; // insert more blocks
        }
    }
    return true;
}

bool parse_config(const stdString &config_name,
                  stdList<stdString> &subarchives)
{
    FUX fux;
    FUX::Element *e, *doc = fux.parse(config_name.c_str());
    if (!(doc && doc->name == "indexconfig"))
    {
        fprintf(stderr, "Cannot parse '%s'\n", config_name.c_str());
        return false;
    }
    stdList<FUX::Element *>::const_iterator els;
    for (els=doc->children.begin(); els!=doc->children.end(); ++els)
        if ((e = (*els)->find("index")))
            subarchives.push_back(e->value);
    return true;
}

bool create_masterindex(int RTreeM,
                        const stdString &config_name,
                        const stdString &index_name)
{
    stdList<stdString> subarchives;
    if (!parse_config(config_name, subarchives))
        return false;
    IndexFile::NameIterator names;
    IndexFile index(RTreeM), subindex(RTreeM);
    bool ok;
    if (!index.open(index_name, false))
    {
        fprintf(stderr, "Cannot create master index file '%s'\n",
                index_name.c_str());
        return false;
    }
    if (verbose)
        printf("Opened master index '%s'.\n", index_name.c_str());
    stdList<stdString>::const_iterator subs;
    for (subs = subarchives.begin(); subs != subarchives.end(); ++subs)
    {
        const stdString &sub_name = *subs;
        if (!subindex.open(sub_name))
        {
            fprintf(stderr, "Cannot open sub-index '%s'\n", sub_name.c_str());
            continue;
        }
        if (verbose)
            printf("Adding sub-index '%s'\n", sub_name.c_str());
        for (ok = subindex.getFirstChannel(names);
             ok;
             ok = subindex.getNextChannel(names))
        {
            const stdString &channel = names.getName();
            if (verbose > 1)
                printf("'%s'\n", channel.c_str());
            AutoPtr<RTree> subtree(subindex.getTree(channel));
            if (!subtree)
            {
                fprintf(stderr, "Cannot get tree for '%s' from '%s'\n",
                        channel.c_str(), sub_name.c_str());
                continue;
            }
            AutoPtr<RTree> tree(index.addChannel(channel));
            if (!tree)
            {
                fprintf(stderr, "Cannot add '%s' to '%s'\n",
                        channel.c_str(), index_name.c_str());
                continue;
            }
            add_tree_to_master(index_name,
                               subindex.getDirectory(),
                               channel, subtree, tree);
        }
        subindex.close();
        if (verbose > 2)
            index.showStats(stdout);
    }
    index.close();
    if (verbose)
        printf("Closed master index '%s'.\n", index_name.c_str());
    return true;
}

int main(int argc, const char *argv[])
{
    initEpicsTimeHelper();
    CmdArgParser parser(argc, argv);
    parser.setHeader("ArchiveIndexTool version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    parser.setArgumentsInfo("<archive list file> <output index>");
    CmdArgFlag help  (parser, "help", "Show Help");
    CmdArgInt RTreeM (parser, "M", "<3-100>", "RTree M value");
    CmdArgInt verbose_flag (parser, "verbose", "<level>", "Show more info");
    RTreeM.set(50);
    if (! parser.parse())
        return -1;
    verbose = verbose_flag;
    if (help)
    {
        printf("This tools reads an archive list file,\n");
        printf("that is a file listing sub-archives,\n");
        printf("and generates a master index for all of them\n");
        printf("in the output index\n");
        return 0;
    }
    if (parser.getArguments().size() != 2)
    {
        parser.usage();
        return -1;
    }
    BenchTimer timer;
    if (!create_masterindex(RTreeM, parser.getArgument(0),
                            parser.getArgument(1)))
        return -1;
    if (verbose > 0)
    {
        timer.stop();
        printf("Time: %s\n", timer.toString().c_str());
    }
    
    return 0;
}






