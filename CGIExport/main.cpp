// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "../ArchiverConfig.h"
#include "GNUPlotExporter.h"
#include <ArgParser.h>
#include <Filename.h>
#include <BinaryTree.h>
#include "HTMLPage.h"
#include "CGIInput.h"

// Use "Bin" archive:
#include "BinArchive.h"
#define ARCHIVE_TYPE BinArchive

// getcwd
#ifdef WIN32
#include <direct.h>
#include <process.h>
#else
#include <unistd.h>
#endif

using namespace std;
USING_NAMESPACE_CHANARCH

stdString GNUPlot = GNUPLOT_PROGRAM;

void Usage(HTMLPage &page)
{
	page.header ("Usage", 2);
	cout << "This program uses the CGI interface,\n";
	cout << "both GET/HEAD and POST are supported.\n";
	cout << "<P>\n";
	page.header ("Recognized form parameters", 3);
	cout << "<UL>\n";
	cout << "<LI>COMMAND: START | HELP | LIST | INFO | PLOT | GET | DEBUG\n";
	cout << "<LI>DIRECTORY: Archive's directory file\n";
	cout << "<LI>and maybe more...\n";
	cout << "</UL>\n";
	cout << "When started with COMMAND=START,\n";
	cout << "a start page should be created that links to the remaining commands....\n";
}

void showEnvironment (HTMLPage &page, char *envp[])
{
	size_t i;
	page.header ("Environment",2);
	cout << "<PRE>\n";
	for (i=0; envp[i]; ++i)
		cout << envp[i] << endl;
	cout << "</PRE>\n";
}

// "visitor" for BinaryTree of channel names
static void listChannelsTraverser (const stdString &item, void *arg)
{
	cout << "<TR><TD>"
		 << item.c_str()
		 << "</TR></TD>\n";
}

void listChannels (HTMLPage &page, const stdString &archive_name, const stdString &pattern)
{
	BinaryTree<stdString> channels;

	try
	{
		Archive archive (new ARCHIVE_TYPE (archive_name));
		ChannelIterator channel (archive);
		archive.findChannelByPattern (pattern, channel);

		while (channel)
		{
			channels.add (channel->getName());
			++channel;
		}
	}
	catch (GenericException &e)
	{
		page.header ("Archive Error",2);
		cout << "<PRE>\n";
		cout << e.what () << endl;
		cout << "</PRE>\n";
	}
	page.header ("Channel List", 2);
	cout << "<TABLE BORDER=1 CELLPADDING=1>\n";
	channels.traverse (listChannelsTraverser);
	cout << "</TABLE>\n";
}

class Info
{
public:
	stdString channel;
	osiTime first;
	osiTime last;
};

bool operator < (const class Info &a, const class Info &b)
{	return a.channel < b.channel; }

// Visitor routine for BinaryTree<Info>
static void listInfoTraverser (const Info &info, void *arg)
{
	cout << "<TR><TD>" << info.channel
		<< "</TD><TD>" << info.first
		<< "</TD><TD>" << info.last
		<< "</TD></TR>\n";
}

void listInfo (HTMLPage &page, const stdString &archive_name, const vector<stdString> &names)
{
	BinaryTree<Info>	infos;
	Info				info;
	try
	{
		Archive archive (new ARCHIVE_TYPE (archive_name));
		ChannelIterator channel (archive);
		
		for (size_t i=0; i<names.size(); ++i)
		{
			if (archive.findChannelByName (names[i], channel))
			{
				info.channel = channel->getName();
				info.first   = channel->getFirstTime();
				info.last    = channel->getLastTime();
			}
			else
			{
				info.channel = names[i];
				info.channel += " -not found-";
				info.first   = nullTime;
				info.last    = nullTime;
			}
			infos.add (info);
		}
	}
	catch (GenericException &e)
	{
		page.header ("Archive Error",2);
		cout << "<PRE>\n";
		cout << e.what () << endl;
		cout << "</PRE>\n";
	}
	page.header ("Channel Info", 2);
	cout << "<TABLE BORDER=1 CELLPADDING=1>\n";
	cout << "<TR><TH>Channel</TH><TH>First archived</TH><TH>Last archived</TH></TR>\n";
	infos.traverse (listInfoTraverser);
	cout << "</TABLE>\n";
}

void listInfo (HTMLPage &page, const stdString &archive_name, const stdString &pattern)
{
	BinaryTree<Info>	infos;
	Info				info;
	try
	{
		Archive archive (new ARCHIVE_TYPE (archive_name));
		ChannelIterator channel (archive);
		
		archive.findChannelByPattern (pattern, channel);
		while (channel)
		{
			info.channel = channel->getName();
			info.first   = channel->getFirstTime();
			info.last    = channel->getLastTime();
			infos.add (info);
			++channel;
		}
	}
	catch (GenericException &e)
	{
		page.header ("Archive Error",2);
		cout << "<PRE>\n";
		cout << e.what () << endl;
		cout << "</PRE>\n";
	}
	page.header ("Channel Info", 2);
	cout << "<TABLE BORDER=1 CELLPADDING=1>\n";
	cout << "<TR><TH>Channel</TH><TH>First archived</TH><TH>Last archived</TH></TR>\n";
	infos.traverse (listInfoTraverser);
	cout << "</TABLE>\n";
}

void getNames (const stdString &input_string, vector<stdString> &names)
{
	const char *input = input_string.c_str();
	const char *delim = " ,\t;\n\r";

	if (! input)
		return;
	
	char name[300];
	size_t i = 0;

	while (*input)
	{
		// find end, next delim or buffer overrrun
		while (*input && !strchr (delim, *input) && i<sizeof(name)-1)
			name[i++] = *(input++);

		while (*input && strchr (delim, *input)) // skip multiple delims.
			++input;

		name[i] = '\0';
		names.push_back (name);
		i = 0;
	}
}

bool decodeTimes (CGIInput &cgi, osiTime &start, osiTime &end)
{
	int year, month, day, hour, min, sec; 
	unsigned long nano;

	osiTime2vals (osiTime::getCurrent (), year, month, day, hour, min, sec, nano);
	vals2osiTime (year, month, day, 0, 0, 0, 0, start);
	vals2osiTime (year, month, day, 23, 59, 59, 0, end);

	if (!cgi.find ("STARTMONTH").empty())
	{
		stdString start_txt;
		strstream buf;
		// 12/22/1998 11:50:00
		buf << cgi.find ("STARTMONTH")
			<< '/' << cgi.find ("STARTDAY")
			<< '/' << cgi.find ("STARTYEAR")
			<< ' ' << cgi.find ("STARTHOUR")
			<< ':' << cgi.find ("STARTMINUTE")
			<< ':' << cgi.find ("STARTSECOND") << '\0';
		start_txt = buf.str();
		buf.freeze (false);
		buf.clear ();

		if (! string2osiTime (start_txt, start))
		{
			HTMLPage	page;
			page.start ("Error in start time");
			page.header ("Error in start time:", 3);
			cout << "Cannot decode '" << start_txt << "'<BR>\n";
			return false;
		}
	}
	if (!cgi.find ("ENDMONTH").empty())
	{
		stdString end_txt;
		strstream buf;
		buf << cgi.find ("ENDMONTH")
			<< '/' << cgi.find ("ENDDAY")
			<< '/' << cgi.find ("ENDYEAR")
			<< ' ' << cgi.find ("ENDHOUR")
			<< ':' << cgi.find ("ENDMINUTE")
			<< ':' << cgi.find ("ENDSECOND") << '\0';
		end_txt = buf.str();
		buf.freeze (false);
		buf.clear ();

		if (! string2osiTime (end_txt, end))
		{
			HTMLPage	page;
			page.start ("Error in end time");
			page.header ("Error in end time:", 3);
			cout << "Cannot decode '" << end_txt << "'<BR>\n";
			return false;
		}
	}

	return true;
}

bool exportFunc (const stdString &archive_name,
			 const stdString &pattern, const vector<stdString> &names,
			 const osiTime &start, const osiTime &end,
			 double round, bool fill, bool status, bool useGNU, const stdString &tempfilebase)
{
	bool have_data = false;

	try
	{
		ArchiveI *archive = new ARCHIVE_TYPE (archive_name);
		Exporter *exporter;

		if (useGNU)
		{
			GNUPlotExporter *gnu = new GNUPlotExporter (archive, tempfilebase);
			gnu->makeImage ();
			gnu->setPath ();
			exporter = gnu;
		}
		else
		{
			exporter = new SpreadSheetExporter (archive, tempfilebase);
			cout << "Content-type: text/plain\n";
			cout << "\n";
		}

		exporter->setStart (start);
		exporter->setEnd (end);
		if (fill)
			exporter->useFilledValues ();
		if (status)
			exporter->enableStatusText (true);
		if (round > 0.0)
			exporter->setTimeRounding (round);
		if (names.empty())
			exporter->exportMatchingChannels (pattern);
		else
			exporter->exportChannelList (names);

		have_data = exporter->getDataCount () > 0;

		delete exporter;
		delete archive;
	}
	catch (GenericException &e)
	{
		cout << "Archive Error :\n";
		cout << "===============\n";
		cout << e.what () << endl;
		return false;
	}
	catch (const char *txt)
	{
		cout << "Caught Error :\n";
		cout << "===============\n";
		cout << txt << endl;
		return false;
	}
	catch (...)
	{
		cout << "Caught Unknown Error\n";
		return false;
	}

	return have_data;
}

static void PrintRoutine (void *arg, const stdString &text)
{
	cerr << text;
}

void help ()
{
	HTMLPage	page;
	page.start ("Help");
	Usage (page);
}

void getTimeTxt (char *result)
{
	int year, month, day, hour, min, sec;
	unsigned long nano;
	osiTime2vals (osiTime::getCurrent(), year, month, day, hour, min, sec, nano);
	sprintf (result, "%02d%02d%02d%02d%02d", month, day, hour, min, sec);
}

int main (int argc, const char *argv[], char *envp[])
{
#ifdef WEB_DIRECTORY
	chdir (WEB_DIRECTORY);
#endif

	TheMsgLogger.SetPrintRoutine (PrintRoutine);

	stdString title ("EPICS Channel Archive");
	stdString command;
	stdString directory;
	stdString pattern;

	stdString script_path, script_dir, script;
	script_path = getenv ("SCRIPT_NAME");
	Filename::getDirname  (script_path, script_dir);
	Filename::getBasename (script_path, script);

	CGIInput cgi;

#ifdef _DEBUG
	// Allow command line arguments that override CGI parms
	// for testing:
	bool command_line = false;
	ArgParser parser;
	if (! parser.parse (argc, argv, "h", "cdp")  ||
		parser.getFlag(0))
	{
		clog << "Usage: " << argv[0] << "[-h] -d <directory> -c <command> -p <pattern>\n";
		return 1;
	}

	if (!parser.getParameter (0).empty())
	{
		command = parser.getParameter (0);
		command_line = true;
	}
	if (!parser.getParameter (1).empty())
	{
		directory = parser.getParameter (1);
		command_line = true;
	}
	if (!parser.getParameter (2).empty())
	{
		pattern = parser.getParameter (2);
		command_line = true;
	}
	if (!command_line)
#endif
	{
		if (!cgi.parse (cin, cout))
		{
			HTMLPage	page;
			page.start ("CGI Error");
			showEnvironment (page, envp);
			return 0;
		}
		command = cgi.find ("COMMAND");
		directory = cgi.find ("DIRECTORY");
		pattern = cgi.find ("PATTERN");
	}

	vector<stdString>	names;
	getNames (cgi.find ("NAMES"), names);
	double round = atof (cgi.find ("ROUND").c_str());
	bool fill = cgi.find ("FILL").length()>0;
	bool status = cgi.find ("STATUS").length()>0;
	osiTime	start, end;
	decodeTimes (cgi, start, end);

	if (command.empty() || command == "HELP")
	{
		help ();
		return 0;
	}

	if (command == "DEBUG")
	{
		HTMLPage	page;
		page.start ("DEBUG Information");
		page.header ("DEBUG Information",1);

		cout << "<I>CGIExport Version " << VERSION << "." << RELEASE << ", built " __DATE__ "</I>\n";

		page.header ("Variables",2);
		const map<stdString, stdString> &var_map = cgi.getVars ();
		map<stdString, stdString>::const_iterator vars = var_map.begin();
		while (vars != var_map.end())
		{
			cout << "'" << vars->first << "' = '" << vars->second << "'<BR>\n";
			++vars;
		}

		char dir[100];
		getcwd (dir, sizeof dir);
		page.header ("Script Info", 2);

		page.header ("Script:", 3);
		cout << "<PRE>" << script << "</PRE><P>\n";

		page.header ("Script path:", 3);
		cout << "<PRE>" << script_dir << "</PRE><P>\n";

		page.header ("Current directory:", 3);
		cout << "<PRE>" << dir << "</PRE><P>\n";

		page.header ("GNUPlot:", 3);
		cout << "<PRE>" << GNUPlot << "</PRE><P>\n";

#ifndef WIN32
		page.header ("Personality:", 3);
		cout << "UID: " << getuid() << '/' << geteuid();
		cout << ", GID: " << getgid() << '/' << getegid() << "<P>\n";
#endif

		showEnvironment (page, envp);

		cout << "<HR>\n";
		page.interFace (script_path, directory, pattern, names, round, fill, status, start, end);
		return 0;
	}

	if (directory.empty())
	{
		HTMLPage	page;
		page.start ("Error: DIRECTORY not set");
		page.header ("Error: DIRECTORY not set",1);
		Usage (page);
		return 0;
	}

	if (command == "LIST")
	{
		HTMLPage	page;
		page.start (title);
		listChannels (page, directory, pattern);
		cout << "<HR>\n";
		page.interFace (script_path, directory, pattern, names, round, fill, status, start, end);

		return 0;
	}

	if (command == "GET")
	{
		exportFunc (directory, pattern, names, start, end, round, fill, status, false, "");
		return 0;
	}

	if (command == "PLOT")
	{
		const char *client = getenv ("REMOTE_HOST");
		if (client == 0) client = getenv ("REMOTE_ADDR");
		if (client == 0) client = "unknown";
	 
		// GNUPlot operates on the "real", physical directory,
		// but the GIF it creates will be accessed through the Web server's
		// virtual directory
		char physical[300];
		char timetext[40];
		getTimeTxt (timetext);
#ifdef USE_RELATIVE_PHYSICAL_PATH
		getcwd (physical, sizeof physical);
#else
		physical[0] = '\0';
#endif
		strcat (physical, PHYSICAL_PATH);
		strcat (physical, timetext);
		strcat (physical, "_");
		strcat (physical, client);

		HTMLPage	page;
		page.start (title);
		page.header ("Channel Plot", 2);
		if (! exportFunc (directory, pattern, names, start, end, round, true, false, true, physical))
		{
			page.header ("Error: No data",3);
			cout << "There seems to be no data for that channel and time range.\n";
			page.interFace (script_path, directory, pattern, names, round, fill, status, start, end);
			return 0;
		}

#ifdef WIN32
		// On Win9x, system() will lauch command.com
		// which really messes things up,
		// starting with a temporarily appearing command prompt window
		// and sometimes hanging the WWW server.
		// -> Circumvent command.com, start GNUPlot directly:
		stdString GNUPlotCommand = stdString(physical) + ".plt";
		int result = _spawnl (_P_WAIT, GNUPlot.c_str(), GNUPlot.c_str(), GNUPlotCommand.c_str(), 0);

		// Still build the complete GNUPlotCommand for further debugging
		GNUPlotCommand = GNUPlot + " " + GNUPlotCommand;
#else
		stdString GNUPlotCommand = GNUPlot + " " + physical + ".plt";
		int result = system (GNUPlotCommand.c_str());
#endif
		if (result != 0)
		{
			page.header ("Error: GNUPlot",3);
			cout << "The execution of GNUPlot to generate your plot failed:<P>\n";
			cout << "<PRE>'" << GNUPlotCommand << "'</PRE><P>\n";
			cout << "Result: " << result << "<P>\n";
			cout << "Possible Reasons:\n";
			cout << "<UL>\n";
			cout << "<LI>No data? Use <B>GET</B> to check this\n";
			cout << "<LI>Constant data so GNUPlot doesn't know how to draw the axis?<BR>";
			cout << "Use <B>GET</B> to check this\n";
			cout << "<LI>Gnuplot not installed?\n";
			cout << "</UL>\n";
			return 0;
		}

#		ifdef USE_RELATIVE_URL
		stdString GNUPlotImage = script_dir;
#		else
		stdString GNUPlotImage;
#		endif

		GNUPlotImage += URL_PATH;
		GNUPlotImage += timetext;
		GNUPlotImage += "_";
		GNUPlotImage += client;
		GNUPlotImage += GNUPlotExporter::imageExtension ();
		cout << "<IMG SRC=\"" + GNUPlotImage + "\"</A><P>\n";
		cout << "<HR>\n";
		page.interFace (script_path, directory, pattern, names, round, fill, status, start, end);

		return 0;
	}

	if (command == "INFO")
	{
		HTMLPage	page;
		page.start (title);
		if (names.empty())
			listInfo (page, directory, pattern);
		else
			listInfo (page, directory, names);
		cout << "<HR>\n";
		page.interFace (script_path, directory, pattern, names, round, fill, status, start, end);
		return 0;
	}

	if (command == "START")
	{
		HTMLPage	page;
		page.start (title);
		page.header ("Channel Archive CGI Interface", 1);
		page.interFace (script_path, directory, pattern, names, round,
			/*fill*/true, /*status*/false, start, end);
		return 0;
	}

	help ();

	return 0;
}
