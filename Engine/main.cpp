// main.cpp

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include <fstream>
#include <signal.h>
#include "Engine.h"
#include "ConfigFile.h"
#include "ArgParser.h"
#include "LockFile.h"
#include "EngineServer.h"
#include <Filename.h>

// wrongly defined for hp700 dce:
#undef open
#undef close

USING_NAMESPACE_CHANARCH
using namespace std;

// For communication sigint_handler -> main loop
bool run = true;

// signals are process-, not thread-bound,
// but this handler is meant to react only
// in the main thread.
static void signal_handler (int sig)
{
	signal (sig, SIG_IGN);
	run = false;
	LOG_MSG ("Exiting on signal " << sig << ", please be patient!\n");
}

static ofstream	*logfile = 0;

static void LoggerPrintRoutine (void *arg, const stdString &text)
{
	cout << text;
	if (logfile)
	{
		*logfile << text;
		logfile->flush();
	}
}

#if JustToHaveANiceSymbolToJumpToForSMVisualStudio
class MAIN {};
#endif

int main (int argc, const char *argv[])
{
	TheMsgLogger.SetPrintRoutine (LoggerPrintRoutine);

	initOsiHelpers ();

	// Handle arguments
	stdString prog_name;
	Filename::getBasename (argv[0], prog_name);

	ArgParser args;
	if (!args.parse (argc, argv, "", "pdl")  ||
		args.getArguments ().size() < 1)
	{
		cerr << "Usage: " << prog_name << " [flags] <configuration> { <directory file> }\n";
		cerr << "\tFlags:\n";
		cerr << "\t\t-p <port>     : WWW server (P)ort\n";
		cerr << "\t\t-d <text>     : set engine's (D)escription (for HTTP display)\n";
		cerr << "\t\t-l <filename> : write (L)ogfile\n";
		cerr << "\n";
		return -1;
	}
	
	if (args.getParameter (2).length() > 0)
	{
		logfile = new ofstream;
		logfile->open (args.getParameter (2).c_str (), ios::out | ios::trunc);
		if (! logfile->is_open())
		{
			cerr << "Cannot open logfile '" << args.getParameter (2) << "'\n";
			delete logfile;
			logfile = 0;
		}
	}

	short port = atoi (args.getParameter (0).c_str());
	if (port > 0)
		EngineServer::_port = port;
	
	// Arg. 1 might be the directory_name to use:
	const stdString &config_file = args.getArgument (0);
	stdString directory_name;
	if (args.getArguments ().size() < 2)
		directory_name = "freq_directory";
	else
		directory_name = args.getArgument (1);

	Lockfile lock_file("archive_active.lck");
	if (! lock_file.Lock (prog_name))
		return -1;

	ConfigFile *config = new ConfigFile;
	try
	{
		Engine::create (directory_name);
		if (! args.getParameter (1).empty())
			theEngine->setDescription (args.getParameter (1));
		run = config->load (config_file);
		if (run)
			config->save ();
	}
	catch (GenericException &e)
	{
		cerr << "Cannot start archive engine:\n";
		cerr << e.what ();
		return -1;
	}

	try
	{
		// Main loop
		theEngine->setConfiguration (config);
		signal (SIGINT, signal_handler);
		signal (SIGTERM, signal_handler);

		cerr << "\n------------------------------------------\n";
		cerr << "Engine Running.\n";
		cerr << "Started " << osiTime::getCurrent() << "\n";
		cerr << "Stop via web browser at http://localhost:" << EngineServer::_port << "/stop\n";
		cerr << "------------------------------------------\n";

		while (run  &&  theEngine->process ())
		{}
	}
	catch (GenericException &e)
	{
		cerr << "Exception caugh in main loop:\n";
		cerr << e.what ();
	}

	// If engine is not shut down properly (ca_task_exit !),
	// the MS VC debugger will freak out
	LOG_MSG ("Shutting down Engine\n");
	theEngine->shutdown ();
	LOG_MSG ("Done\n");
	delete config;
	lock_file.Unlock ();
	if (logfile)
	{
		logfile->close ();
		delete logfile;
		logfile = 0;
	}

	return 0;
}
