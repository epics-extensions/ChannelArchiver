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
                  epicsThreadPriorityMedium),
          _signal(epicsEventEmpty) // initially taken
    {
        _go = true;
        _writing = false;
    }

    void start()
    {
        _thread.start();
    }

    bool isRunning() const
    {   return _go; }

    // request this thread to exit ASAP
    void stop()
    {
        _go = false;
        _signal.signal();
        _thread.exitWait();
    }

    void write()
    {
        if (_writing)
        {
            stdString t;
            LOG_MSG("Warning: WriteThread called while busy\n");
            return;
        }
        _signal.signal();
    }

    virtual void run();

private:
    epicsThread  _thread;
    epicsEvent   _signal;
    mutable bool _go;
    mutable bool _writing;
};

#endif //__WRITETHREAD_H__
