#ifndef __WRITETHREAD_H__
#define __WRITETHREAD_H__

#include <Thread.h>

BEGIN_NAMESPACE_TOOLS

// This Thread is started/stopped by the Engine class.
//
// It handles all the Archive writes,
// it doesn't do any ChannelAccess.
class WriteThread : public Thread
{
public:
    WriteThread ()
        : _wait (true) // initially taken
    {
        _go = true;
        _writing = false;
    }

    // request this thread to exit ASAP
    void stop ()
    {
        _go = false;
        _wait.give ();
    }

    bool isRunning () const
    {   return _go; }

    void write (const osiTime &now)
    {
        if (_writing)
        {
            LOG_MSG ("Warning: WriteThread called while busy\n");
            return;
        }
        
        _now = now;
        if (! _wait.give ())
            LOG_MSG ("WriteThread::write: cannot give semaphore\n");
    }

    virtual int run ();

private:
    osiTime         _now;
    ThreadSemaphore _wait;
    bool            _go;
    bool            _writing;
};

END_NAMESPACE_TOOLS

#endif //__WRITETHREAD_H__
