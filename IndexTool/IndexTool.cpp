// System
#include <signal.h>
// Base
#include <epicsVersion.h>
// Tools
#include <AutoPtr.h>
#include <BenchTimer.h>
#include <Filename.h>
#include <ArgParser.h>
#include <IndexConfig.h>
#include <Lockfile.h>
// Local
#include "IndexInfo.h"
#include "MasterIndex.h"

volatile bool run = true;

static void signal_handler(int sig)
{
    run = false;
    fprintf(stderr, "Exiting on signal %d\n", sig);
}

int main(int argc, const char *argv[])
{
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    
    CmdArgParser parser(argc, argv);
    parser.setHeader("ArchiveIndexTool version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    parser.setArgumentsInfo("{archive list file/old index} {index}");
    parser.setFooter("\nThis tools reads an archive list file, "
                     "that is a file listing sub-archives,\n"
                     "and generates a master index for all of them "
                     "in the output index.\n"
                     "With the -reindex flag, it creates a new index "
                     "from an existing index\n"
                     "(shortcut for creating a list file that contains only "
                     " a single old index).\n"
                     "It can also simply display information about an existing index.\n");
    CmdArgFlag    help      (parser, "help", "Show Help");
    CmdArgInt     RTreeM    (parser, "M", "<3-100>", "RTree M value (default: 50 for 'full', 3 for 'shallow')");
    CmdArgFlag    shallow   (parser, "shallow", "Build shallow master index");
    CmdArgFlag    reindex   (parser, "reindex", "Build new index from old index (no list file)");
    CmdArgFlag    info      (parser, "info", "Show index info (no output index)");
    CmdArgInt     verbosity (parser, "verbose", "<level 0-10>", "Show more info");
    CmdArgString  channel   (parser, "channel", "<name>", "Show info for single channel");
 
    if (!parser.parse()  ||  help)
    {
        parser.usage();
        return -1;
    }

    if (RTreeM.get() == 0)
        RTreeM.set(shallow ? 3 : 50);
    if (channel.isSet())
        info.set();
    if (info)
    {
        if (parser.getArguments().size() != 1)
        {
            fprintf(stderr, "Missing <index> file\n");
            parser.usage();
            return -1;
        }
        IndexInfo(parser.getArgument(0), verbosity, channel);
        return 0;
    }

    if (parser.getArguments().size() != 2)
    {
        if (reindex)
            fprintf(stderr, "Missing <old index> and <output index>.\n");
        else
            fprintf(stderr, "Missing <archive list file> and <output index>.\n");
        parser.usage();
        return -1;
    }
    stdString index_config = parser.getArgument(0);
    stdString new_index_name = parser.getArgument(1);
    if (Filename::containsPath(new_index_name))
    {
        fprintf(stderr, "The new index file name, '%s',"
                " must not contain any path information,\n",
                new_index_name.c_str());
        fprintf(stderr, "it has to refer to a file in the current directory.\n");
        return -1;
    }
    try
    {
        Lockfile lock_file("indextool_active.lck", argv[0]);
        BenchTimer timer;
        
        IndexConfig config;
        if (reindex) // config_name is really the one and only index name
            config.subarchives.push_back(index_config);
        else
            if (!config.parse(index_config))
                throw GenericException(__FILE__, __LINE__,
                                       "File '%s' is no XML indexconfig",
                                       index_config.c_str());
        AutoPtr<MasterIndex> master;
        if (shallow)
            master = new ShallowIndex(new_index_name, RTreeM, config.subarchives, verbosity);
        else
            master = new MasterIndex(new_index_name, RTreeM, config.subarchives, verbosity);
        master->create();
        timer.stop();
        if (verbosity.get() > 0)
            printf("Time: %s\n", timer.toString().c_str());
    }
    catch (GenericException &e)
    {
        printf("Error:\n%s\n", e.what());
        return -1;
    }
    return 0;
}

