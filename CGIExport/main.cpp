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

#include "BinArchive.h"
#include "MultiArchive.h"

// getcwd
#ifdef WIN32
#include <direct.h>
#include <process.h>
#else
#include <unistd.h>
#endif

stdString GNUPlot = GNUPLOT_PROGRAM;

void Usage(HTMLPage &page)
{
	page.header ("Usage", 2);
    std::cout << "This program uses the CGI interface,\n";
	std::cout << "both GET/HEAD and POST are supported.\n";
	std::cout << "<P>\n";
	page.header ("Recognized form parameters", 3);
	std::cout << "<UL>\n";
	std::cout << "<LI>COMMAND: START | HELP | LIST | INFO | PLOT | GET | DEBUG\n";
	std::cout << "<LI>DIRECTORY: Archive's directory file\n";
	std::cout << "<LI>and maybe more...\n";
	std::cout << "</UL>\n";
	std::cout << "When started with COMMAND=START,\n";
	std::cout << "a start page should be created that links to the remaining commands....\n";
}

void showEnvironment (HTMLPage &page, char *envp[])
{
	size_t i;
	page.header ("Environment",2);
	std::cout << "<PRE>\n";
	for (i=0; envp[i]; ++i)
		std::cout << envp[i] << "\n";
	std::cout << "</PRE>\n";
}

// "visitor" for BinaryTree of channel names
static void listChannelsTraverser (const stdString &item, void *arg)
{
	std::cout << "<TR><TD>"
		 << item.c_str()
		 << "</TR></TD>\n";
}

void listChannels (HTMLPage &page)
{
	BinaryTree<stdString> channels;

	page.start ();
	try
	{
		Archive archive (new CGIEXPORT_ARCHIVE_TYPE (page._directory));
		ChannelIterator channel (archive);
		archive.findChannelByPattern (page._pattern, channel);

		while (channel)
		{
			channels.add (channel->getName());
			++channel;
		}
	}
	catch (GenericException &e)
	{
		page.header ("Archive Error",2);
		std::cout << "<PRE>\n";
		std::cout << e.what () << "\n";
		std::cout << "</PRE>\n";
	}
	page.header ("Channel List", 2);
	std::cout << "<TABLE BORDER=1 CELLPADDING=1>\n";
	channels.traverse (listChannelsTraverser);
	std::cout << "</TABLE>\n";
	page.interFace ();
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
	std::cout << "<TR><TD>" << info.channel
		<< "</TD><TD>" << info.first
		<< "</TD><TD>" << info.last
		<< "</TD></TR>\n";
}

void listInfo (HTMLPage &page)
{
	page.start ();
	BinaryTree<Info>	infos;
	Info				info;
	try
	{
		Archive archive (new CGIEXPORT_ARCHIVE_TYPE (page._directory));
		ChannelIterator channel (archive);

		if (page._names.empty())
		{
			archive.findChannelByPattern (page._pattern, channel);
			while (channel)
			{
				info.channel = channel->getName();
				info.first   = channel->getFirstTime();
				info.last    = channel->getLastTime();
				infos.add (info);
				++channel;
			}
		}
		else
		{
			for (size_t i=0; i<page._names.size(); ++i)
			{
				if (archive.findChannelByName (page._names[i], channel))
				{
					info.channel = channel->getName();
					info.first   = channel->getFirstTime();
					info.last    = channel->getLastTime();
				}
				else
				{
					info.channel = page._names[i];
					info.channel += " -not found-";
					info.first   = nullTime;
					info.last    = nullTime;
				}
				infos.add (info);
			}
		}
	}
	catch (GenericException &e)
	{
		page.header ("Archive Error",2);
		std::cout << "<PRE>\n";
		std::cout << e.what () << "\n";
		std::cout << "</PRE>\n";
		return;
	}
	page.header ("Channel Info", 2);
	std::cout << "<TABLE BORDER=1 CELLPADDING=1>\n";
	std::cout << "<TR><TH>Channel</TH><TH>First archived</TH><TH>Last archived</TH></TR>\n";
	infos.traverse (listInfoTraverser);
	std::cout << "</TABLE>\n";
	page.interFace ();
}

void getNames (const stdString &input_string, stdVector<stdString> &names)
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
        std::strstream buf;
		// 12/22/1998 11:50:00
		buf << cgi.find ("STARTMONTH")
			<< '/' << cgi.find ("STARTDAY")
			<< '/' << cgi.find ("STARTYEAR")
			<< ' ' << cgi.find ("STARTHOUR")
			<< ':' << cgi.find ("STARTMINUTE")
			<< ':' << cgi.find ("STARTSECOND") << '\0';
		start_txt = buf.str();
		buf.rdbuf()->freeze (false);
		buf.clear ();

		if (! string2osiTime (start_txt, start))
			return false;
	}
	if (!cgi.find ("ENDMONTH").empty())
	{
		stdString end_txt;
        std::strstream buf;
		buf << cgi.find ("ENDMONTH")
			<< '/' << cgi.find ("ENDDAY")
			<< '/' << cgi.find ("ENDYEAR")
			<< ' ' << cgi.find ("ENDHOUR")
			<< ':' << cgi.find ("ENDMINUTE")
			<< ':' << cgi.find ("ENDSECOND") << '\0';
		end_txt = buf.str();
		buf.rdbuf()->freeze (false);
		buf.clear ();

		if (! string2osiTime (end_txt, end))
			return false;
	}

	return true;
}

bool exportFunc (HTMLPage &page, bool useGNU=false, const char *temp_file_base=0)
{
	bool have_data = false;
	stdString tempfilebase;
	if (temp_file_base)
		tempfilebase = temp_file_base;

	try
	{
		ArchiveI *archive = new CGIEXPORT_ARCHIVE_TYPE (page._directory);
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
			std::cout << "Content-type: text/plain\n";
			std::cout << "\n";
		}

		exporter->setStart (page._start);
		exporter->setEnd (page._end);
		if (page._fill)
			exporter->useFilledValues ();
		if (page._status)
			exporter->enableStatusText (true);
		if (page._round > 0.0)
			exporter->setTimeRounding (page._round);
		if (page._interpol > 0.0)
			exporter->setLinearInterpolation (page._interpol);
		if (page._names.empty())
			exporter->exportMatchingChannels (page._pattern);
		else
			exporter->exportChannelList (page._names);

		have_data = exporter->getDataCount () > 0;

		delete exporter;
		delete archive;
	}
	catch (GenericException &e)
	{
		std::cout << "Archive Error :\n";
		std::cout << "===============\n";
		std::cout << e.what () << "\n";
		return false;
	}
	catch (const char *txt)
	{
		std::cout << "Caught Error :\n";
		std::cout << "===============\n";
		std::cout << txt << "\n";
		return false;
	}
	catch (...)
	{
		std::cout << "Caught Unknown Error\n";
		return false;
	}

	return have_data;
}

static void PrintRoutine (void *arg, const stdString &text)
{
    std::cerr << text;
}

void help (HTMLPage &page)
{
	page._title = "Help";
	page.start ();
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
	TheMsgLogger.SetPrintRoutine (PrintRoutine);
#ifdef WEB_DIRECTORY
	chdir (WEB_DIRECTORY);
#endif

	HTMLPage	page;
	page._title = "EPICS Channel Archive";
	page._cgi_path = getenv ("SCRIPT_NAME");

	stdString script_dir, script;
	Filename::getDirname  (page._cgi_path, script_dir);
	Filename::getBasename (page._cgi_path, script);

	CGIInput cgi;

	// Allow command line arguments that override CGI parms for testing:
	bool command_line = false;
	CmdArgParser parser (argc, argv);
	parser.setHeader ("This command is meant to be executed via CGI\n"
		"For debugging, these options are available:\n\n");

	CmdArgString directory (parser, "directory", "<archive>", "Specify archive file");
	CmdArgString command   (parser, "command", "<text>", "Command to execute");
	if (! parser.parse ())
		return 1;

	// Trick CGIInput by setting some variables from command line:
	if (command.get().length() > 0)
	{
		cgi.add ("COMMAND", command.get());
		command_line = true;
	}
	if (directory.get().length() > 0)
	{
		cgi.add ("DIRECTORY", directory.get());
		command_line = true;
	}
	if (!command_line)
	{
		if (!cgi.parse (std::cin, std::cout))
		{
			page._title = "CGI Error";
			page.start ();
			showEnvironment (page, envp);
			return 0;
		}
	}

	// Configure HTMLPage from CGIInput
	page._command = cgi.find ("COMMAND");
	page._directory = cgi.find ("DIRECTORY");
	page._pattern = cgi.find ("PATTERN");
	page._round = atof (cgi.find ("ROUND").c_str());
	page._interpol = atof (cgi.find ("INTERPOL").c_str());
	page._fill = cgi.find ("FILL").length()>0;
	page._status = cgi.find ("STATUS").length()>0;
	getNames (cgi.find ("NAMES"), page._names);
	if (! decodeTimes (cgi, page._start, page._end))
	{
		page._title = "Time Error";
		page.start ();
		std::cout << "Cannot decode times.\n";
		return 0;
	}

	// ----------------------------------------------------
	// Dispatch commands
	// ----------------------------------------------------
	if (page._command.empty() || page._command == "HELP")
	{
		help (page);
		return 0;
	}

	if (page._command == "DEBUG")
	{
		page._title = "DEBUG Information";
		page.start ();
		page.header ("DEBUG Information",1);

		std::cout << "<I>CGIExport Version " VERSION_TXT ", built " __DATE__ "</I>\n";

		page.header ("Variables",2);
		const stdMap<stdString, stdString> &var_map = cgi.getVars ();
		stdMap<stdString, stdString>::const_iterator vars = var_map.begin();
		while (vars != var_map.end())
		{
			std::cout << "'" << vars->first << "' = '" << vars->second << "'<BR>\n";
			++vars;
		}

		char dir[100];
		getcwd (dir, sizeof dir);
		page.header ("Script Info", 2);

		page.header ("Script:", 3);
		std::cout << "<PRE>" << script << "</PRE><P>\n";

		page.header ("Script path:", 3);
		std::cout << "<PRE>" << script_dir << "</PRE><P>\n";

		page.header ("Current directory:", 3);
		std::cout << "<PRE>" << dir << "</PRE><P>\n";

		page.header ("GNUPlot:", 3);
		std::cout << "<PRE>" << GNUPlot << "</PRE><P>\n";

#ifndef WIN32
		page.header ("Personality:", 3);
		std::cout << "UID: " << getuid() << '/' << geteuid();
		std::cout << ", GID: " << getgid() << '/' << getegid() << "<P>\n";
#endif

		showEnvironment (page, envp);

		std::cout << "<HR>\n";
		page.interFace ();
		return 0;
	}

	if (page._directory.empty())
	{
		page._title = "Error: DIRECTORY not set";
		page.start ();
		page.header ("Error: DIRECTORY not set",1);
		Usage (page);
		return 0;
	}

	if (page._command == "LIST")
	{
		listChannels (page);
		return 0;
	}

	if (page._command == "GET")
	{
		exportFunc (page);
		return 0;
	}

	if (page._command == "PLOT")
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

		page.start ();
		page.header ("Channel Plot", 2);
		if (! exportFunc (page, true, physical))
		{
			page.header ("Error: No data",3);
			std::cout << "There seems to be no data for that channel and time range.\n";
			page.interFace ();
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
			std::cout << "The execution of GNUPlot to generate your plot failed:<P>\n";
			std::cout << "<PRE>'" << GNUPlotCommand << "'</PRE><P>\n";
			std::cout << "Result: " << result << "<P>\n";
			std::cout << "Possible Reasons:\n";
			std::cout << "<UL>\n";
			std::cout << "<LI>No data? Use <B>GET</B> to check this\n";
			std::cout << "<LI>Constant data so GNUPlot doesn't know how to draw the axis?<BR>";
			std::cout << "Use <B>GET</B> to check this\n";
			std::cout << "<LI>Gnuplot not installed?\n";
			std::cout << "</UL>\n";
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
		std::cout << "<IMG SRC=\"" + GNUPlotImage + "\"</A><P>\n";
		std::cout << "<HR>\n";
		page.interFace ();

		return 0;
	}

	if (page._command == "INFO")
	{
		listInfo (page);
		return 0;
	}

	if (page._command == "START")
	{
		page.start ();
		page.header ("Channel Archive CGI Interface", 1);
		page.interFace ();
		return 0;
	}

	help (page);

	return 0;
}
