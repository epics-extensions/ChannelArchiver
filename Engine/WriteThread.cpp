
#include <signal.h>
#include <MsgLogger.h>
#include <osiTimeHelper.h>
#include "ArchiveException.h"
#include "WriteThread.h"
#include "Engine.h"

int WriteThread::run ()
{
#ifdef HAVE_SIGACTION
    // Block signals
    //
    // There is a handler installed in main(),
    // but we don't want to be interrupted
    // in I/O calls for this thread:
    sigset_t sigs_to_block;
    sigemptyset (&sigs_to_block);
    sigaddset (&sigs_to_block, SIGINT);
    sigaddset (&sigs_to_block, SIGTERM);
    if (pthread_sigmask (SIG_BLOCK, &sigs_to_block, 0))
        LOG_MSG ("WriteThread cannot block signals\n");
#endif

    LOG_MSG ("WriteThread started\n");
    while (_go)
    {
        if (! _wait.take ())
        {
            LOG_MSG ("WriteThread cannot take _wait semaphore,"
                     << "quitting\n");
            _go = false;
        }
        
        if (_go)
        {
            _writing = true;
            try
            {
                theEngine->writeArchive ();
            }
            catch (ArchiveException &e)
            {
                LOG_MSG ("WriteThread Cannot write, got\n\t"
                         << e.what() << "\n");
                _go = false;
            }
            _writing = false;
        }
    }
    return 0;
}



