// Manager:
//
// Could end up being a command-line tool
// for simple ChannelArchive management:
// * List channels
// * List values
// * Extract subsection into new archive
// * ??
//
// Right now it's more a collection of API tests.

// Warning: names too long for debug info
#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "BinArchive.h"
#include "ArgParser.h"
#include "BinValueIterator.h"
#include <signal.h>
#include "ExpandingValueIteratorI.h"
#include "Filename.h"
#include "ascii.h"

#ifndef MANAGER_VERSION
#define MANAGER_VERSION "?.?"
#endif

USING_NAMESPACE_CHANARCH
using namespace std;

stdString prog_name;

static bool be_verbose = false;

// For communication sigint_handler -> main loop
static bool run = true;
static void signal_handler (int sig)
{
    run = false;
	cout << "exiting on signal " << sig << " as soon as possible\n";
}                                                        

// List all channel names for given pattern
// (leave empty to list all channels)
void list_channels (const stdString &archive_name, const stdString &pattern)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator	channel (archive);

	archive.findChannelByPattern (pattern, channel);
	while (run && channel)
	{
		cout << channel->getName() << endl;
		++ channel;
	}
}

// List all the values stampes start <= value.getTime() < end for a given channel,
// iterated version.
// start == end == 0 will list all values.
void list_values (const stdString &archive_name, const stdString &channel_name,
				const osiTime &start, const osiTime &end)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator channel(archive);
	ValueIterator	value(archive);

	if (! archive.findChannelByName (channel_name, channel))
	{
		cerr << "Cannot find channel '" << channel_name << "':\n";
		return;
	}

	cout << "Channel " << channel->getName() << "\n";
	channel->getValueAfterTime (start, value);
	while (run && value && (end == nullTime  ||  value->getTime() < end))
	{
		cout << *value << endl;
		++ value;
	}
}

void dump (const stdString &archive_name, const stdString &channel_pattern,
	const osiTime &start, const osiTime &end)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator channel(archive);
	ValueIterator	value(archive);

	archive.findChannelByPattern (channel_pattern, channel);
	while (run && channel)
	{
		cout << "Channel " << channel->getName() << "\n";
		channel->getValueAfterTime (start, value);
		while (run && value && (end == nullTime  ||  value->getTime() < end))
		{
			cout << *value << endl;
			++ value;
		}
		++channel;
	}
}

void show_info (const stdString &archive_name, const stdString &pattern)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator	channel (archive);
	size_t channel_count = 0;
	osiTime start, end, time;

	for (archive.findChannelByPattern (pattern, channel); 	channel;  ++channel)
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
		cout	<< channel->getFirstTime () << "\t"
				<< channel->getLastTime () << "\t"
				<< channel->getName() << "\n";
	}
	cout << "Channel count : " << channel_count << endl;
	cout << "First sample  : " << start << endl;
	cout << "Last  sample  : " << end   << endl;
}

void show_channel_info (const stdString &archive_name, const stdString &channel_name)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator channel (archive);

	if (archive.findChannelByName (channel_name, channel))
	{
		cout << "start: " << channel->getFirstTime () << endl;
		cout << "end:   " << channel->getLastTime ()  << endl;
	}
	else
		cout << channel_name << ": not found\n";
}

void seek_time (const stdString &archive_name, const stdString &channel_name, const osiTime &start)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator channel (archive);
	ValueIterator	value (archive);

	if (! archive.findChannelByName (channel_name, channel))
	{
		cerr << "Cannot find channel '" << channel_name << "':\n";
		return;
	}

	cout << "Channel '" << channel->getName() << "\n";

	channel->getValueNearTime (start, value);
	if (value)
		cout << "Found: " << value->getTime () << endl;
	else
		cout << "Not found\n";
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
void export (const stdString &archive_name, 
			 const stdString &channel_pattern,
			 const osiTime &start, const osiTime &end,
			 const stdString &new_dir_name,
			 size_t repeat_limit)
{
	size_t i, chunk, chunk_count=0, val_count=0;
	osiTime next_file_time, last_stamp;

	Archive old_archive (new BinArchive (archive_name));
	BinArchive *new_archiveI = new BinArchive (new_dir_name, true);
	new_archiveI->setSecsPerFile (BinArchive::SECS_PER_MONTH);
	Archive new_archive (new_archiveI);

	ChannelIterator old_channel (old_archive);
	ChannelIterator new_channel (new_archive);
	ValueIterator values (old_archive);

	for (old_archive.findChannelByPattern (channel_pattern, old_channel); 
		run && old_channel;
		++old_channel)
	{
		cout << old_channel->getName () << flush;
		chunk_count = val_count = 0;

		try
		{
			if (! old_channel->getValueAfterTime (start, values))
			{
				cout << "\t" << "0 chunks\t0 values\n";
				continue;
			}
			new_archiveI->calcNextFileTime (*values, next_file_time);
			if (isValidTime(next_file_time)  &&  next_file_time < end)
				chunk = values.determineChunk (next_file_time);
			else
				chunk = values.determineChunk (end);
			if (chunk <= 0)
			{
				cout << "\t" << "0 chunks\t0 values\n";
				continue;
			}

			if (new_archive.findChannelByName (old_channel->getName (), new_channel))
			{
				last_stamp = new_channel->getLastTime ();
				if (isValidTime (start)  &&  last_stamp > start)
				{
					cout << "Error:\n";
					cout << "Archive " << new_dir_name << " does already contain\n";
					cout << "values for channel " << old_channel->getName () << "\n";
					cout << "and they are stamped " << last_stamp << ", which is after\n";
					cout << "the start time of    " << start << "\n";
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
				for (i=0;   values && i<chunk;   ++i, ++values)
				{
					if (isValidTime (last_stamp) && isValidTime(values->getTime()))	// time stamp checks
					{	
						if (values->getTime() < last_stamp)
						{
							cout << *values << "\tgoing back in time, skipped\n";
							continue;
						}
					}
					if (repeat_limit > 0 &&
						(values->getSevr() == ARCH_REPEAT || values->getSevr() == ARCH_EST_REPEAT) &&
						(size_t)values->getStat() >= repeat_limit)
					{
						cout << *values << "\trepeat count beyond " << repeat_limit << ", skipped\n";
						continue;
					}
					if (! new_channel->addValue (*values))
					{
						new_channel->addBuffer (*values, values.getPeriod(), chunk);
						new_channel->addValue (*values);
					}
					last_stamp = values->getTime();
				}
				++chunk_count;
				val_count += chunk;

				new_archiveI->calcNextFileTime (*values, next_file_time);
				if (isValidTime (next_file_time)  &&  next_file_time < end)
					chunk = values.determineChunk (next_file_time);
				else
					chunk = values.determineChunk (end);
			}
			new_channel->releaseBuffer ();
		}
		catch (GenericException &e)
		{
			cout <<  "\nError:\n" << e.what () << "\n";
		}

		cout << "\t" << chunk_count << " chunks\t" << val_count << " values\n";
	}
}

void compare (const stdString &archive_name, const stdString &target_name)
{
	Archive src (new BinArchive (archive_name));
	Archive dst (new BinArchive (target_name));
	ChannelIterator src_chan (src);
	ChannelIterator dst_chan (dst);
	ValueIterator src_val (src);
	ValueIterator dst_val (dst);

	for (src.findFirstChannel (src_chan); src_chan; ++src_chan)
	{
		cout << src_chan->getName () << " : ";

		if (! dst.findChannelByName (src_chan->getName (), dst_chan))
		{
			cout << "not found\n";
			continue;
		}

		src_chan->getFirstValue (src_val);
		dst_chan->getFirstValue (dst_val);
		while (src_val)
		{
			if (*src_val != *dst_val)
			{
				cout << "difference:\n";
				cout << *src_val << "\n";
				cout << *dst_val << "\n";
			}
			++src_val;
			++dst_val;
		}
		if (! src_val)
			cout << " OK\n";
	}
}

// Test ExpandingValueIteratorI
void expand (const stdString &archive_name, const stdString &channel_name,
				const osiTime &start, const osiTime &end)
{
	Archive			archive (new BinArchive (archive_name));
	ChannelIterator channel(archive);

	if (! archive.findChannelByName (channel_name, channel))
	{
		cerr << "Cannot find channel '" << channel_name << "':\n";
		return;
	}

	cout << "Channel '" << channel->getName() << "\n";

	ValueIterator	raw_values(archive);
	channel->getValueAfterTime (start, raw_values);
	ExpandingValueIteratorI *expand = new ExpandingValueIteratorI (raw_values);
	ValueIterator value (expand);
	while (value && (end == nullTime  ||  value->getTime() < end))
	{
		cout << *value;
		if (expand->isExpanded())
			cout << "\t(expanded)";
		cout << endl;
		++value;
	}

	cout << "reverse:\n";
	--value; // move from 'after end' to 'last value'
	while (value && (start == nullTime  || value->getTime() > start))
	{
		cout << *value;
		if (expand->isExpanded())
			cout << "\t(expanded)";
		cout << endl;
		--value;
	}
}

// Interpolation Test
void interpolate (const stdString &directory, const stdString &channel_name,
				const osiTime &start, const osiTime &end,
				double interpol)
{
#if 0
	Archive	a (directory);
	ChannelIterator channel = a.findChannelByName (channel_name);
	if (! channel)
	{
		cerr << "Cannot find channel '" << channel_name << "':\n";
		return;
	}

	cout << "Channel '" << channel->getName() << "\n";

	ValueIterator vi = channel.getValueAfterTime (start);
	ExpandingValueIterator evi (&vi);
	LinInterpolValueIterator livi (&evi, interpol);
	InfoFilterValueIterator value (&livi);
	while (value && (end == nullTime  ||  value->getTime() < end))
	{
		cout << *value << endl;
		++value;
	}
#endif
}

// This is low-level.
// The API shown in here should not be used by
// general Archive clients
void headers (const stdString &directory, const stdString &channel_name)
{
	Archive	archive (new BinArchive (directory));
	ChannelIterator channel (archive);
	
	if (! archive.findChannelByName (channel_name, channel))
	{
		cerr << "Cannot find channel '" << channel_name << "':\n";
		return;
	}

	ValueIterator value (archive);
	if (! channel->getFirstValue (value))
	{
		cerr << "No values for '" << channel_name << "':\n";
		return;
	}

	const CtrlInfoI *info;
	BinValueIterator *bvi = dynamic_cast<BinValueIterator *>(value.getI());
	do
	{
		cout << "Buffer:  " << bvi->getHeader().getFilename()
			<< " @ " << bvi->getHeader().getOffset () << "\n";
		cout << "Prev:    " << bvi->getHeader()->getPrevFile ()
			<< " @ " << bvi->getHeader()->getPrev () << "\n";
		cout << "Time:    " << bvi->getHeader()->getBeginTime() << "\n";
		cout << "...      " << bvi->getHeader()->getEndTime () << "\n";
		cout << "New File:" << bvi->getHeader()->getNextTime () << "\n";

		cout << "Samples: " << bvi->getHeader()->getNumSamples () << "\n";
		cout << "Size:    " << bvi->getHeader()->getBufSize()
			<< " bytes, free: " << bvi->getHeader()->getBufFree() << " bytes\n";

		cout << "Period:  " << value.getPeriod() << "\n";

		info = value->getCtrlInfo ();
		if (info)
			info->show (cout);
		else
			cout << "No CtrlInfo\n";

		cout << "Next:    " << bvi->getHeader()->getNextFile ()
			<< " @ " << bvi->getHeader()->getNext () << "\n";
		cout << "\n";
	}
	while (run && bvi->nextBuffer ());
}

void test (const stdString &directory, const osiTime &start, const osiTime &end)
{
	Archive	archive (new BinArchive (directory));
	ChannelIterator channel(archive);
	ValueIterator value(archive);
	size_t chan_errors, errors = 0, channels = 0;

	archive.findFirstChannel (channel);
	while (run && channel)
	{
		cout << '.';
		cout.flush ();
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
						cout << "\n" << channel->getName() << " back in time\n";
						cout << last << "   is followed by:\n";
						cout << *value << "\n";
						++chan_errors;
					}
				}
				last = value->getTime ();
				++value;
			}
		}
		catch (GenericException &e)
		{
			cout << "\n" << e.what () << "\n";
			++chan_errors;
		}

		if (chan_errors)
		{
			errors += chan_errors;
			cout << "\n" << channel->getName() << " : " << chan_errors << " errors,\n";
		}
		++channel;
	}
	cout << "\n";
	cout << channels << " channels\n";
	cout << errors << " errors\n";
}

void Usage ()
{
	cerr << "Archive Manager version " MANAGER_VERSION ", built " __DATE__ "\n";
	cerr << "\n";
	cerr << "Usage: " << prog_name << " [options] <archive>\n";
	cerr << "\toptions:\n";
	cerr << "\t-i                     : show archive information\n";
	cerr << "\t-m <regular expression>: list matching names\n";
	cerr << "\t-M <regular expression>: dump values for matching names\n";
	cerr << "\t-c <channel name>      : use given channel\n";
	cerr << "\t-s <time>              : start time as mm/dd/yyyy hh:mm:ss[.nano-secs]\n";
	cerr << "\t-e <time>              : end time (exclusive)\n";
	cerr << "\t-x <new directory file>: export data into new archive\n";
	cerr << "\t-r <repeat count limit>: for export: remove all 'repeat' entries beyond limit\n";
	cerr << "\t-h <channel name>      : show headers for channel\n";
	cerr << "\t-O <channel name>      : output ASCII dump for channel\n";
	cerr << "\t-I <dump file>         : read ASCII dump for channel into archive\n";
	cerr << "\t-C <target>            : Compare: test if all in archive is found in target\n";
	cerr << "\t-S <time>              : Seek test\n";
	cerr << "\t-T                     : Test archive\n";
}

static void PrintRoutine (void *arg, const stdString &text)
{
	cout << text;
}

int main (int argc, const char *argv[])
{
	TheMsgLogger.SetPrintRoutine (PrintRoutine);
	initOsiHelpers ();

	signal (SIGINT, signal_handler);
	signal (SIGTERM, signal_handler);

	bool seek_test = false;
	bool show_headers = false;
	bool dump_values = false;
	bool dump_ascii = false;
	bool read_ascii = false;
	Filename::getBasename (argv[0], prog_name);
	size_t repeat_limit = 0;

	ArgParser	parser;
	if (! parser.parse (argc, argv, "ivET", "cmsexIShCMOrj"))
	{
		Usage ();
		return -1;
	}
	// Create nicely named refs for flags/parms/args
	bool do_info = parser.getFlag (0);
	be_verbose = parser.getFlag (1);
	bool expand_test = parser.getFlag (2);
	bool the_test = parser.getFlag (3);
	stdString channel_name = parser.getParameter(0);
	stdString channel_pattern = parser.getParameter(1);
	stdString start_text = parser.getParameter(2);
	const stdString &end_text = parser.getParameter(3);
	const stdString &new_dir_name = parser.getParameter(4);
	if (! parser.getParameter(11).empty ())
	{
		repeat_limit = (size_t) atol(parser.getParameter(11).c_str());
	}
	stdString file_name;
	if (! parser.getParameter(6).empty ())
	{
		start_text = parser.getParameter(6);
		seek_test = true;
	}
	if (! parser.getParameter(7).empty ())
	{
		channel_name = parser.getParameter(7);
		show_headers = true;
	}
	stdString compare_target = parser.getParameter(8);
	if (! parser.getParameter(9).empty ())
	{
		channel_pattern = parser.getParameter(9);
		dump_values = true;
	}
	if (! parser.getParameter(10).empty ())
	{
		channel_name = parser.getParameter(10);
		dump_ascii = true;
	}
	if (! parser.getParameter(5).empty ())
	{
		file_name = parser.getParameter(5);
		read_ascii = true;
	}

	if (parser.getArguments().size () != 1)
	{
		cerr << "No Directory file specified\n";
		Usage ();
		return -1;
	}
	const stdString &archive_name = parser.getArgument (0);

	osiTime start, end;
	string2osiTime (start_text, start);
	string2osiTime (end_text, end);

	try
	{
		if (the_test)
			test (archive_name, start, end);
		else if (dump_ascii)
			output_ascii (archive_name, channel_name, start, end);
		else if (read_ascii)
			input_ascii (archive_name, file_name);
		else if (do_info)
		{
			if (channel_name.empty ())
				show_info (archive_name, channel_pattern);
			else
				show_channel_info (archive_name, channel_name);
		}
		else if (! compare_target.empty())
			compare (archive_name, compare_target);
		else if (seek_test)
			seek_time (archive_name, channel_name, start);
		else if (show_headers)
			headers (archive_name, channel_name);
		else if (dump_values)
			dump (archive_name, channel_pattern, start, end);
		else if (new_dir_name.length ())
		{
			if (channel_pattern.empty ())
			{
				channel_pattern = channel_name;
			}
			export (archive_name, channel_pattern, start, end, new_dir_name, repeat_limit);
		}
		else if (channel_name.empty ())
			list_channels (archive_name, channel_pattern);
		else
		{
			if (expand_test)
				expand (archive_name, channel_name, start, end);
			else
				list_values (archive_name, channel_name, start, end);
		}
	}
	catch (GenericException &e)
	{
		LOG_MSG ("Error:\n" << e.what () << "\n");
		cerr <<  "Error:\n" << e.what () << "\n";
		return -1;
	}
	cout << endl;

	return 0;
}
