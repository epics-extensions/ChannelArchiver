// -*- c++ -*-
#if !defined(_TRACER_H_)
#define _TRACER_H_

#include<GenericException.h>
#include<epicsMutex.h>

//CLASS MsgLogger
// Tracer class:
//
// System-independent logging tool, e.g.
// <PRE>
//  LOG_MSG("in init()\n");
//  LOG_MSG("Value of i is %d\n", i);
//  LOG_ASSERT(i > 2);
// </PRE>
//
// LOG_... calls will print to
// <UL>
// <LI> Debug Window        (WIN32)
// <LI> clog                (other architecture)
// <LI> user defined target (if set)
// </UL>
class MsgLogger
{
public:
    MsgLogger();

    typedef void (*PrintRoutine) (void *arg, const char *text);
    
    // Call to override default output routine
    void SetPrintRoutine(PrintRoutine print, void *arg = 0)
    {
        _print = print;
        _print_arg = arg;
    }

    // Restore default output routine
    void SetDefaultPrintRoutine();

    void Print(const char *s);

    void Lock()
    {    _lock.lock(); }

    void Unlock()
    {    _lock.unlock(); }

private:
    PrintRoutine _print;
    void         *_print_arg;
    epicsMutex   _lock;
};

// The gloablly available MsgLogger instance:
extern MsgLogger TheMsgLogger;

void LOG_MSG(const char *format, ...);

#ifdef CMLOG
# define LOG_ASSERT(e)                                              \
    if (! (e))                                                      \
    {                                                               \
        if (log_cmlog)                                              \
	        cmlog_assert( #e, __FILE__, __LINE__ );                 \
        LOG_MSG("\nASSERT '%s' FAILED:\n%s (%d)\n\n",               \
                #e, __FILE__, __LINE__);                            \
        throw GenericException (__FILE__,__LINE__);                 \
    }
#else
# define LOG_ASSERT(e)                                              \
    if (! (e))                                                      \
    {                                                               \
        LOG_MSG("\nASSERT '%s' FAILED:\n%s (%d)\n\n",               \
                #e, __FILE__, __LINE__);                            \
        throw GenericException (__FILE__,__LINE__);                 \
    }
#endif

#endif // !defined(_TRACER_H_)
