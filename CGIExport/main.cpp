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

#include <epicsVersion.h>
#include<ArgParser.h>
#include<Filename.h>
#include<BinaryTree.h>
#include<RegularExpression.h>
#include"ArchiverConfig.h"
#include"Exporter.h"
#include"GNUPlotExporter.h"
#include"MatlabExporter.h"
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
	page.header("CGIExport", 2);
    printf("CGIExport version " VERSION_TXT ", "
           EPICS_VERSION_STRING
           ", built " __DATE__ ", " __TIME__);
    page.header("Usage", 2);
    printf("This program uses the CGI interface,\n");
	printf("both GET/HEAD and POST are supported.\n");
	printf("<P>\n");
	page.header("Recognized form parameters", 3);
	printf("<UL>\n");
	printf("<LI>COMMAND: "
           "START | HELP | LIST | INFO | PLOT | GET | DEBUG\n");
#ifdef NO_CGI_DEBUG
	printf("<br>(DEBUG is disabled)\n");
#endif
    printf("<LI>DIRECTORY: Archive's directory file, often 'freq_directory'\n");
	printf("<LI>PATTERN:   Regular expression for channel names\n");
	printf("<LI>GLOB:      If defined, shell glob is used instead of Regular expression\n");
	printf("<LI>NAMES:     List of channel names\n");
    printf("<LI>FORMAT:    PLOT | MATLAB | MLSHEET | SPREADSHEET<br>\n"
           "(used by COMMAND=GET)\n");
	printf("<LI>STATUS:    Show channel status (disconnected, ...)\n");
	printf("<LI>INTERPOL:  Use linear Interpolation (seconds)\n");
    printf("<LI>FILL:      Step-interpolation, repeat value to fill gaps\n");
    printf("<LI>STARTMONTH, STARTDAY, STARTYEAR, STARTHOUR, STARTMINUTE, STARTSECOND\n");
    printf("<LI>ENDMONTH, ENDDAY, ENDYEAR, ENDHOUR, ENDMINUTE, ENDSECOND\n");
    printf("<LI>and maybe more...\n");
	printf("</UL>\n");
	printf("When started with COMMAND=START,\n");
	printf("a start page should be created that links to the remaining commands....\n");
}

// COMMAND==HELP
static void cmdHelp(HTMLPage &page)
{
	page._title = "Help";
	page.start();
	usage(page);
}

static void showEnvironment(HTMLPage &page, const char *envp[])
{
	size_t i;
	page.header("Environment",2);
	printf("<PRE>\n");
	for (i=0; envp[i]; ++i)
		printf("%s\n", envp[i]);
	printf("</PRE>\n");
}

// "visitor" for BinaryTree of channel names
static void cmdListTraverser(const stdString &item, void *arg)
{  
   static int namesCnt = 0;
   HTMLPage *pagep = (HTMLPage*)arg;
   if (++namesCnt <= 100)
       pagep->_names.push_back(item);
   if (namesCnt == 101)
       printf("<b>List truncated to first 100 names!</b><p>\n");
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
		printf("<PRE>\n");
        printf("%s\n", e.what());
        printf("</PRE>\n");
	}
	page.header("Channel List", 2);
	channels.traverse(cmdListTraverser, &page);
	page.interFace();
}

class Info
{
public:
	stdString channel;
	epicsTime first;
	epicsTime last;
};

static bool operator < (const class Info &a, const class Info &b)
{	return a.channel < b.channel; }

// Visitor routine for BinaryTree<Info>
static void cmdInfoTraverser(const Info &info, void *arg)
{
    stdString f, l;

    epicsTime2string(info.first, f);
    epicsTime2string(info.last, l);
    printf("<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n",
           info.channel.c_str(), f.c_str(), l.c_str());
}

// COMMAND==INFO
static void cmdInfo(HTMLPage &page)
{
	page.start();

	BinaryTree<Info> infos;
	Info			 info;
	try
	{
		Archive archive(new CGIEXPORT_ARCHIVE_TYPE(page._directory));
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
		printf("<PRE>\n");
		printf("%s\n", e.what());
		printf("</PRE>\n");
		return;
	}
	page.header("Channel Info", 2);
	printf("<TABLE BORDER=1 CELLPADDING=1>\n");
	printf("<TR><TH>Channel</TH><TH>First archived</TH><TH>Last archived</TH></TR>\n");
	infos.traverse(cmdInfoTraverser);
	printf("</TABLE>\n");

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
static bool decodeTimes(const CGIInput &cgi, epicsTime &start, epicsTime &end)
{
	int year, month, day, hour, min, sec; 
	unsigned long nano;

	epicsTime2vals(epicsTime::getCurrent(), year, month, day, hour, min, sec, nano);
	vals2epicsTime(year, month, day, 0, 0, 0, 0, start);
	vals2epicsTime(year, month, day, 23, 59, 59, 0, end);

	if (!cgi.find("STARTMONTH").empty())
	{
        vals2epicsTime(atoi(cgi.find("STARTYEAR").c_str()),
                       atoi(cgi.find("STARTMONTH").c_str()),
                       atoi(cgi.find("STARTDAY").c_str()),
                       atoi(cgi.find("STARTHOUR").c_str()),
                       atoi(cgi.find("STARTMINUTE").c_str()),
                       atoi(cgi.find("STARTSECOND").c_str()), 0, start);
	}
	if (!cgi.find("ENDMONTH").empty())
	{
        vals2epicsTime(atoi(cgi.find("ENDYEAR").c_str()),
                       atoi(cgi.find("ENDMONTH").c_str()),
                       atoi(cgi.find("ENDDAY").c_str()),
                       atoi(cgi.find("ENDHOUR").c_str()),
                       atoi(cgi.find("ENDMINUTE").c_str()),
                       atoi(cgi.find("ENDSECOND").c_str()), 0, end);
	}
    if (start > end)
    {
        epicsTime t = start;
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
    fmt_Matlab,
    fmt_MatlabSheet
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
		ArchiveI *archive =
            new CGIEXPORT_ARCHIVE_TYPE(page._directory, page._start,page._end);
		Exporter *exporter;

		if (format == fmt_GNUPlot)
		{
			GNUPlotExporter *gnu = 
			   new GNUPlotExporter(archive, tempfilebase, page._reduce?600:0);
			gnu->makeImage();
            gnu->setY0(page._y0);
            gnu->setY1(page._y1);
            if (page._use_logscale)
                gnu->useLogscale();
#ifndef WIN32
            // Pipe based on _pipe call only works for console apps.
            // Within the WIN32 web server it doesn't seem to function.
			gnu->usePipe();
#endif
			exporter = gnu;
		}
		else
		if (format == fmt_Matlab)
		{
			exporter = new MatlabExporter(archive);
			printf("Content-type: application/octet-stream\n");
			printf("Content-Disposition: filename=\"archive_data.m\"\n");
            printf("Content-Description: EPICS ChannelArchiver Data\n");
			printf("\n");
		}
        else
		if (format == fmt_MatlabSheet)
		{
            SpreadSheetExporter *sse = new SpreadSheetExporter(archive);
            sse->useMatlabFormat();
			exporter = sse;
			printf("Content-type: text/plain\n");
			printf("Content-Disposition: filename=\"archive_data\"\n");
            printf("Content-Description: EPICS ChannelArchiver Data\n");
			printf("\n");
		}
        else
		if (format == fmt_Excel)
		{
			exporter = new SpreadSheetExporter (archive);
			printf("Content-type: application/ms-excel\n");
			printf("Content-Disposition: filename=\"archive_data.xls\"\n");
            printf("Content-Description: EPICS ChannelArchiver Data\n");
			printf("\n");
		}
		else
		{
			exporter = new SpreadSheetExporter (archive);
			printf("Content-type: text/plain\n");
			printf("Content-Disposition: filename=\"archive_data\"\n");
            printf("Content-Description: EPICS ChannelArchiver Data\n");
			printf("\n");
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
		printf("Archive Error :\n");
		printf("<PRE>\n");
		printf("%s\n", e.what());
		printf("</PRE>\n");
		return false;
	}
	catch (const char *txt)
	{
		printf("Caught Error :\n");
		printf("<PRE>\n");
		printf("%s\n", txt);
		printf("</PRE>\n");
		return false;
	}
	catch (...)
	{
		printf("Caught Unknown Error\n");
		return false;
	}

	return have_data;
}

static void getTimeTxt(char *result)
{
	int year, month, day, hour, min, sec;
	unsigned long nano;
	epicsTime2vals(epicsTime::getCurrent(),
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
        printf("<ul>\n");
        printf("<li>No data<br>\n");
        printf("    There seems to be no data for that channel\n");
        printf("    and time range.\n");
        printf("    Check the <A HREF=\"%s\">", dataURL.c_str());
        printf("    data that was supposed to be plotted.</A>\n");
        printf("<li>Internal GNUPlot error<br>\n");
        printf("    If you can't get <i>any</i> plot today\n");
        printf("    even though the data is available as\n");
        printf("    a spreadsheet, the plotting is broken.\n");
        printf("</ul>\n");
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
            printf("The execution of GNUPlot plot failed:<P>\n");
            printf("<PRE>'%s %s'</PRE><P>\n",
                   GNUPLOT_PROGRAM, GNUPlotCommand);
            printf("Result: %d<P>\n", result);
            return 0;
        }
    }
#endif
    
    printf("<A HREF=\"%s\">", dataURL.c_str());
    printf("<IMG SRC=\"%s\" ALT=\"Click Image to see raw data\"</A></A><P>\n", imageURL.c_str());
    printf("<HR>\n");
    page.interFace();
    
    return 0;
}

// Command == DEBUG
void cmdDebug(HTMLPage &page, const CGIInput &cgi, const char *envp[])
{
    page._title = "DEBUG Information";
    page.start();
    page.header("DEBUG Information",1);
    
    printf("<I>CGIExport version " VERSION_TXT ", "
           EPICS_VERSION_STRING
           ", built " __DATE__ ", " __TIME__ "</I>\n");
    
    page.header("Variables parsed from CGI input",2);
    const stdMap<stdString, stdString> &var_map = cgi.getVars ();
    stdMap<stdString, stdString>::const_iterator vars = var_map.begin();
    while (vars != var_map.end())
    {
        printf("'%s' = '%s'<BR>\n", vars->first.c_str(), vars->second.c_str());
        ++vars;
    }
    
    char dir[100];
    getcwd(dir, sizeof dir);
    page.header("Script Info", 2);

    page.header("Current directory:", 3);
    printf("<PRE>%s</PRE><P>\n", dir);

    page.header("Script:", 3);
    printf("<PRE>%s</PRE><P>\n", script.c_str());
    
    page.header("Script path:", 3);
    printf("<PRE>%s</PRE><P>\n", script_dir.c_str());
    
    page.header("GNUPlot Program:", 3);
    printf("<PRE>%s</PRE><P>\n", GNUPLOT_PROGRAM);

    page.header("GNUPlot Pipe:", 3);
    printf("<PRE>%s</PRE><P>\n", GNUPLOT_PIPE);

    stdString data, dataURL, imageURL;
    getFilenames(data, dataURL, imageURL);

    page.header("Data:", 3);
    printf("<PRE>%s</PRE><P>\n", data.c_str());

    page.header("Data URL:", 3);
    printf("<PRE>%s</PRE><P>\n", dataURL.c_str());
    
    page.header("Image URL:", 3);
    printf("<PRE>%s</PRE><P>\n", imageURL.c_str());
    
#ifndef WIN32
    page.header("Personality:", 3);
    printf("UID: %d/%d, GID: %d/%d<P>\n", getuid(), geteuid(), getgid(), getegid());
#endif
    showEnvironment(page, envp);
    
    printf("<HR>\n");
    page.interFace();
}

// For MsgLogger
static void PrintRoutine(void *arg, const char *text)
{   fputs(text, stderr); }

int main(int argc, const char *argv[], const char *envp[])
{
    initEpicsTimeHelper();
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
	parser.setHeader("This command is meant to be executed via CGI\n"
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
		if (!cgi.parse(stdin, stdout))
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
	page._y0 = atof(cgi.find ("Y0").c_str());
	page._y1 = atof(cgi.find ("Y1").c_str());
    page._use_logscale = cgi.find("LOGY").length()>0;
	getNames(cgi.find("NAMES"), page._names);
	if (! decodeTimes(cgi, page._start, page._end))
	{
		page._title = "Time Error";
		page.start();
		printf("Cannot decode times.\n");
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
        if (page._format == "MLSHEET")
            format = fmt_MatlabSheet;
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
