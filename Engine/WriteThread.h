// WriteThread                               -*- c++ -*-
#ifndef __WRITETHREAD_H__
#define __WRITETHREAD_H__

#include <epicsThread.h>
#include <epicsMutex.h>

// This Thread is started/stopped by the Engine class.
//
// It handles all the Archive writes,
// it doesn't do any ChannelAccess.
class WriteThread : public epicsThreadRunable
{
public:
    WriteThread()
        : _thread(*this, "WriteThread",
                  epicsThreadGetStackSize(epicsThreadStackBig),
                  epicsThreadPriorityMedium)
    {
        _go = true;
        _writing = false;
        _wait.lock(); // initially taken
        _thread.start();
    }

    // request this thread to exit ASAP
    void stop()
    {
        _go = false;
        _wait.unlock();
    }

    bool isRunning() const
    {   return _go; }

    void write()
    {
        if (_writing)
        {
            stdString t;
            LOG_MSG("Warning: WriteThread called while busy\n");
            return;
        }
        _wait.unlock();
    }

    virtual void run();

private:
    epicsThread  _thread;
    epicsMutex   _wait;
    mutable bool _go;
    mutable bool _writing;
};

#endif //__WRITETHREAD_H__
