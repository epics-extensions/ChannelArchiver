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

#include"../ArchiverConfig.h"
#include"Exporter.h"
#include"GNUPlotExporter.h"
#include"MatlabExporter.h"
#include<ArgParser.h>
#include<Filename.h>
#include<BinaryTree.h>
#include<RegularExpression.h>
#include"HTMLPage.h"
#include"CGIInput.h"
#include"BinArchive.h"
#include"MultiArchive.h"

// getcwd
#ifdef WIN32
#include<direct.h>
#include<process.h>
#else
#include<unistd.h>
#endif

static stdString script_dir, script;


// Called on error (after setting title) or as part of cmdHelp
static void usage(HTMLPage &page)
{
	page.header("Usage", 2);
    std::cout << "This program uses the CGI interface,\n";
	std::cout << "both GET/HEAD and POST are supported.\n";
	std::cout << "<P>\n";
	page.header("Recognized form parameters", 3);
	std::cout << "<UL>\n";
	std::cout << "<LI>COMMAND: "
        "START | HELP | LIST | INFO | PLOT | GET | DEBUG\n";
#ifdef NO_CGI_DEBUG
	std::cout << "<br>(DEBUG is disabled)\n";
#endif
    std::cout << "<LI>DIRECTORY: Archive's directory file, often 'freq_directory'\n";
	std::cout << "<LI>PATTERN:   Regular expression for channel names\n";
	std::cout << "<LI>GLOB:      If defined, shell glob is used instead of Regular expression\n";
	std::cout << "<LI>NAMES:     List of channel names\n";
    std::cout << "<LI>FORMAT:    PLOT | MATLAB | EXCEL | SPREADSHEET<br>\n"
              << "(used by COMMAND=GET)\n";
	std::cout << "<LI>STATUS:    Show channel status (disconnected, ...)\n";
	std::cout << "<LI>INTERPOL:  Use linear Interpolation (seconds)\n";
    std::cout << "<LI>FILL:      Step-interpolation, repeat value to fill gaps\n";
    std::cout << "<LI>STARTMONTH, STARTDAY, STARTYEAR, STARTHOUR, STARTMINUTE, STARTSECOND\n";
    std::cout << "<LI>ENDMONTH, ENDDAY, ENDYEAR, ENDHOUR, ENDMINUTE, ENDSECOND\n";
    std::cout << "<LI>and maybe more...\n";
	std::cout << "</UL>\n";
	std::cout << "When started with COMMAND=START,\n";
	std::cout << "a start page should be created that links to the remaining commands....\n";
}

// COMMAND==HELP
static void cmdHelp(HTMLPage &page)
{
	page._title = "Help";
	page.start ();
	usage(page);
}

static void showEnvironment(HTMLPage &page, const char *envp[])
{
	size_t i;
	page.header("Environment",2);
	std::cout << "<PRE>\n";
	for (i=0; envp[i]; ++i)
		std::cout << envp[i] << "\n";
	std::cout << "</PRE>\n";
}

// "visitor" for BinaryTree of channel names
static void cmdListTraverser(const stdString &item, void *arg)
{  
   static int namesCnt = 0;
   HTMLPage *pagep = (HTMLPage*)arg;
   if (++namesCnt <= 100) pagep->_names.push_back(item);
   if (namesCnt == 101) std::cout << "<b>List truncated to first 100 names!</b><p>\n";
}

// COMMAND==LIST
static void cmdList(HTMLPage &page)
{
	BinaryTree<stdString> channels;
	page.start();
	try
	{
	   Archive archive(new CGIEXPORT_ARCHIVE_TYPE(page._directory, page._start, page._end));
	   ChannelIterator channel(archive);
	   
	   if (page._glob)
	   {
	      stdString expr = RegularExpression::fromGlobPattern(page._pattern);
	      archive.findChannelByPattern(expr, channel);
	   }
	   else
	      archive.findChannelByPattern(page._pattern, channel);
	   
	   page._names.clear();
	   while (channel)
	   {
	      channels.add(channel->getName());
	      ++channel;
	   }
	}
	catch (GenericException &e)
	{
		page.header("Archive Error",2);
		std::cout << "<PRE>\n";
		std::cout << e.what() << "\n";
		std::cout << "</PRE>\n";
	}
	page.header("Channel List", 2);
	channels.traverse(cmdListTraverser, &page);
	page.interFace();
}

class Info
{
public:
	stdString channel;
	osiTime first;
	osiTime last;
};

static bool operator < (const class Info &a, const class Info &b)
{	return a.channel < b.channel; }

// Visitor routine for BinaryTree<Info>
static void cmdInfoTraverser(const Info &info, void *arg)
{
   std::cout << "<TR><TD>" << info.channel
	     << "</TD><TD>" << info.first
	     << "</TD><TD>" << info.last
	     << "</TD></TR>\n";
}

// COMMAND==INFO
static void cmdInfo(HTMLPage &page)
{
	page.start();
	BinaryTree<Info> infos;
	Info			 info;
	try
	{
		Archive archive(new CGIEXPORT_ARCHIVE_TYPE(page._directory, page._start, page._end));
		ChannelIterator channel(archive);

		if (page._names.empty())
		{
            if (page._glob)
            {
                stdString expr =
                    RegularExpression::fromGlobPattern(page._pattern);
                archive.findChannelByPattern(expr, channel);
            }
            else
                archive.findChannelByPattern(page._pattern, channel);
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
				if (archive.findChannelByName(page._names[i], channel))
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
		page.header("Archive Error",2);
		std::cout << "<PRE>\n";
		std::cout << e.what() << "\n";
		std::cout << "</PRE>\n";
		return;
	}
	page.header("Channel Info", 2);
	std::cout << "<TABLE BORDER=1 CELLPADDING=1>\n";
	std::cout << "<TR><TH>Channel</TH><TH>First archived</TH><TH>Last archived</TH></TR>\n";
	infos.traverse(cmdInfoTraverser);
	std::cout << "</TABLE>\n";
	page.interFace ();
}

// Turn string of names into vector of names
static void getNames(const stdString &input_string,
                     stdVector<stdString> &names)
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
		while (*input && !strchr(delim, *input) && i<sizeof(name)-1)
			name[i++] = *(input++);

		while (*input && strchr(delim, *input)) // skip multiple delims.
			++input;

		name[i] = '\0';
		names.push_back(name);
		i = 0;
	}
}

// Get start/end time from CGIInput
static bool decodeTimes (const CGIInput &cgi, osiTime &start, osiTime &end)
{
	int year, month, day, hour, min, sec; 
	unsigned long nano;

	osiTime2vals(osiTime::getCurrent(), year, month, day, hour, min, sec, nano);
	vals2osiTime(year, month, day, 0, 0, 0, 0, start);
	vals2osiTime(year, month, day, 23, 59, 59, 0, end);

	if (!cgi.find("STARTMONTH").empty())
	{
		stdString start_txt;
        std::strstream buf;
		// 12/22/1998 11:50:00
		buf << cgi.find("STARTMONTH")
			<< '/' << cgi.find("STARTDAY")
			<< '/' << cgi.find("STARTYEAR")
			<< ' ' << cgi.find("STARTHOUR")
			<< ':' << cgi.find("STARTMINUTE")
			<< ':' << cgi.find("STARTSECOND") << '\0';
		start_txt = buf.str();
		buf.rdbuf()->freeze(false);
		buf.clear();

		if (! string2osiTime(start_txt, start))
			return false;
	}
	if (!cgi.find("ENDMONTH").empty())
	{
		stdString end_txt;
        std::strstream buf;
		buf << cgi.find("ENDMONTH")
			<< '/' << cgi.find("ENDDAY")
			<< '/' << cgi.find("ENDYEAR")
			<< ' ' << cgi.find("ENDHOUR")
			<< ':' << cgi.find("ENDMINUTE")
			<< ':' << cgi.find("ENDSECOND") << '\0';
		end_txt = buf.str();
		buf.rdbuf()->freeze(false);
		buf.clear();

		if (! string2osiTime(end_txt, end))
			return false;
	}
    if (start > end)
    {
        osiTime t = start;
        start = end;
        end = t;
    }

	return true;
}

typedef enum
{
    fmt_Spreadsheet,
    fmt_Excel,
    fmt_GNUPlot,
    fmt_Matlab
} Format;

// Export data as configured for HTMLPage (names, start, end, ...)
// to stdout (or temp_file_base files for GNUPlot).
// Result: Any data?
static bool exportFunc(HTMLPage &page, Format format,
                       const char *temp_file_base=0)
{
	bool have_data = false;
	stdString tempfilebase;
	if (temp_file_base)
		tempfilebase = temp_file_base;

	try
	{
		ArchiveI *archive = new CGIEXPORT_ARCHIVE_TYPE(page._directory, page._start, page._end);
		Exporter *exporter;

		if (format == fmt_GNUPlot)
		{
			GNUPlotExporter *gnu = 
			   new GNUPlotExporter(archive, tempfilebase, page._reduce?600:0);
			gnu->makeImage();
#ifndef WIN32
            // Pipe based on _pipe call only works for console apps.
            // Within the web server it doesn't seem to function.
			gnu->usePipe();
#endif
			exporter = gnu;
		}
		else
		if (format == fmt_Matlab)
		{
			exporter = new MatlabExporter (archive);
			std::cout << "Content-type: application/octet-stream\n";
			std::cout << "Content-Disposition: filename=\"archive_data.m\"\n";
            std::cout << "Content-Description: EPICS ChannelArchiver Data\n";
			std::cout << "\n";
		}
        else
		if (format == fmt_Excel)
		{
			exporter = new SpreadSheetExporter (archive);
			std::cout << "Content-type: application/ms-excel\n";
			std::cout << "Content-Disposition: filename=\"archive_data.xls\"\n";
            std::cout << "Content-Description: EPICS ChannelArchiver Data\n";
			std::cout << "\n";
		}
		else
		{
			exporter = new SpreadSheetExporter (archive);
			std::cout << "Content-type: text/plain\n";
			std::cout << "Content-Disposition: filename=\"archive_data\"\n";
            std::cout << "Content-Description: EPICS ChannelArchiver Data\n";
			std::cout << "\n";
		}

		exporter->setStart(page._start);
		exporter->setEnd(page._end);
		if (page._fill)
			exporter->useFilledValues();
		if (page._status)
			exporter->enableStatusText(true);
		if (page._interpol > 0.0)
			exporter->setLinearInterpolation(page._interpol);
		if (page._names.empty())
        {
            if (page._glob)
            {
                stdString expr =
                    RegularExpression::fromGlobPattern(page._pattern);
                exporter->exportMatchingChannels(expr);
            }
            else
                exporter->exportMatchingChannels(page._pattern);
        }
		else
			exporter->exportChannelList(page._names);
		have_data = exporter->getDataCount() > 0;

		delete exporter;
		delete archive;
	}
	catch (GenericException &e)
	{
		std::cout << "Archive Error :\n";
		std::cout << "<PRE>\n";
		std::cout << e.what() << "\n";
		std::cout << "</PRE>\n";
		return false;
	}
	catch (const char *txt)
	{
		std::cout << "Caught Error :\n";
		std::cout << "<PRE>\n";
		std::cout << txt << "\n";
		std::cout << "</PRE>\n";
		return false;
	}
	catch (...)
	{
		std::cout << "Caught Unknown Error\n";
		return false;
	}

	return have_data;
}

static void getTimeTxt(char *result)
{
	int year, month, day, hour, min, sec;
	unsigned long nano;
	osiTime2vals(osiTime::getCurrent(),
                 year, month, day, hour, min, sec, nano);
	sprintf(result, "%02d%02d%02d%02d%02d", month, day, hour, min, sec);
}

static void getFilenames(stdString &data,
                         stdString &dataURL,
                         stdString &imageURL)
{
    // Construct path for temp. files (data & GNUPlot command file)
    const char *client = getenv("REMOTE_HOST");
    if (client == 0) client = getenv("REMOTE_ADDR");
    if (client == 0) client = "unknown";

    // GNUPlot operates on the "real", physical directory,
    // but the GIF it creates will be accessed through the Web server's
    // virtual directory
    char physical[300];
#ifdef USE_RELATIVE_PHYSICAL_PATH
    getcwd(physical, sizeof physical);
#else
    physical[0] = '\0';
#endif
    char timetext[40];
    getTimeTxt(timetext);
    
    strcat(physical, PHYSICAL_PATH);
    strcat(physical, timetext);
    strcat(physical, "_");
    strcat(physical, client);
    data = physical;
    
    dataURL.reserve(200);
#ifdef USE_RELATIVE_URL
    dataURL = script_dir;
#endif
    dataURL += URL_PATH;
    dataURL += timetext;
    dataURL += "_";
    dataURL += client;
    
    imageURL.reserve(dataURL.length() + 5);
    imageURL = dataURL;
    imageURL += GNUPlotExporter::imageExtension();
}

static bool cmdPlot(HTMLPage &page)
{
    stdString data, dataURL, imageURL;

    getFilenames(data, dataURL, imageURL);

    
    page.start();
    page.header("Channel Plot", 2);
    if (! exportFunc(page, fmt_GNUPlot, data.c_str()))
    {
        page.header("Error: Possible reasons",3);
        std::cout << "<ul>\n";
        std::cout << "<li>No data<br>\n";
        std::cout << "    There seems to be no data for that channel\n";
        std::cout << "    and time range.\n";
        std::cout << "    Check the <A HREF=\"" << dataURL << "\">";
        std::cout << "    data that was supposed to be plotted.</A>\n";
        std::cout << "<li>Internal GNUPlot error<br>\n";
        std::cout << "    If you can't get <i>any</i> plot today\n";
        std::cout << "    even though the data is available as\n";
        std::cout << "    a spreadsheet, the plotting is broken.\n";
        std::cout << "</ul>\n";
        page.interFace();
        return 0;
    }
#ifdef WIN32
    else
    {
        stdString GNUPlotCommand;
        GNUPlotCommand = data;
        GNUPlotCommand += ".plt";
        int result = _spawnl(_P_WAIT, GNUPLOT_PROGRAM, GNUPLOT_PROGRAM,
                             GNUPlotCommand.c_str(), 0);
        if (result != 0)
        {
            page.header("Error: GNUPlot",3);
            std::cout << "The execution of GNUPlot plot failed:<P>\n";
            std::cout << "<PRE>'" << GNUPLOT_PROGRAM
                      << " " << GNUPlotCommand << "'</PRE><P>\n";
            std::cout << "Result: " << result << "<P>\n";
            return 0;
        }
    }
#endif
    
    std::cout << "<A HREF=\"" << dataURL << "\">";
    std::cout << "<IMG SRC=\""
              << imageURL
              << "\" ALT=\"Click Image to see raw data\"</A></A><P>\n";
    std::cout << "<HR>\n";
    page.interFace();
    
    return 0;
}

// Command == DEBUG
void cmdDebug(HTMLPage &page, const CGIInput &cgi, const char *envp[])
{
    page._title = "DEBUG Information";
    page.start();
    page.header("DEBUG Information",1);
    
    std::cout << "<I>CGIExport Version " VERSION_TXT
        ", built " __DATE__ "</I>\n";
    
    page.header("Variables parsed from CGI input",2);
    const stdMap<stdString, stdString> &var_map = cgi.getVars ();
    stdMap<stdString, stdString>::const_iterator vars = var_map.begin();
    while (vars != var_map.end())
    {
        std::cout << "'" << vars->first << "' = '"
                  << vars->second << "'<BR>\n";
        ++vars;
    }
    
    char dir[100];
    getcwd(dir, sizeof dir);
    page.header("Script Info", 2);

    page.header("Current directory:", 3);
    std::cout << "<PRE>" << dir << "</PRE><P>\n";

    page.header("Script:", 3);
    std::cout << "<PRE>" << script << "</PRE><P>\n";
    
    page.header("Script path:", 3);
    std::cout << "<PRE>" << script_dir << "</PRE><P>\n";
    
    page.header("GNUPlot Program:", 3);
    std::cout << "<PRE>" << GNUPLOT_PROGRAM << "</PRE><P>\n";

    page.header("GNUPlot Pipe:", 3);
    std::cout << "<PRE>" << GNUPLOT_PIPE << "</PRE><P>\n";

    stdString data, dataURL, imageURL;
    getFilenames(data, dataURL, imageURL);

    page.header("Data:", 3);
    std::cout << "<PRE>" << data << "</PRE><P>\n";

    page.header("Data URL:", 3);
    std::cout << "<PRE>" << dataURL << "</PRE><P>\n";
    
    page.header("Image URL:", 3);
    std::cout << "<PRE>" << imageURL << "</PRE><P>\n";
    
#ifndef WIN32
    page.header("Personality:", 3);
    std::cout << "UID: " << getuid() << '/' << geteuid();
    std::cout << ", GID: " << getgid() << '/' << getegid() << "<P>\n";
#endif
    
    showEnvironment(page, envp);
    
    std::cout << "<HR>\n";
    page.interFace();
}

// For MsgLogger
static void PrintRoutine(void *arg, const stdString &text)
{   std::cerr << text; }

int main(int argc, const char *argv[], const char *envp[])
{
	TheMsgLogger.SetPrintRoutine(PrintRoutine);
#ifdef WEB_DIRECTORY
	chdir(WEB_DIRECTORY);
#endif

	HTMLPage	page;
	page._title = "EPICS Channel Archive";
	page._cgi_path = getenv ("SCRIPT_NAME");
	Filename::getDirname (page._cgi_path, script_dir);
	Filename::getBasename(page._cgi_path, script);

	CGIInput cgi;

	// For testing, allow command line arguments that override CGI parms:
	bool command_line = false;
	CmdArgParser parser(argc, argv);
	parser.setHeader ("This command is meant to be executed via CGI\n"
                      "For debugging, these options are available:\n\n");
	CmdArgString directory(parser, "directory", "<archive>",
                           "Specify archive file");
	CmdArgString command  (parser, "command", "<text>",
                           "Command to execute");
	CmdArgString fmt      (parser, "format", "<text>",
                           "Format for GET command");
	if (! parser.parse())
		return 1;

	// Trick CGIInput by setting some variables from command line:
	if (command.get().length() > 0)
	{
		cgi.add("COMMAND", command.get());
		command_line = true;
	}
	if (directory.get().length() > 0)
	{
		cgi.add("DIRECTORY", directory.get());
		command_line = true;
	}
	if (fmt.get().length() > 0)
	{
		cgi.add("FORMAT", fmt.get());
		command_line = true;
	}
	if (!command_line)
	{
		if (!cgi.parse(std::cin, std::cout))
		{
			page._title = "CGI Error";
			page.start();
			showEnvironment(page, envp);
			return 0;
		}
	}

	// Configure HTMLPage from CGIInput
	page._command = cgi.find("COMMAND");
	page._format = cgi.find("FORMAT");
	page._directory = cgi.find("DIRECTORY");
	page._pattern = cgi.find("PATTERN");
	page._interpol = atof(cgi.find ("INTERPOL").c_str());
	page._reduce = cgi.find("REDUCE").length()==0;
	page._glob = cgi.find("GLOB").length()>0;
	page._fill = cgi.find("FILL").length()>0;
	page._status = cgi.find("STATUS").length()>0;
	getNames(cgi.find("NAMES"), page._names);
	if (! decodeTimes(cgi, page._start, page._end))
	{
		page._title = "Time Error";
		page.start();
		std::cout << "Cannot decode times.\n";
        showEnvironment(page, envp);
		return 0;
	}

	// ----------------------------------------------------
	// Dispatch commands
	// ----------------------------------------------------
	if ((page._command.empty() && page._directory.empty()) || page._command == "HELP")
	{
		cmdHelp(page);
		return 0;
	}

	if (page._command.empty() || page._command == "START")
	{   // Show empty interface which has most of the buttons etc.
		page.start();
		page.header("Channel Archive CGI Interface", 1);
		page.interFace();
		return 0;
	}

#ifndef NO_CGI_DEBUG
	if (page._command == "DEBUG")
	{
        cmdDebug(page, cgi, envp);
		return 0;
	}
#endif

	if (page._directory.empty())
	{
		page._title = "Error: DIRECTORY not set";
		page.start();
		page.header("Error: DIRECTORY not set",1);
		usage(page);
		return 0;
	}

	if (page._command == "LIST")
	{
		cmdList(page);
		return 0;
	}

	if (page._command == "GET")
	{
        Format format = fmt_Spreadsheet;
        
        if (page._format == "EXCEL")
            format = fmt_Excel;
        else
        if (page._format == "MATLAB")
            format = fmt_Matlab;
        else
        if (page._format == "PLOT")
            format = fmt_GNUPlot;
            
        if (format == fmt_GNUPlot)
            cmdPlot(page);
        else
            exportFunc(page, format);

		return 0;
	}

	if (page._command == "PLOT")
	{
        cmdPlot(page);
        return 0;
	}

	if (page._command == "INFO")
	{
		cmdInfo(page);
		return 0;
	}

	cmdHelp(page);

	return 0;
}
