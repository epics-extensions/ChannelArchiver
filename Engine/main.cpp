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

// System
#include <signal.h>
// Tools
#include "Filename.h"
#include "epicsVersion.h"
#include "epicsTimeHelper.h"
#include "ArgParser.h"
#include "MsgLogger.h"
// Engine
#include "Engine.h"
#include "EngineConfig.h"
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
    if (logfile)
    {
        fputs(text, logfile);
        fflush(logfile);
    }
    else
    {
        fputs(text, stdout);
        fflush(stdout);
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
    parser.setHeader ("ArchiveEngine Version " ARCH_VERSION_TXT ", "
                      EPICS_VERSION_STRING
                      ", built " __DATE__ ", " __TIME__ "\n\n");
    port.set (EngineServer::_port); // default
    if (!parser.parse()    ||   parser.getArguments ().size() != 2)
    {
        parser.usage ();
        return -1;
    }
    EngineServer::_port = (int)port;
    EngineServer::_nocfg = (bool)nocfg;
    HTMLPage::_nocfg = (bool)nocfg;
    const stdString &config_name = parser.getArgument (0);
    stdString index_name = parser.getArgument (1);
    if (log.get().length() > 0)
    {
        logfile = fopen(log.get().c_str(), "at");
        if (! logfile)
        {
            LOG_MSG("Cannot open logfile '%s'\n", log.get().c_str());
            return -1;
        }
    }
    Lockfile lock_file("archive_active.lck");
    if (! lock_file.Lock (argv[0]))
        return -1;
    LOG_MSG("Starting Engine with configuration file %s, index %s\n",
            config_name.c_str(), index_name.c_str());
    Engine::create(index_name);
    {
        Guard guard(theEngine->mutex);
        if (! description.get().empty())
            theEngine->setDescription(guard, description);
        EngineConfig config;
        run = config.read(guard, theEngine, config_name);
    }
#ifdef ENGINE_DEBUG
    LOG_MSG("ChannelArchiver thread 0x%08X entering main loop\n",
            epicsThreadGetIdSelf());
#endif
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
    // Main loop
    LOG_MSG("\n----------------------------------------------------\n"
            "Engine Running. Stop via http://localhost:%d/stop\n"
            "----------------------------------------------------\n",
            EngineServer::_port);
    while (run)
        theEngine->process();
    // If engine is not shut down properly (ca_task_exit !),
    // the MS VC debugger will freak out
    LOG_MSG ("Shutting down Engine\n");
    theEngine->shutdown ();    
    LOG_MSG ("Removing lockfile.\n");
    lock_file.Unlock ();
    if (logfile)
        fclose(logfile);
    LOG_MSG ("Done.\n");
    return 0;
}
