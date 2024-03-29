// ArchiveDataTool ----------

// System
#include <stdio.h>
// Base
#include <epicsVersion.h>
// Tools
#include <AutoPtr.h>
#include <ArgParser.h>
#include <BenchTimer.h>
#include <epicsTimeHelper.h>
// Storage
#include <OldDirectoryFile.h>
#include <IndexFile.h>
#include <DataFile.h>
#include <RawDataReader.h>
#include <DataWriter.h>

int verbose;

bool do_enforce_off = false;

// TODO: export/import: ASCII or XML
// TODO: delete/rename channel

void show_hash_info(const stdString &index_name)
{
    IndexFile index(3);
    index.open(index_name);
    index.showStats(stdout);
}

void count_channel_values(Index::Result *index_info,
                          DbrType &type, DbrCount &count, size_t &blocks, size_t &values)
{
    type   = 0;
    count  = 0;
    blocks = 0;
    values = 0;
    if (! index_info)
        return;
    DataFile *datafile;
    AutoPtr<DataHeader> header;

    stdString start, end;

    AutoPtr<RTree::Datablock> block(index_info->getRTree()->getFirstDatablock());
    for (/**/;  block && block->isValid();  block->getNextDatablock())
    {
        ++blocks;
        datafile = DataFile::reference(index_info->getDirectory(),
                                       block->getDataFilename(),
                                       false);
        header = datafile->getHeader(block->getDataOffset());
        datafile->release();
        if (header)
        {
            if (blocks == 1)
            {
                type  = header->data.dbr_type;
                count = header->data.dbr_count;
            }
            values += header->data.num_samples;
            header = 0;
        }
        else
            printf("Cannot read header in data file.\n");
    }
}

void list_names(const stdString &index_name, bool channel_detail)
{
    IndexFile index(3);
    AutoPtr<Index::NameIterator> names(index.iterator());
    epicsTime t0, t1;
    stdString start, end;
    size_t channels = 0, blocks = 0, values = 0;
    index.open(index_name);
    if (channel_detail)
        printf("# Name\tType\tCount\tStart\tEnd\tBlocks\tValues\n");
    for (/**/;  names && names->isValid();  names->next())
    {
        AutoPtr<Index::Result> info(index.findChannel(names->getName()));
        Interval range(info->getRTree()->getInterval());
        if (channels == 0)
        {
            t0 = range.getStart();
            t1 = range.getEnd();
        }
        else
        {
            if (t0 > range.getStart())
                t0 = range.getStart();
            if (t1 < range.getEnd())
                t1 = range.getEnd();
        }
    	size_t chan_blocks, chan_values;
        DbrType  type;
        DbrCount count;
        count_channel_values(info, type, count, chan_blocks, chan_values);
        ++channels;
        blocks += chan_blocks;
        values += chan_values;
    	if (channel_detail)
            printf("%s\t%d\t%d\t%s\t%s\t%zu\t%zu\n",
                   names->getName().c_str(),
                   (int) type, (int) count,
                   epicsTimeTxt(range.getStart(), start),
                   epicsTimeTxt(range.getEnd(), end),
                   chan_blocks, chan_values);
    }
    printf("%zu channels, %zu values, %s - %s\n",
           channels, values,
           epicsTimeTxt(t0, start), epicsTimeTxt(t1, end));
}

DataHeader *get_dataheader(const stdString &dir, const stdString &file,
                           FileOffset offset)
{
    DataFile *datafile = DataFile::reference(dir, file, false);
    if (!datafile)
        return 0;
    DataHeader *header = datafile->getHeader(offset);
    datafile->release(); // ref'ed by header
    return header; // might be NULL
}

// Try to determine the period and a good guess
// for the number of samples for a channel.
void determine_period_and_samples(IndexFile &index,
                                  const stdString &channel,
                                  double &period, size_t &num_samples)
{
    period      = 1.0; // initial guess
    num_samples = 10;
    // Whatever fails only means that there is no data,
    // so the initial guess remains OK.
    // Otherwise we peek into the last datablock.
    AutoPtr<Index::Result> info(index.findChannel(channel));
    if (!info)
        return;
    AutoPtr<RTree::Datablock> block(info->getRTree()->getLastDatablock());
    if (!block)
        return;
    AutoPtr<DataHeader> header(get_dataheader(info->getDirectory(),
                                              block->getDataFilename(),
                                              block->getDataOffset()));
    if (!header)
        return;
    period = header->data.period;
    num_samples = header->data.num_samples;
    if (verbose > 2)
        printf("Last source buffer: Period %g, %lu samples.\n",
               period, (unsigned long) num_samples);
}

// Helper for copy: Handles samples of a single channel
void copy_channel(const stdString &channel_name,
                  const epicsTime *start, const epicsTime *end,
                  IndexFile &index, RawDataReader &reader,
                  IndexFile &new_index,
                  size_t &channel_count, size_t &value_count, size_t &back_count)
{
    double          period = 1.0;
    size_t          num_samples = 4096;    
    AutoPtr<DataWriter> writer;
    RawValue::Data *last_value = 0;
    bool            last_value_set = false;
    size_t          count = 0, back = 0;
    if (verbose > 1)
    {
        printf("Channel '%s': ", channel_name.c_str());
        if (verbose > 3)
            printf("\n");
        fflush(stdout);
    }
    try
    {
        determine_period_and_samples(index, channel_name, period, num_samples);
        const RawValue::Data *value = reader.find(channel_name, start);
        while (value && start && RawValue::getTime(value) < *start)
        {   // Correct for "before-or-at" idea of find()
            if (verbose > 2)
                printf("Skipping sample before start time\n");
            value = reader.next();
        }
        for (/**/; value; value = reader.next())
        {
            if (end  &&  RawValue::getTime(value) >= *end)
                break; // Reached or exceeded end time
            if (!writer || reader.changedType() || reader.changedInfo())
            {   // Need new or different writer
                writer = new DataWriter(
                    new_index, channel_name,
                    reader.getInfo(), reader.getType(), reader.getCount(),
                    period, num_samples);
                RawValue::free(last_value);
                last_value_set = false;
                last_value = RawValue::allocate(reader.getType(),
                                                reader.getCount(), 1);
                if (!writer  ||  !last_value)
                {
                    printf("Cannot allocate DataWriter/RawValue\n");
                    return;
                }
            }
            if (RawValue::getTime(value) < writer->getLastStamp())
            {
                ++back;
                if (verbose > 2)
                    printf("Skipping %lu back-in-time values\r",
                           (unsigned long) back);
                continue;
            }
            if (! writer->add(value))
            {
                printf("DataWriter::add claims back-in-time\n");
                continue;
            }
            // Keep track of last time stamp and status (not value!)
            RawValue::setStatus(last_value,
                                RawValue::getStat(value),
                                RawValue::getSevr(value));
            RawValue::setTime(last_value, RawValue::getTime(value));
            last_value_set = true;
            ++count;
            if (verbose > 3 && (count % 1000 == 0))
                printf("Copied %lu values\r", (unsigned long) count);
            
        }
    }
    catch (GenericException &e)
    {   // Catch errrors in the sample copy loop,
        // meaning to catch read errors on this channel, which are reported
        // but don't stop the copy process for the other channels.
        // Of course we'd also catch write errors in here,
        // but don't really distinguish them...
        fprintf(stderr, "Error on channel '%s':\n%s\n",
                channel_name.c_str(), e.what());
    }
    // Always try to add the 'off' markers.
    // Write errors in here are propagated up,
    // because if we cannot write, we're hosed.
    if (do_enforce_off && last_value_set && count > 0 &&
        RawValue::getSevr(last_value) != ARCH_STOPPED)
    {   // Try to add an "Off" Sample.
        RawValue::setStatus(last_value, 0, ARCH_STOPPED);
        writer->add(last_value);        
    }
    writer = 0;
    if (verbose > 1)
    {
        if (back)
            printf("%lu values, %lu back-in-time                       \n",
                   (unsigned long) count, (unsigned long) back);
        else
            printf("%lu values                                         \n",
                   (unsigned long) count);
    }
    ++channel_count;
    value_count += count;
    back_count += back;
}

// Copy samples from archive with index_name
// to new index copy_name.
// Uses all samples in source archive or  [start ... end[ .
void copy(const stdString &index_name, const stdString &copy_name,
          int RTreeM, const epicsTime *start, const epicsTime *end,
          const stdString &single_name)
{
    IndexFile               index(RTreeM), new_index(RTreeM);
    size_t                  channel_count = 0, value_count = 0, back_count = 0;
    BenchTimer              timer;
    stdString               dir1, dir2;
    Filename::getDirname(index_name, dir1);
    Filename::getDirname(copy_name, dir2);
    if (dir1 == dir2)
    {
        printf("You have to assert that the new index (%s)\n"
               "is in a  directory different from the old index\n"
               "(%s)\n", copy_name.c_str(), index_name.c_str());
        return;
    }
    index.open(index_name);
    new_index.open(copy_name, Index::ReadAndWrite);
    if (verbose)
        printf("Copying values from '%s' to '%s'\n",
               index_name.c_str(), copy_name.c_str());
    RawDataReader reader(index);
    if (single_name.empty())
    {
        AutoPtr<Index::NameIterator> names(index.iterator());
        while (names && names->isValid())
        {
            copy_channel(names->getName(), start, end, index, reader,
                         new_index, channel_count, value_count, back_count);
            names->next();
        }    
    }
    else
        copy_channel(single_name, start, end, index, reader,
                     new_index, channel_count, value_count, back_count);
    new_index.close();
    index.close();
    timer.stop();
    if (verbose)
    {
        printf("Total: %lu channels, %lu values\n",
               (unsigned long) channel_count, (unsigned long) value_count);
        printf("Skipped %lu back-in-time values\n",
               (unsigned long) back_count);
        printf("Runtime: %s\n", timer.toString().c_str());
    }
}

void convert_dir_index(int RTreeM,
                       const stdString &dir_name, const stdString &index_name)
{
    OldDirectoryFile dir;
    if (!dir.open(dir_name))
        return;
    
    OldDirectoryFileIterator channels = dir.findFirst();
    IndexFile index(RTreeM);
    if (verbose)
        printf("Opened directory file '%s'\n", dir_name.c_str());
    index.open(index_name, Index::ReadAndWrite);
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
        AutoPtr<Index::Result> info(index.addChannel(channels.entry.data.name));
        if (!info)
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
                       (unsigned long)header->offset,
                       start.c_str(),
                       end.c_str());
            }
            if (!info->getRTree()->insertDatablock(
                    Interval(header->data.begin_time, header->data.end_time),
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
}


void convert_index_dir(const stdString &index_name, const stdString &dir_name)
{
    IndexFile index(50);
    epicsTime start_time, end_time;
    index.open(index_name.c_str());
    OldDirectoryFile dir;
    if (!dir.open(dir_name, true))
    {
        fprintf(stderr, "Cannot create  '%s'\n", dir_name.c_str());
        return;
    }
    
    AutoPtr<Index::NameIterator> names(index.iterator());
    for (/**/;  names && names->isValid();  names->next())
    {
        if (verbose)
            printf("Channel '%s':\n", names->getName().c_str());
        AutoPtr<const Index::Result> info(index.findChannel(names->getName()));
        LOG_ASSERT(info);
        AutoPtr<RTree::Datablock> block(info->getRTree()->getFirstDatablock());
        if (!block)
        {
            printf("No first datablock for channel '%s'\n",
                   names->getName().c_str());
            continue;
        }
        stdString start_file = block->getDataFilename();
        FileOffset start_offset = block->getDataOffset();

        block = info->getRTree()->getLastDatablock();
        if (!block)
        {
            printf("No last datablock for channel '%s'\n",
                   names->getName().c_str());
            continue;
        }
        stdString end_file = block->getDataFilename();
        FileOffset end_offset = block->getDataOffset();

        // Check by reading first+last buffer & getting times
        bool times_ok = false;
        AutoPtr<DataHeader> header(
            get_dataheader(info->getDirectory(), start_file, start_offset));
        if (header)
        {
            start_time = header->data.begin_time;
            header = get_dataheader(info->getDirectory(), end_file, end_offset);
            if (header)
            {
                end_time = header->data.end_time;
                header = 0;
                times_ok = true;
            }
        }
        if (times_ok)
        {
            stdString start, end;
            if (verbose)
                printf("%s - %s\n",
                       epicsTimeTxt(start_time, start),
                       epicsTimeTxt(end_time, end));
            OldDirectoryFileIterator dfi;
            dfi = dir.find(names->getName());
            if (!dfi.isValid())
                dfi = dir.add(names->getName());
            dfi.entry.setFirst(start_file, start_offset);
            dfi.entry.setLast(end_file, end_offset);
            dfi.entry.data.create_time = start_time;
            dfi.entry.data.first_save_time = start_time;
            dfi.entry.data.last_save_time = end_time;
            dfi.save();
        }
    }
}

unsigned long dump_datablocks_for_channel(IndexFile &index,
                                          const stdString &channel_name,
                                          unsigned long &direct_count,
                                          unsigned long &chained_count)
{
    DataFile *datafile;
    AutoPtr<DataHeader> header;
    direct_count = chained_count = 0;
    AutoPtr<Index::Result> info(index.findChannel(channel_name));
    if (! info)
        return 0;
    stdString start, end;
    if (verbose > 1)
        printf("RTree M for channel '%s': %d\n",
               channel_name.c_str(), info->getRTree()->getM());
    if (verbose > 2)
        printf("Datablocks for channel '%s':\n", channel_name.c_str());
    AutoPtr<RTree::Datablock> block(info->getRTree()->getFirstDatablock());
    for (/**/;  block && block->isValid();  block->getNextDatablock())
    {
        ++direct_count;
        if (verbose > 2)
            printf("'%s' @ 0x%lX: Indexed range %s - %s\n",
                   block->getDataFilename().c_str(),
                   (unsigned long)block->getDataOffset(),
                   epicsTimeTxt(block->getInterval().getStart(), start),
                   epicsTimeTxt(block->getInterval().getEnd(), end));
        if (verbose > 3)
        {
            datafile = DataFile::reference(info->getDirectory(),
                                           block->getDataFilename(),
                                           false);
            header = datafile->getHeader(block->getDataOffset());
            datafile->release();
            if (header)
            {
                header->show(stdout, true);
                header = 0;
            }
            else
                printf("Cannot read header in data file.\n");
        }
        bool first_hidden_block = true;
        while (block->getNextChainedBlock())
        {
            if (first_hidden_block && verbose > 2)
            {
                first_hidden_block = false;
                printf("Hidden blocks:\n");
            }
            ++chained_count;
            if (verbose > 2)
            {
                printf("---  '%s' @ 0x%lX\n",
                       block->getDataFilename().c_str(),
                       (unsigned long)block->getDataOffset());
                if (verbose > 3)
                {
                    datafile = DataFile::reference(info->getDirectory(),
                                                   block->getDataFilename(),
                                                   false);
                    header = datafile->getHeader(block->getDataOffset());
                    if (header)
                    {
                        header->show(stdout, false);
                        header = 0;
                    }
                    else
                        printf("Cannot read header in data file.\n");
                    datafile->release();
                }   
            }
        }
        printf("\n");
    }
    return direct_count + chained_count;
}

unsigned long dump_datablocks(const stdString &index_name,
                              const stdString &channel_name)
{
    unsigned long direct_count, chained_count;
    IndexFile index(3);
    index.open(index_name);
    dump_datablocks_for_channel(index, channel_name,
                                direct_count, chained_count);
    printf("%ld data blocks, %ld hidden blocks, %ld total\n",
           direct_count, chained_count,
           direct_count + chained_count);
    return direct_count + chained_count;
}

unsigned long dump_all_datablocks(const stdString &index_name)
{
    unsigned long total = 0, direct = 0, chained = 0, channels = 0;
    IndexFile index(3);
    index.open(index_name);
    AutoPtr<Index::NameIterator> names(index.iterator());
    for (/**/;  names && names->isValid();  names->next())
    {
        ++channels;
        unsigned long direct_count, chained_count;
        total += dump_datablocks_for_channel(index, names->getName(),
                                             direct_count, chained_count);
        direct += direct_count;
        chained += chained_count;
    }
    printf("Total: %ld channels, %ld datablocks\n", channels, total);
    printf("       %ld visible datablocks, %ld hidden\n", direct, chained);
    return total;
}

void dot_index(const stdString &index_name, const stdString channel_name,
               const stdString &dot_name)
{
    IndexFile index(3);
    index.open(index_name);
    AutoPtr<Index::Result> result(index.findChannel(channel_name));
    if (!result)
    {
        fprintf(stderr, "Cannot find '%s' in index '%s'.\n",
                channel_name.c_str(), index_name.c_str());
        return;
    }
    result->getRTree()->makeDot(dot_name.c_str());
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
    index.open(index_name);
    AutoPtr<Index::Result> result(index.findChannel(channel_name));
    if (result)
    {
        AutoPtr<RTree::Datablock> block(result->getRTree()->search(start));
        if (block)
        {
            stdString s, e;
            printf("Found block %s - %s\n",
                   epicsTimeTxt(block->getInterval().getStart(), s),
                   epicsTimeTxt(block->getInterval().getEnd(), e));
        }
        else
            printf("Nothing found\n");
    }
    else
        fprintf(stderr, "Cannot find channel '%s'\n", channel_name.c_str());
    return true;
}

bool check (const stdString &index_name)
{
    IndexFile index(3);
    index.open(index_name);
    return index.check(verbose);
}

int main(int argc, const char *argv[])
{
    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Data Tool version " ARCH_VERSION_TXT ", "
                     EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n"
                     );
    parser.setArgumentsInfo("<index-file>");
    CmdArgFlag help          (parser, "help", "Show help");
    CmdArgInt  verbosity     (parser, "verbose", "<level>", "Show more info");
    CmdArgFlag info          (parser, "info", "Simple archive info");
    CmdArgFlag list_index    (parser, "list", "List channel name info");
    CmdArgString copy_index  (parser, "copy", "<new index>", "Copy channels");
    CmdArgString start_time  (parser, "start", "<time>",
                              "Format: \"mm/dd/yyyy[ hh:mm:ss[.nano-secs]]\"");
    CmdArgString end_time    (parser, "end", "<time>", "(exclusive)");
    CmdArgDouble file_limit  (parser, "file_limit", "<MB>", "File Size Limit");
    CmdArgString basename    (parser, "basename", "<string>", "Basename for new data files");
    CmdArgFlag enforce_off   (parser, "append_off", "Enforce a final 'Archive_Off' value when copying data");
    CmdArgString dir2index   (parser, "dir2index", "<dir. file>",
                              "Convert old directory file to index");
    CmdArgString index2dir   (parser, "index2dir", "<dir. file>",
                              "Convert index to old directory file");
    CmdArgInt RTreeM         (parser, "M", "<1-100>", "RTree M value");
    CmdArgFlag dump_blocks   (parser, "blocks", "List channel's data blocks");
    CmdArgFlag all_blocks    (parser, "Blocks", "List all data blocks");
    CmdArgString dotindex    (parser, "dotindex", "<dot filename>",
                              "Dump contents of RTree index into dot file");
    CmdArgString channel_name(parser, "channel", "<name>", "Channel name");
    CmdArgFlag hashinfo      (parser, "hashinfo", "Show Hash table info");
    CmdArgString seek_test   (parser, "seek", "<time>", "Perform seek test");
    CmdArgFlag test          (parser, "test", "Perform some consistency tests");

    try
    {
        // Defaults
        RTreeM.set(50);
        file_limit.set(100.0);
        // Get Arguments
        if (! parser.parse())
            return -1;
        if (help   ||   parser.getArguments().size() != 1)
        {
            parser.usage();
            return -1;
        }
        // Consistency checks
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
        DataWriter::file_size_limit = (unsigned long)(file_limit*1024*1024);
        if (file_limit < 10.0)
            fprintf(stderr, "file_limit values under 10.0 MB are not useful\n");
        // Start/end time
        epicsTime *start = 0, *end = 0;
        stdString txt;
        if (start_time.get().length() > 0)
        {
            start = new epicsTime;
            string2epicsTime(start_time.get(), *start);
            if (verbose > 1)
                printf("Using start time %s\n", epicsTimeTxt(*start, txt));
        }
        if (end_time.get().length() > 0)
        {
            end = new epicsTime();
            string2epicsTime(end_time.get(), *end);
            if (verbose > 1)
                printf("Using end time   %s\n", epicsTimeTxt(*end, txt));
        }
        // Base name
        if (basename.get().length() > 0)
    	    DataWriter::data_file_name_base = basename.get();
        if (enforce_off)
    	    do_enforce_off = true;
        // What's requested?
        if (info)
    	    list_names(index_name, false);
        else if (list_index)
            list_names(index_name, true);
        else if (copy_index.get().length() > 0)
        {
            copy(index_name, copy_index, RTreeM, start, end, channel_name);
            return 0;
        }
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
    }
    catch (GenericException &e)
    {
        fprintf(stderr, "Error:\n%s\n", e.what());
    }
        
    return 0;
}

