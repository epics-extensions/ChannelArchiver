
#include <signal.h>
#include <MsgLogger.h>
#include <epicsTimeHelper.h>
#include "ArchiveException.h"
#include "WriteThread.h"
#include "Engine.h"

void WriteThread::run()
{
#ifdef HAVE_SIGACTION
    // Block signals
    //
    // There is a handler installed in main(),
    // but we don't want to be interrupted
    // in I/O calls for this thread:
    sigset_t sigs_to_block;
    sigemptyset(&sigs_to_block);
    sigaddset(&sigs_to_block, SIGINT);
    sigaddset(&sigs_to_block, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &sigs_to_block, 0))
        LOG_MSG("WriteThread cannot block signals\n");
#endif

    LOG_MSG("WriteThread started\n");
    while (_go)
    {
        _signal.wait();
        if (_go)
        {
            _writing = true;
            try
            {
                theEngine->writeArchive();
            }
            catch (ArchiveException &e)
            {
                LOG_MSG("WriteThread Cannot write, got\n%s\n", e.what());
                _go = false;
            }
            _writing = false;
        }
    }
    LOG_MSG("WriteThread exiting\n");
}

