// System
#include <stdio.h>
// Base
#include <epicsVersion.h>
// Tools
#include "ArchiverConfig.h"
#include "ArgParser.h"
#include "epicsTimeHelper.h"
// Storage
#include "DirectoryFile.h"
#include "DataFile.h"
// rtree
#include "archiver_index.h"

bool verbose;

void convert_dir_index(const stdString &dir_name, const stdString &index_name)
{
    DirectoryFile dir(dir_name);
    DirectoryFileIterator channels = dir.findFirst();
    archiver_Index index;
    
    if (verbose)
        printf("Opened directory file '%s'\n", dir_name.c_str());

    if (!index.create(index_name.c_str()))
    {
        fprintf(stderr, "Cannot create index '%s'\n",
                index_name.c_str());
        return;
    }
    if (verbose)
        printf("Created index '%s'\n", index_name.c_str());
    
    while (channels.isValid())
    {
        if (verbose)
            printf("Channel '%s':\n", channels.entry.data.name);

        DataFile *datafile =
            DataFile::reference(dir.getDirname(),
                                channels.entry.data.first_file, false);
        DataHeader *header =
            datafile->getHeader(channels.entry.data.first_offset);
        datafile->release();
        while (header && header->isValid())
        {
            if (verbose)
            {
                stdString start, end;
                epicsTime2string(header->data.begin_time, start);
                epicsTime2string(header->data.end_time, end);    
                printf("'%s' @ 0x0%lX: %s - %s\n",
                       header->datafile->getBasename().c_str(),
                       header->offset,
                       start.c_str(),
                       end.c_str());
            }
            archiver_Unit au = archiver_Unit(
                key_Object(header->datafile->getBasename().c_str(),
                           header->offset),
                interval(header->data.begin_time,
                         header->data.end_time), 1);
            if (!index.addAU(channels.entry.data.name, au))
            {
                fprintf(stderr, "addAU failed\n");
                break;
            }
            header->read_next();
        }
        delete header;
                
        channels.next();
    }
    DataFile::close_all();
}

void dump_index(const stdString &index_name)
{
    archiver_Index index;
    if (!index.open(index_name.c_str()))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return;
    }
    channel_Name_Iterator *names = index.getChannelNameIterator();
    if (!names)
    {
        fprintf(stderr, "Cannot get name iterator for index '%s'\n",
                index_name.c_str());
        return;
    }
    char name[CHANNEL_NAME_LENGTH];
    key_AU_Iterator *aus;
    bool ok = names->getFirst(name);
    key_Object key;
    interval iv, key_iv;
    stdString start, end;
    while (ok)
    {
        printf("Channel '%s':\n", name);
        if (!index.getEntireIndexedInterval(name, &iv))
        {
            fprintf(stderr, "Cannot get interval for channel '%s'\n", name);
        }       
        else
        {
             epicsTime2string(iv.getStart(), start);
             epicsTime2string(iv.getEnd(), end);    
             printf("Total: %s - %s\n",
                    start.c_str(), end.c_str());
            aus = index.getKeyAUIterator(name);
            if (aus)
            {
                bool have_au = aus->getFirst(iv, &key, &key_iv);
                while (have_au)
                {
                    epicsTime2string(key_iv.getStart(), start);
                    epicsTime2string(key_iv.getEnd(), end);    
                    printf("'%s' @ 0x%lX: %s - %s\n",
                           key.getPath(), key.getOffset(),
                           start.c_str(), end.c_str());
                    have_au = aus->getNext(&key, &key_iv);
                }
                delete aus;
            }
        }
        ok = names->getNext(name);
    }
    delete names;
}

int main(int argc, const char *argv[])
{
    initEpicsTimeHelper();

    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Data Tool version " VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    CmdArgFlag help (parser, "help", "Show help");
    CmdArgFlag verbose_flag (parser, "verbose", "Show more info");
    CmdArgString dir2index (parser, "dir2index", "<dir. file>",
                             "Convert old directory file to RTree index");
    CmdArgString dumpindex (parser, "dumpindex", "<index file>",
                             "Dump contents of RTree index");
    CmdArgString output (parser, "output", "<file name>", "Output file");
    if (! parser.parse())
        return -1;
    if (help   ||   parser.getArguments().size() > 0)
    {
        parser.usage();
        return -1;
    }
    verbose = verbose_flag;

    if (dir2index.get().length() > 0)
    {
        if (output.get().length() == 0)
        {
            fprintf(stderr, "Option dir2index requires output option\n");
            return 1;
        }
        convert_dir_index(dir2index.get(), output.get());
        return 0;
    }
    else if (dumpindex.get().length() > 0)
    {
        dump_index(dumpindex.get());
    }
    else
    {
        parser.usage();
        return -1;
    }
    
    return 0;
}
