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

#include <signal.h>
#include <Filename.h>
#include <epicsVersion.h>
#include <epicsTimeHelper.h>
#include <ArgParser.h>
#include "Engine.h"
#include "ConfigFile.h"
#include "LockFile.h"
#include "EngineServer.h"
#include "HTMLPage.h"

#ifdef SAMPLE_TEST
#include "ArchiveChannel.h"
#endif

// For communication sigint_handler -> main loop
bool run = true;

// signals are process-, not thread-bound.
static void signal_handler(int sig)
{
#ifndef HAVE_SIGACTION
    signal(sig, SIG_IGN);
#endif
    run = false;
}

FILE *logfile = 0;

static void LoggerPrintRoutine(void *arg, const char *text)
{
    fputs(text, stdout);
    if (logfile)
    {
        fputs(text, logfile);
        fflush(logfile);
    }
}

int main(int argc, const char *argv[])
{
    initEpicsTimeHelper();

    TheMsgLogger.SetPrintRoutine(LoggerPrintRoutine);

    CmdArgParser parser (argc, argv);
    parser.setArgumentsInfo  ("<config-file> <index-file>");
    CmdArgInt port           (parser, "port", "<port>",
                              "WWW server's TCP port (default 4812)");
    CmdArgString description (parser, "description", "<text>",
                              "description for HTTP display");
    CmdArgString log         (parser, "log", "<filename>", "write logfile");
    CmdArgFlag   nocfg       (parser, "nocfg", "disable online configuration");
    parser.setHeader ("ArchiveEngine Version " VERSION_TXT ", "
                      EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n");
    port.set (EngineServer::_port); // default

    if (!parser.parse() ||
        parser.getArguments ().size() != 2)
    {
        parser.usage ();
        return -1;
    }
    
    if (log.get().length() > 0)
    {
        logfile = fopen(log.get().c_str(), "wt");
        if (! logfile)
        {
            LOG_MSG("Cannot open logfile '%s'\n", log.get().c_str());
        }
    }

    EngineServer::_port = (int)port;
    EngineServer::_nocfg = (bool)nocfg;
    HTMLPage::_nocfg = (bool)nocfg;
    
    const stdString &config_name = parser.getArgument (0);
    stdString index_name = parser.getArgument (1);

    LOG_MSG("Starting Engine with configuration file %s, index %s\n",
            config_name.c_str(), index_name.c_str());

    Lockfile lock_file("archive_active.lck");
    if (! lock_file.Lock (argv[0]))
        return -1;

    ConfigFile *config = new ConfigFile;
    try
    {
        Engine::create(index_name);
        if (! description.get().empty())
            theEngine->setDescription(description);
        run = config->load(config_name);
        if (run)
            config->save();
    }
    catch (GenericException &e)
    {
        LOG_MSG("Cannot start archive engine:%s\n", e.what());
        return -1;
    }

#ifdef ENGINE_DEBUG
    LOG_MSG("ChannelArchiver thread 0x%08X entering main loop\n",
            epicsThreadGetIdSelf());
#endif
    try
    {
        // Main loop
        theEngine->setConfiguration(config);
#ifdef HAVE_SIGACTION
        struct sigaction action;
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_handler = signal_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        if (sigaction(SIGINT, &action, 0) ||
            sigaction(SIGTERM, &action, 0))
            LOG_MSG("Error setting signal handler\n");
#else
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
#endif

        LOG_MSG("\n------------------------------------------\n"
                "Engine Running.\n"
                "Stop via web browser at http://localhost:%d/stop\n"
                "------------------------------------------\n",
                EngineServer::_port);
#ifdef SAMPLE_TEST
        ArchiveChannel *ac = new ArchiveChannel("fred", 5.0, new SampleMechanismMonitored());

        ac->startCA();

#endif

        while (run)
        {
            theEngine->process();
        }
    }
    catch (GenericException &e)
    {
        LOG_MSG("Exception caugh in main loop:\n%s\n", e.what());
    }

    // If engine is not shut down properly (ca_task_exit !),
    // the MS VC debugger will freak out
    LOG_MSG ("Shutting down Engine\n");
    theEngine->shutdown ();

    epicsThreadSleep(1.0);
    
    LOG_MSG ("Removing lockfile.\n");
    delete config;
    lock_file.Unlock ();
    if (logfile)
        fclose(logfile);
    LOG_MSG ("Done.\n");

    return 0;
}
