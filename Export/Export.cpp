// Export.cpp

#include "../ArchiverConfig.h"
#include "BinArchive.h"
#include "MultiArchive.h"
#include "GNUPlotExporter.h"
#include "ArgParser.h"

using namespace std;
USING_NAMESPACE_CHANARCH

int main (int argc, const char *argv[])
{
	CmdArgParser parser (argc, argv);
	parser.setArgumentsInfo (" <directory file> { channel }");
	CmdArgFlag   verbose     (parser, "verbose", "Verbose mode");
	CmdArgFlag   GNUPlot     (parser, "gnuplot", "generate GNUPlot output mode");
	CmdArgFlag   GIFPlot     (parser, "GIF", "generate GNUPlot output for gif image");
	CmdArgFlag   status_text (parser, "text", "include text column for status information");
	CmdArgString output      (parser, "output", "<file>", "output to file instead of stdout");
	CmdArgString pattern     (parser, "match", "<pattern>", "reg. expr. pattern for channel names");
	CmdArgString start_time  (parser, "start", "<time>", "start time as mm/dd/yy hh:mm:ss[.nano-secs]pattern");
	CmdArgString end_time    (parser, "end", "<time>", "end time (exclusive)");
	CmdArgDouble round       (parser, "round", "<seconds>", "round time stamps if within 'seconds'");
	CmdArgDouble interpol    (parser, "interpolate", "<seconds>", "interpolate values");

	if (! parser.parse ())
		return -1;
	if (parser.getArguments ().size() < 1)
	{
		parser.usage ();
		return -1;
	}

	if (GIFPlot)
		GNUPlot.set (true);
	if (GNUPlot  &&  output.get().empty())
	{
		cerr << "Error:\n";
		cerr << "For GNUPlot output (-g or -G flag) you must specify\n";
		cerr << "an output file (-o)\n\n";
		parser.usage ();
		return -1;
	}

	try
	{
		ArchiveI *archive = new EXPORT_ARCHIVE_TYPE (parser.getArgument (0));
		Exporter *exporter;
		
		if (GNUPlot)
		{
			GNUPlotExporter *gnu = new GNUPlotExporter (archive, output);
			if (GIFPlot)
				gnu->makeImage ();
			exporter = gnu;
		}
		else
			exporter = new SpreadSheetExporter (archive, output);

		exporter->setVerbose (verbose);

		if (status_text)
			exporter->enableStatusText ();
		if (double(round) > 0)
			exporter->setTimeRounding (round);
		if (double(interpol) > 0)
			exporter->setLinearInterpolation (interpol);

		osiTime time;
		// start time provided ?
		if (! start_time.get().empty())
		{
			string2osiTime (start_time, time);
			exporter->setStart (time);
		}
		// end time provided ?
		if (! end_time.get().empty())
		{
			string2osiTime (end_time, time);
			exporter->setEnd (time);
		}

		// List of channels given?
		if (parser.getArguments ().size() > 1)
		{	// yes, use it
			vector<stdString>	channel_names;
			if (! pattern.get().empty())
			{
				cerr << "Pattern from '-m' switch is ignored\n";
				cerr << "since a list of channels was also provided\n";
			}
			// first argument was directory file name, skip that:
			for (size_t i=1; i<parser.getArguments ().size(); ++i)
				channel_names.push_back (parser.getArgument(i));
			exporter->exportChannelList (channel_names);
		}
		else
			exporter->exportMatchingChannels (pattern);
		delete archive;
	}
	catch (const GenericException &e)
	{
		cerr << "Exception:\n" << e.what () << endl;
		LOG_MSG ("Exception:\n" << e.what () << endl);

		return -1;
	}

	return 0;
}
