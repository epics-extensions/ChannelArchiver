// System
#include <stdio.h>
// Base
#include <epicsVersion.h>
// Tools
#include "AutoPtr.h"
#include "ArchiverConfig.h"
#include "ArgParser.h"
#include "epicsTimeHelper.h"
// Storage
#include "DirectoryFile.h"
#include "DataFile.h"
#include "IndexFile.h"

int verbose;

// TODO: copy program that combines sub-archives

void show_hash_info(const stdString &index_name)
{
    IndexFile index(3);
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return;
    }
    index.showStats(stdout);
    index.close();
}

void list_names(const stdString &index_name)
{
    IndexFile index(3);
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return;
    }
    IndexFile::NameIterator names;
    if (!index.getFirstChannel(names))
        return;
    epicsTime stime, etime;
    stdString start, end;
    do
    {
        AutoPtr<RTree> tree(index.getTree(names.getName()));
        if (!tree)
        {
            fprintf(stderr, "Cannot get tree for channel '%s'\n",
                    names.getName().c_str());
            continue;
        }
        tree->getInterval(stime, etime);
        printf("Channel '%s' (M=%d): %s - %s\n",
               names.getName().c_str(),
               tree->getM(),
               epicsTimeTxt(stime, start),
               epicsTimeTxt(etime, end));
    }
    while (index.getNextChannel(names));
    index.close();
}

void convert_dir_index(int RTreeM,
                       const stdString &dir_name, const stdString &index_name)
{
    DirectoryFile dir;
    if (!dir.open(dir_name))
        return;
	
    DirectoryFileIterator channels = dir.findFirst();
    IndexFile index(RTreeM);
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
        AutoPtr<RTree> tree(index.addChannel(channels.entry.data.name));
        if (!tree)
        {
            fprintf(stderr, "Cannot add channel '%s' to index '%s'\n",
                    channels.entry.data.name, index_name.c_str());
            continue;
        }   
        DataFile *datafile =
            DataFile::reference(dir.getDirname(),
                                channels.entry.data.first_file, false);
        AutoPtr<DataHeader> header(
            datafile->getHeader(channels.entry.data.first_offset));
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
    }
    DataFile::close_all();
    index.close();
}

#ifdef TODO
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
#endif

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

unsigned long dump_datablocks_for_channel(IndexFile &index,
                                          const stdString &channel_name,
                                          unsigned long &direct_count,
                                          unsigned long &chained_count)
{
    direct_count = chained_count = 0;
    AutoPtr<RTree> tree(index.getTree(channel_name));
    if (! tree)
        return 0;
    RTree::Datablock block;
    RTree::Node node(tree->getM(), true);
    stdString start, end;
    int idx;
    bool ok;
    if (verbose > 2)
        printf("Datablocks for channel '%s': ", channel_name.c_str());
    for (ok = tree->getFirstDatablock(node, idx, block);
         ok;
         ok = tree->getNextDatablock(node, idx, block))
    {
        ++direct_count;
        if (verbose > 2)
            printf("'%s' @ 0x%lX: %s - %s\n",
                   block.data_filename.c_str(), block.data_offset,
                   epicsTimeTxt(node.record[idx].start, start),
                   epicsTimeTxt(node.record[idx].end, end));
        while (tree->getNextChainedBlock(block))
        {
            ++chained_count;
            if (verbose > 2)
                printf("---  '%s' @ 0x%lX\n",
                       block.data_filename.c_str(), block.data_offset);
        }
    }
    return direct_count + chained_count;
}

unsigned long dump_datablocks(const stdString &index_name,
                              const stdString &channel_name)
{
    unsigned long direct_count, chained_count;
    IndexFile index(3);
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return 0;
    }
    dump_datablocks_for_channel(index, channel_name,
                                direct_count, chained_count);
    index.close();
    printf("%ld data blocks, %ld hidden blocks, %ld total\n",
           direct_count, chained_count,
           direct_count + chained_count);
    return direct_count + chained_count;
}

unsigned long dump_all_datablocks(const stdString &index_name)
{
    unsigned long total = 0, direct = 0, chained = 0, channels = 0;
    IndexFile index(3);
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return 0;
    }
    IndexFile::NameIterator names;
    if (!index.getFirstChannel(names))
        return 0;
    do
    {
        ++channels;
        unsigned long direct_count, chained_count;
        total += dump_datablocks_for_channel(index, names.getName(),
                                             direct_count, chained_count);
        direct += direct_count;
        chained += chained_count;
    }
    while (index.getNextChannel(names));
    index.close();
    printf("Total: %ld channels, %ld datablocks\n", channels, total);
    printf("       %ld visible datablocks, %ld hidden\n", direct, chained);
    return total;
}

void dot_index(const stdString &index_name, const stdString channel_name,
               const stdString &dot_name)
{
    IndexFile index(3);
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return;
    }
    AutoPtr<RTree> tree(index.getTree(channel_name));
    if (!tree)
    {
        fprintf(stderr, "Cannot find '%s' in index '%s'\n",
                channel_name.c_str(), index_name.c_str());
        return;
    }
    tree->makeDot(dot_name.c_str());
    index.close();
}

bool seek_time(const stdString &index_name,
               const stdString &channel_name,
               const stdString &start_txt)
{
    epicsTime start;
    if (!string2epicsTime(start_txt, start))
    {
        fprintf(stderr, "Cannot convert '%s' to time stamp\n",
                start_txt.c_str());
        return false;
    }
    IndexFile index(3);
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return false;
    }
    AutoPtr<RTree> tree(index.getTree(channel_name));
    if (tree)
    {
        RTree::Datablock block;
        RTree::Node node(tree->getM(), true);
        int idx;
        if (tree->searchDatablock(start, node, idx, block))
        {
            stdString s, e;
            printf("Found block %s - %s\n",
                   epicsTimeTxt(node.record[idx].start, s),
                   epicsTimeTxt(node.record[idx].end, e));
        }
        else
        {
            printf("Nothing found\n");
        }
    }
    else
    {
        fprintf(stderr, "Cannot find channel '%s'\n",
                channel_name.c_str());
    }
    index.close();
    return true;
}

bool check (const stdString &index_name)
{
    IndexFile index(3);
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index '%s'\n",
                index_name.c_str());
        return false;
    }
    bool ok = index.check(verbose);
    index.close();
    return ok;
}

int main(int argc, const char *argv[])
{
    initEpicsTimeHelper();

    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Data Tool version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    parser.setArgumentsInfo("<index-file>");
    CmdArgFlag help(parser, "help", "Show help");
    CmdArgInt verbosity(parser, "verbose", "<level>", "Show more info");
    CmdArgFlag list_index(parser, "list", "List channel name info");
    CmdArgString dir2index(parser, "dir2index", "<dir. file>",
                           "Convert old directory file to index");
    CmdArgString index2dir(parser, "index2dir", "<dir. file>",
                           "Convert index to old directory file");
    CmdArgInt RTreeM(parser, "M", "<1-100>", "RTree M value");
    CmdArgFlag dump_blocks(parser, "blocks", "List channel's data blocks");
    CmdArgFlag all_blocks(parser, "Blocks", "List all data blocks");
    CmdArgString dotindex(parser, "dotindex", "<dot filename>",
                          "Dump contents of RTree index into dot file");
    CmdArgString channel_name (parser, "channel", "<name>", "Channel name");
    CmdArgFlag hashinfo(parser, "hashinfo", "Show Hash table info");
    CmdArgString seek_test(parser, "seek", "<time>", "Perform seek test");
    CmdArgFlag test(parser, "test", "Perform some consistency tests");
    RTreeM.set(50);
    if (! parser.parse())
        return -1;
    if (help   ||   parser.getArguments().size() != 1)
    {
        parser.usage();
        return -1;
    }
    if ((dump_blocks ||
         dotindex.get().length() > 0  ||
         seek_test.get().length() > 0)
        && channel_name.get().length() <= 0)
    {
        fprintf(stderr,
                "Options 'blocks' and 'dotindex' require 'channel'.\n");
        return -1;
    }
    verbose = verbosity;
    stdString index_name = parser.getArgument(0);

    if (list_index)
        list_names(index_name);
    else if (hashinfo)
        show_hash_info(index_name);
    else if (dir2index.get().length() > 0)
    {
        convert_dir_index(RTreeM, dir2index, index_name);
        return 0;
    }
    else if (index2dir.get().length() > 0)
    {
        convert_index_dir(index_name, index2dir);
        return 0;
    }
    else if (all_blocks)
    {
        dump_all_datablocks(index_name);
        return 0;
    }
    else if (dump_blocks)
    {
        dump_datablocks(index_name, channel_name);
        return 0;
    }
    else if (dotindex.get().length() > 0)
    {
        dot_index(index_name, channel_name, dotindex);
        return 0;
    }
    else if (seek_test.get().length() > 0)
    {
        seek_time(index_name, channel_name, seek_test);
        return 0;
    }
    else if (test)
    {
        return check(index_name) ? 0 : -1;
    }
    else
    {
        parser.usage();
        return -1;
    }
    
    return 0;
}
