// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------
//
// ArchiveManager:
//
// Could end up being a command-line tool
// for simple ChannelArchive management:
// * List channels
// * List values
// * Extract subsection into new archive
// * ??
//
// Right now it's more a collection of API tests.
//

// Warning: names too long for debug info
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "../ArchiverConfig.h"
#include "BinArchive.h"
#include "ArgParser.h"
#include "BinValueIterator.h"
#include <signal.h>
#include "ExpandingValueIteratorI.h"
#include "Filename.h"
#include "ascii.h"

#ifdef MANAGER_USE_MULTI
#   include "MultiArchive.h"
#   define ARCHIVE_TYPE MultiArchive
#else
#   define ARCHIVE_TYPE BinArchive
#endif

// For communication sigint_handler -> main loop
static bool run = true;
static void signal_handler (int sig)
{
    run = false;
    std::cout << "exiting on signal " << sig << " as soon as possible\n";
}                                                        

// List all channel names for given pattern
// (leave empty to list all channels)
void list_channels (const stdString &archive_name, const stdString &pattern)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel (archive);

    archive.findChannelByPattern (pattern, channel);
    while (run && channel)
    {
        std::cout << channel->getName() << "\n";
        ++ channel;
    }
}

// List all the values stampes start <= value.getTime() < end
// for a given channel,
// iterated version.
// start == end == 0 will list all values.
void list_values (const stdString &archive_name, const stdString &channel_name,
                const osiTime &start, const osiTime &end)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel(archive);
    ValueIterator   value(archive);

    if (! archive.findChannelByName (channel_name, channel))
    {
        std::cerr << "Cannot find channel '" << channel_name << "':\n";
        return;
    }

    std::cout << "Channel " << channel->getName() << "\n";
    channel->getValueAfterTime (start, value);
    while (run && value && (end == nullTime  ||  value->getTime() < end))
    {
        std::cout << *value << "\n";
        ++ value;
    }
}

void dump (const stdString &archive_name, const stdString &channel_pattern,
           const osiTime &start, const osiTime &end)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel(archive);
    ValueIterator   value(archive);

    archive.findChannelByPattern (channel_pattern, channel);
    while (run && channel)
    {
        std::cout << "Channel " << channel->getName() << "\n";
        channel->getValueAfterTime (start, value);
        while (run && value && (end == nullTime  ||  value->getTime() < end))
        {
            std::cout << *value << "\n";
            ++ value;
        }
        ++channel;
    }
}

void show_info (const stdString &archive_name, const stdString &pattern)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel (archive);
    size_t channel_count = 0;
    osiTime start, end, time;

    for (archive.findChannelByPattern(pattern, channel); channel; ++channel)
    {
        ++channel_count;

        time = channel->getFirstTime ();
        if (isValidTime(time) &&  (time < start  ||  start == nullTime))
            start = time;
        time = channel->getLastTime ();
        if (time > end)
            end = time;
        if (pattern.empty ())
            continue;
        std::cout    << channel->getFirstTime () << "\t"
                << channel->getLastTime () << "\t"
                << channel->getName() << "\n";
    }
    std::cout << "Channel count : " << channel_count << "\n";
    std::cout << "First sample  : " << start << "\n";
    std::cout << "Last  sample  : " << end   << "\n";
}

void show_channel_info (const stdString &archive_name,
                        const stdString &channel_name)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel (archive);

    if (archive.findChannelByName (channel_name, channel))
    {
        std::cout << "start: " << channel->getFirstTime () << "\n";
        std::cout << "end:   " << channel->getLastTime ()  << "\n";
    }
    else
        std::cout << channel_name << ": not found\n";
}

void seek_time (const stdString &archive_name,
                const stdString &channel_name,
                const osiTime &start)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel (archive);
    ValueIterator   value (archive);

    if (! archive.findChannelByName (channel_name, channel))
    {
        std::cerr << "Cannot find channel '" << channel_name << "':\n";
        return;
    }

    std::cout << "Channel '" << channel->getName() << "'\n";

    channel->getValueNearTime (start, value);
    if (value)
        std::cout << "Found: " << value->getTime () << "\n";
    else
        std::cout << "Not found\n";
}

// "Export", copy all values from [start...end[
// for all channels that match pattern
// into new archive.
//
// Start, end may be 0 to copy the whole archive.
//
// Huge repeat counts can be suppressed.
//
// (this makes the code more complicated to read...)
void do_export (const stdString &archive_name, 
                const stdString &channel_pattern,
                const osiTime &start, const osiTime &end,
                const stdString &new_dir_name,
                size_t repeat_limit,
                size_t days_per_file)
{
    size_t i, chunk, chunk_count=0, val_count=0;
    osiTime next_file_time, last_stamp;

    Archive old_archive (new BinArchive (archive_name));
    BinArchive *new_archiveI = new BinArchive (new_dir_name, true);
    if (days_per_file > 0)
        new_archiveI->setSecsPerFile (BinArchive::SECS_PER_DAY*days_per_file);
    else
        new_archiveI->setSecsPerFile (BinArchive::SECS_PER_MONTH);
    Archive new_archive (new_archiveI);

    ChannelIterator old_channel (old_archive);
    ChannelIterator new_channel (new_archive);
    ValueIterator values (old_archive);

    for (old_archive.findChannelByPattern (channel_pattern, old_channel); 
         run && old_channel;
         ++old_channel)
    {
        std::cout << old_channel->getName ();
        std::cout.flush();
        chunk_count = val_count = 0;

        try
        {
            if (! old_channel->getValueAfterTime (start, values))
            {
                std::cout << "\t" << "0 chunks\t0 values\n";
                continue;
            }
            new_archiveI->calcNextFileTime (values->getTime(), next_file_time);
            if (isValidTime(next_file_time)  &&  next_file_time < end)
                chunk = values.determineChunk (next_file_time);
            else
                chunk = values.determineChunk (end);
            if (chunk <= 0)
            {
                std::cout << "\t" << "0 chunks\t0 values\n";
                continue;
            }

            if (new_archive.findChannelByName (old_channel->getName (),
                                               new_channel))
            {
                last_stamp = new_channel->getLastTime ();
                if (isValidTime (start)  &&  last_stamp > start)
                {
                    std::cout << "Error:\n"
                         << "Archive " << new_dir_name
                         << " does already contain\n"
                         << "values for channel " << old_channel->getName ()
                         << "\n"
                         << "and they are stamped " << last_stamp
                         << ", which is after\n"
                         << "the start time of    " << start << "\n";
                    break;
                }
            }
            else
            {
                new_archive.addChannel (old_channel->getName (), new_channel);
                last_stamp = nullTime;
            }

            while (chunk > 0)
            {
                new_channel->lockBuffer (*values, values.getPeriod());
                for (i=0;   values && i<chunk;   ++i, ++values)
                {
                    if (! isValidTime(values->getTime()))
                    {
                        std::cout << *values << "\tinvalid time stamp, skipped\n";
                        continue;
                    }
                    if (isValidTime (last_stamp))   // time stamp checks
                    {   
                        if (values->getTime() < last_stamp)
                        {
                            std::cout << *values
                                 << "\tgoing back in time, skipped\n";
                            continue;
                        }
                    }
                    if (repeat_limit > 0 &&
                        (values->getSevr() == ARCH_REPEAT ||
                         values->getSevr() == ARCH_EST_REPEAT) &&
                        (size_t)values->getStat() >= repeat_limit)
                    {
                        std::cout << *values
                             << "\trepeat count beyond " << repeat_limit
                             << ", skipped\n";
                        continue;
                    }
                    if (! new_channel->addValue (*values))
                    {
                        new_channel->addBuffer (*values, values.getPeriod(),
                                                chunk);
                        new_channel->addValue (*values);
                    }
                    last_stamp = values->getTime();
                }
                ++chunk_count;
                val_count += chunk;

                new_archiveI->calcNextFileTime (values->getTime(),
                                                next_file_time);
                if (isValidTime (next_file_time)  &&  next_file_time < end)
                    chunk = values.determineChunk (next_file_time);
                else
                    chunk = values.determineChunk (end);
            }
            new_channel->releaseBuffer ();
        }
        catch (GenericException &e)
        {
            std::cout <<  "\nError:\n" << e.what () << "\n";
        }

        std::cout << "\t" << chunk_count << " chunks\t" << val_count << " values\n";
    }
}

void compare (const stdString &archive_name, const stdString &target_name)
{
    Archive src (new ARCHIVE_TYPE (archive_name));
    Archive dst (new ARCHIVE_TYPE (target_name));
    ChannelIterator src_chan (src);
    ChannelIterator dst_chan (dst);
    ValueIterator src_val (src);
    ValueIterator dst_val (dst);

    for (src.findFirstChannel (src_chan); run && src_chan; ++src_chan)
    {
        std::cout << src_chan->getName () << " : ";

        if (! dst.findChannelByName (src_chan->getName (), dst_chan))
        {
            std::cout << "not found\n";
            continue;
        }

        src_chan->getFirstValue (src_val);
        dst_chan->getFirstValue (dst_val);
        while (src_val)
        {
            if (*src_val != *dst_val)
            {
                std::cout << "difference:\n";
                std::cout << "< " << *src_val << "\n";
                std::cout << "> " << *dst_val << "\n";
            }
            ++src_val;
            ++dst_val;
        }
        if (! src_val)
            std::cout << " OK\n";
        std::cout.flush ();
    }
}

// Test ExpandingValueIteratorI
void expand (const stdString &archive_name,
             const stdString &channel_name,
             const osiTime &start,
             const osiTime &end)
{
    Archive         archive (new ARCHIVE_TYPE (archive_name));
    ChannelIterator channel(archive);

    if (! archive.findChannelByName (channel_name, channel))
    {
        std::cerr << "Cannot find channel '" << channel_name << "':\n";
        return;
    }

    std::cout << "Channel '" << channel->getName() << "\n";

    ValueIterator   raw_values(archive);
    channel->getValueAfterTime (start, raw_values);
    ExpandingValueIteratorI *expand = new ExpandingValueIteratorI (raw_values);
    ValueIterator value (expand);
    while (value && (end == nullTime  ||  value->getTime() < end))
    {
        std::cout << *value;
        if (expand->isExpanded())
            std::cout << "\t(expanded)";
        std::cout << "\n";
        ++value;
    }

    std::cout << "reverse:\n";
    --value; // move from 'after end' to 'last value'
    while (value && (start == nullTime  || value->getTime() > start))
    {
        std::cout << *value;
        if (expand->isExpanded())
            std::cout << "\t(expanded)";
        std::cout << "\n";
        --value;
    }
}

// Interpolation Test
void interpolate (const stdString &directory, const stdString &channel_name,
                  const osiTime &start, const osiTime &end,
                  double interpol)
{
#if 0
    Archive a (directory);
    ChannelIterator channel = a.findChannelByName (channel_name);
    if (! channel)
    {
        cerr << "Cannot find channel '" << channel_name << "':\n";
        return;
    }

    std::cout << "Channel '" << channel->getName() << "\n";

    ValueIterator vi = channel.getValueAfterTime (start);
    ExpandingValueIterator evi (&vi);
    LinInterpolValueIterator livi (&evi, interpol);
    InfoFilterValueIterator value (&livi);
    while (value && (end == nullTime  ||  value->getTime() < end))
    {
        std::cout << *value << "\n";
        ++value;
    }
#endif
}

// This is low-level.
// The API shown in here should not be used by
// general Archive clients
void headers (const stdString &directory, const stdString &channel_name)
{
    Archive archive (new BinArchive (directory));
    ChannelIterator channel (archive);
    
    if (! archive.findChannelByName (channel_name, channel))
    {
        std::cerr << "Cannot find channel '" << channel_name << "':\n";
        return;
    }

    ValueIterator value (archive);
    if (! channel->getFirstValue (value))
    {
        std::cerr << "No values for '" << channel_name << "':\n";
        return;
    }

    const CtrlInfoI *info;
    BinValueIterator *bvi = dynamic_cast<BinValueIterator *>(value.getI());
    do
    {
        std::cout << "Buffer:  " << bvi->getHeader().getFilename()
            << " @ " << bvi->getHeader().getOffset () << "\n";
        std::cout << "Prev:    " << bvi->getHeader()->getPrevFile ()
            << " @ " << bvi->getHeader()->getPrev () << "\n";
        std::cout << "Time:    " << bvi->getHeader()->getBeginTime() << "\n";
        std::cout << "...      " << bvi->getHeader()->getEndTime () << "\n";
        std::cout << "New File:" << bvi->getHeader()->getNextTime () << "\n";

        std::cout << "Samples: " << bvi->getHeader()->getNumSamples () << "\n";
        std::cout << "Size:    " << bvi->getHeader()->getBufSize()
            << " bytes, free: " << bvi->getHeader()->getBufFree() <<" bytes\n";

        std::cout << "Period:  " << value.getPeriod() << "\n";

        info = value->getCtrlInfo ();
        if (info)
            info->show (std::cout);
        else
            std::cout << "No CtrlInfo\n";

        std::cout << "Next:    " << bvi->getHeader()->getNextFile ()
            << " @ " << bvi->getHeader()->getNext () << "\n";
        std::cout << "\n";
    }
    while (run && bvi->nextBuffer ());
}

void test(const stdString &directory, const osiTime &start, const osiTime &end)
{
    Archive archive (new ARCHIVE_TYPE (directory));
    ChannelIterator channel(archive);
    ValueIterator value(archive);
    size_t chan_errors, errors = 0, channels = 0;

    archive.findFirstChannel (channel);
    while (run && channel)
    {
        std::cout << '.';
        std::cout.flush ();
        ++channels;
        channel->getValueAfterTime (start, value);
        chan_errors = 0;
        osiTime last;
        try
        {
            while (run && value && (!isValidTime (end) || value->getTime() < end))
            {
                if (isValidTime (last))
                {
                    if (value->getTime () < last)
                    {
                        std::cout << "\n" << channel->getName() << " back in time\n";
                        std::cout << last << "   is followed by:\n";
                        std::cout << *value << "\n";
                        ++chan_errors;
                    }
                }
                last = value->getTime ();
                ++value;
            }
        }
        catch (GenericException &e)
        {
            std::cout << "\n" << e.what () << "\n";
            ++chan_errors;
        }

        if (chan_errors)
        {
            errors += chan_errors;
            std::cout << "\n" << channel->getName() << " : " << chan_errors << " errors,\n";
        }
        ++channel;
    }
    std::cout << "\n";
    std::cout << channels << " channels\n";
    std::cout << errors << " errors\n";
}

//#define EXPERIMENT
#ifdef EXPERIMENT
// Ever changing experimental routine,
// usually empty in "useful" versions of the ArchiveManager
void experiment (const stdString &archive_name)
{
}
#endif

static void PrintRoutine (void *arg, const stdString &text)
{
    std::cout << text;
}

int main (int argc, const char *argv[])
{
    TheMsgLogger.SetPrintRoutine (PrintRoutine);
    initOsiHelpers ();

    CmdArgParser parser (argc, argv);
    parser.setHeader ("Archive Manager version " VERSION_TXT ", built " __DATE__ "\n\n");
    parser.setArgumentsInfo ("<archive>");

    CmdArgFlag   do_show_info   (parser, "info", "Show archive information");
    CmdArgFlag   do_test        (parser, "test", "Test archive for errors");
    CmdArgString channel_name   (parser, "channel", "<channel>", "Specify channel name");
    CmdArgString channel_pattern(parser, "match", "<regular expression>", "List matching channels");
    CmdArgString dump_channels  (parser, "Match", "<regular expression>", "Dump values for matching channels");
    CmdArgString start_text     (parser, "start", "<time>", "Start time as mm/dd/yyyy hh:mm:ss[.nano-secs]");
    CmdArgString end_text       (parser, "end", "<time>", "End time (exclusive)");
    CmdArgString export_archive (parser, "xport", "<new archive>", "export data into new archive");
    CmdArgInt    repeat_limit   (parser, "repeat_limit", "<seconds>", "remove 'repeat' entries beyond limit (export)");
    CmdArgInt    days_per_file  (parser, "FileSize", "<days>", "Days per binary data file (export, binary file format detail)");
    CmdArgString show_headers   (parser, "headers", "<channel>", "show headers for channel");
    CmdArgString ascii_output   (parser, "Output", "<channel>", "output ASCII dump for channel");
    CmdArgString ascii_input    (parser, "Input", "<ascii file>", "read ASCII dump for channel into archive");
    CmdArgString compare_target (parser, "Compare", "<target archive>", "Compare with target archive");
    CmdArgFlag   do_seek_test   (parser, "Seek", "Seek test (use with -start)");
#ifdef EXPERIMENT
    CmdArgFlag   do_experiment  (parser, "Experiment", "Perform experiment (temprary option)");
#endif

    if (! parser.parse ())
        return -1;

    if (parser.getArguments().size () != 1)
    {
        parser.usage ();
        return -1;
    }
    stdString archive_name = parser.getArgument (0);

    osiTime start, end;
    string2osiTime (start_text, start);
    string2osiTime (end_text, end);

    signal (SIGINT, signal_handler);
    signal (SIGTERM, signal_handler);

    try
    {
        if (do_show_info)
        {
            if (channel_name.get().length () > 0)
                show_channel_info (archive_name, channel_name);
            else
                show_info (archive_name, channel_pattern);
        }
#ifdef EXPERIMENT
        else if (do_experiment)
            experiment (archive_name);
#endif
        else if (do_test)
            test (archive_name, start, end);
        else if (ascii_output.get().length() > 0)
            output_ascii (archive_name, ascii_output.get(), start, end);
        else if (ascii_input.get().length() > 0)
            input_ascii (archive_name, ascii_input);
        else if (compare_target.get().length() > 0)
            compare (archive_name, compare_target);
        else if (do_seek_test)
            seek_time (archive_name, channel_name, start);
        else if (show_headers.get().length() > 0)
            headers (archive_name, show_headers);
        else if (dump_channels.get().length() > 0)
            dump (archive_name, dump_channels, start, end);
        else if (export_archive.get().length () > 0)
        {
            if (channel_pattern.get().empty ())
                channel_pattern.set (channel_name.get());
            do_export (archive_name, channel_pattern, start, end, export_archive, (size_t) repeat_limit.get(), days_per_file.get());
        }
        else if (channel_name.get().empty ())
            list_channels (archive_name, channel_pattern);
        else
            list_values (archive_name, channel_name, start, end);
    }
    catch (GenericException &e)
    {
        std::cerr <<  "Error:\n" << e.what () << "\n";
        return -1;
    }
    std::cout << "\n";

    return 0;
}
