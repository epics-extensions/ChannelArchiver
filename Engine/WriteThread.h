#ifndef __WRITETHREAD_H__
#define __WRITETHREAD_H__

#include <Thread.h>

// This Thread is started/stopped by the Engine class.
//
// It handles all the Archive writes,
// it doesn't do any ChannelAccess.
class WriteThread : public GenericThread
{
public:
    WriteThread()
        : _wait(true) // initially taken
    {
        _go = true;
        _writing = false;
    }

    // request this thread to exit ASAP
    void stop()
    {
        _go = false;
        _wait.give();
    }

    bool isRunning() const
    {   return _go; }

    void write()
    {
        if (_writing)
        {
            LOG_MSG("Warning: WriteThread called while busy\n");
            return;
        }
        if (! _wait.give())
            LOG_MSG("WriteThread::write: cannot give semaphore\n");
    }

    virtual int run();

private:
    ThreadSemaphore _wait;
    bool            _go;
    bool            _writing;
};

#endif //__WRITETHREAD_H__
