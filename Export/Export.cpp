// Export.cpp

#include "BinArchive.h"
#include "GNUPlotExporter.h"
#include "ArgParser.h"

using namespace std;
USING_NAMESPACE_CHANARCH

// Global info variables
const char *prog_name;

void Usage ()
{
	cerr << "Usage: " << prog_name << " [flags] <directory file> { channel }\n";
	cerr << "Flags:\n";
	cerr << "\t-v            : verbose mode\n";
	cerr << "\t-g            : generate GNUPlot output\n";
	cerr << "\t-G            : generate GNUPlot output for gif image\n";
	cerr << "\t-t            : include text column for status information\n";
	cerr << "\t-o <filename> : output to file instead of stdout\n";
	cerr << "\t-m <pattern>  : use channels matching given regular expression pattern\n";
	cerr << "\t-s <time>     : start time as mm/dd/yy hh:mm:ss[.nano-secs]\n";
	cerr << "\t-e <time>     : end time (exclusive)\n";
	cerr << "\t-r <seconds>  : round all time stamps that are within 'seconds'\n";
	cerr << "\n";
}

int main (int argc, const char *argv[])
{
	prog_name = argv[0];
	ArgParser parser;

	if (! parser.parse (argc, argv, "vgGt", "mseor"))
	{
		Usage ();
		return -1;
	}
	if (parser.getArguments ().size() < 1)
	{
		cerr << "Error: Missing directory file name\n";
		Usage ();
		return -1;
	}

	bool be_verbose = parser.getFlag (0);
	bool GNUPlot = parser.getFlag (1);
	bool GIFPlot = parser.getFlag (2);
	bool generate_status_text = parser.getFlag (3);

	const stdString &pattern = parser.getParameter(0);
	const stdString &start_time = parser.getParameter(1);
	const stdString &end_time = parser.getParameter(2);
	const stdString &output = parser.getParameter (3);
	double round = atof (parser.getParameter (4).c_str());

	if (GIFPlot)
		GNUPlot = true;
	if (GNUPlot  &&  output.empty())
	{
		cerr << "Error:\n";
		cerr << "For GNUPlot output (-g or -G flag) you must specify\n";
		cerr << "an output file (-o)\n";
		Usage ();
		return -1;
	}

	try
	{
		ArchiveI *archive = new BinArchive (parser.getArgument (0));
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

		exporter->setVerbose (be_verbose);

		if (generate_status_text)
			exporter->enableStatusText ();

		if (round > 0)
			exporter->setTimeRounding (round);

		osiTime time;
		// start time provided ?
		if (! start_time.empty())
		{
			string2osiTime (start_time, time);
			exporter->setStart (time);
		}
		// end time provided ?
		if (! end_time.empty())
		{
			string2osiTime (end_time, time);
			exporter->setEnd (time);
		}

		// List of channels given?
		if (parser.getArguments ().size() > 1)
		{	// yes, use it
			vector<stdString>	channel_names;
			if (! pattern.empty())
			{
				cerr << "Pattern from '-m' switch is ignored\n";
				cerr << "since a list of channels was also provided\n";
			}
			// first argument was directory file name, cut that:
			channel_names = parser.getArguments ();
			channel_names.erase (channel_names.begin());

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
