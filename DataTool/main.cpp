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
#include "IndexFile.h"

bool verbose;

void list_names(const stdString &index_name)
{
    IndexFile index;
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return;
    }
    IndexFile::NameIterator names;
    if (!index.getFirstChannel(names))
    {
        fprintf(stderr, "Cannot get name iterator for index '%s'\n",
                index_name.c_str());
        return;
    }
    epicsTime stime, etime;
    stdString start, end;
    RTree *tree;
    do
    {
        if (!(tree = index.getTree(names.getName())))
        {
            fprintf(stderr, "Cannot get tree for channel '%s'\n",
                    names.getName().c_str());
            continue;
        }
        tree->getInterval(stime, etime);
        printf("Channel '%s': %s - %s\n",
               names.getName().c_str(),
               epicsTimeTxt(stime, start),
               epicsTimeTxt(etime, end));
        delete tree;
    }
    while (index.getNextChannel(names));
    index.close();
}

void convert_dir_index(const stdString &dir_name, const stdString &index_name)
{
    DirectoryFile dir;
    if (!dir.open(dir_name))
        return;
	
    DirectoryFileIterator channels = dir.findFirst();
    IndexFile index;
    if (verbose)
        printf("Opened directory file '%s'\n", dir_name.c_str());
    if (!index.open(index_name, false))
    {
        fprintf(stderr, "Cannot create index '%s'\n",
                index_name.c_str());
        return;
    }
    if (verbose)
        printf("Created index '%s'\n", index_name.c_str());
    for (/**/;  channels.isValid();  channels.next())
    {
        if (verbose)
            printf("Channel '%s':\n", channels.entry.data.name);
        if (! Filename::isValid(channels.entry.data.first_file))
        {
            if (verbose)
                printf("No values\n");
            continue;
        }
        RTree *tree = index.addChannel(channels.entry.data.name);
        if (!tree)
        {
            fprintf(stderr, "Cannot add channel '%s' to index '%s'\n",
                    channels.entry.data.name, index_name.c_str());
            continue;
        }   
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
            if (!tree->insertDatablock(
                    header->data.begin_time, header->data.end_time,
                    header->offset,  header->datafile->getBasename()))
            {
                fprintf(stderr, "insertDatablock failed for channel '%s'\n",
                        channels.entry.data.name);
                break;
            }
            header->read_next();
        }
        delete header;
        delete tree;
    }
    DataFile::close_all();
    index.close();
}

static DataHeader *get_dataheader(const stdString &file, FileOffset offset)
{
    DataFile *datafile =
        DataFile::reference("", file, false);
    if (!datafile)
        return 0;
    DataHeader *header = datafile->getHeader(offset);
    datafile->release(); // ref'ed by header
    return header; // might be NULL
}                

void convert_index_dir(const stdString &index_name, const stdString &dir_name)
{
#ifdef TODO
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

    DirectoryFile dir;
    if (!dir.open(dir_name, true))
        return;
    
    stdString name;
    key_AU_Iterator *aus;
    bool ok = names->getFirst(&name);
    key_Object key;
    interval iv, key_iv;
    epicsTime start_time, end_time;
    stdString start_file, end_file;
    FileOffset start_offset, end_offset;
    while (ok)
    {
        if (verbose)
            printf("Channel '%s':\n", name.c_str());
        if (!index.getEntireIndexedInterval(name.c_str(), &iv))
        {
            fprintf(stderr, "Cannot get interval for channel '%s'\n", name.c_str());
        }       
        else
        {
            aus = index.getKeyAUIterator(name.c_str());
            // Get first and last data buffer from RTree
            if (aus)
            {
                bool have_au = aus->getFirst(iv, &key, &key_iv);
                if (have_au)
                {
                    start_file = key.getPath();
                    start_offset = key.getOffset();
                    while (have_au)
                    {
                        end_file = key.getPath();
                        end_offset = key.getOffset();
                        have_au = aus->getNext(&key, &key_iv);
                    }
                    if (verbose)
                        printf("'%s' @ 0x%lX - '%s' @ 0x%lX\n",
                               start_file.c_str(), start_offset,
                               end_file.c_str(), end_offset);
                }
                delete aus;
                // Check by reading first+last buffer & getting times
                bool times_ok = false;
                DataHeader *header;
                header = get_dataheader(start_file, start_offset);
                if (header)
                {
                    start_time = header->data.begin_time;
                    delete header;
                    header = get_dataheader(end_file, end_offset);
                    if (header)
                    {
                        end_time = header->data.end_time;
                        delete header;
                        times_ok = true;
                    }
                }
                if (times_ok)
                {
                    stdString start, end;
                    epicsTime2string(start_time, start);
                    epicsTime2string(end_time, end);
                    if (verbose)
                        printf("%s - %s\n",
                               start.c_str(), end.c_str());
                    DirectoryFileIterator dfi;
                    dfi = dir.find(name);
                    if (!dfi.isValid())
                        dfi = dir.add(name);
                    dfi.entry.setFirst(start_file, start_offset);
                    dfi.entry.setLast(end_file, end_offset);
                    dfi.save();
                }
            }
            else
            {
                fprintf(stderr, "Cannot get AU iterator for channel '%s'\n",
                        name.c_str());
            }   
        }
        ok = names->getNext(&name);
    }
    delete names;
#endif
}

void dump_index(const stdString &index_name, const stdString channel_name)
{
#ifdef TODO
    archiver_Index index;
    if (!index.open(index_name.c_str()))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return;
    }
    channel_Name_Iterator *names;
    if (channel_name.empty())
    {
        names = index.getChannelNameIterator();
        if (!names)
        {
            fprintf(stderr, "Cannot get name iterator for index '%s'\n",
                    index_name.c_str());
            return;
        }
    }
    else
        names = 0;
    stdString name;
    key_AU_Iterator *aus;
    bool ok = true;
    if (channel_name.empty())
        ok = names->getFirst(&name);
    else
        name = channel_name;
    key_Object key;
    interval iv, key_iv;
    stdString start, end;
    while (ok)
    {
        printf("Channel '%s':\n", name.c_str());
        if (!index.getEntireIndexedInterval(name.c_str(), &iv))
        {
            fprintf(stderr, "Cannot get interval for channel '%s'\n",
                    name.c_str());
        }       
        else
        {
             epicsTime2string(iv.getStart(), start);
             epicsTime2string(iv.getEnd(), end);    
             printf("Total: %s - %s\n",
                    start.c_str(), end.c_str());
            aus = index.getKeyAUIterator(name.c_str());
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
        if (channel_name.empty())
            ok = names->getNext(&name);
        else
            ok = false;
    }
    if (names)
        delete names;
#endif
}

int main(int argc, const char *argv[])
{
    initEpicsTimeHelper();

    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Data Tool version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    CmdArgFlag help (parser, "help", "Show help");
    CmdArgFlag verbose_flag (parser, "verbose", "Show more info");
    CmdArgString list_index(parser, "list", "<index>", "List name info");
    CmdArgString dir2index (parser, "dir2index", "<dir. file>",
                             "Convert old directory file to RTree index");
    CmdArgString index2dir (parser, "index2dir", "<index>",
                             "Convert RTree index to old directory file");
    CmdArgString dumpindex (parser, "dumpindex", "<index>",
                             "Dump contents of RTree index");
    CmdArgString output (parser, "output", "<file name>", "Output file");
    CmdArgString channel_name (parser, "channel", "<name>", "Channel name");
    if (! parser.parse())
        return -1;
    if (help   ||   parser.getArguments().size() > 0)
    {
        parser.usage();
        return -1;
    }
    verbose = verbose_flag;

    if (list_index.get().length() > 0)
        list_names(list_index);
    else if (dir2index.get().length() > 0)
    {
        if (output.get().length() == 0)
        {
            fprintf(stderr, "Option dir2index requires output option\n");
            return 1;
        }
        convert_dir_index(dir2index.get(), output.get());
        return 0;
    }
    else if (index2dir.get().length() > 0)
    {
        if (output.get().length() == 0)
        {
            fprintf(stderr, "Option index2dir requires output option\n");
            return 1;
        }
        convert_index_dir(index2dir.get(), output.get());
        return 0;
    }
    else if (dumpindex.get().length() > 0)
    {
        dump_index(dumpindex, channel_name);
    }
    else
    {
        parser.usage();
        return -1;
    }
    
    return 0;
}
