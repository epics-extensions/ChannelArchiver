// -*- c++ -*-
#if !defined(_TRACER_H_)
#define _TRACER_H_

// System
#include <stdlib.h>
// Base
#include <epicsMutex.h>

/// \ingroup Tools
/// The MsgLogger is a trace or logging helper.
///
/// Note that it is not necessary to instantiate
/// another logger. Instead, one can use the following
/// printf and assert-like macros that call a predefined,
/// global MsgLogger:
/// \code
///  LOG_MSG("in init()\n");
///  LOG_MSG("Value of i is %d\n", i);
///  LOG_ASSERT(i > 2);
/// \endcode
class MsgLogger
{
public:
    /// Constructor
    MsgLogger();

    /// Type for user-defined output routine.
    typedef void (*PrintRoutine) (void *arg, const char *text);
    
    /// Override default output routine.
    void SetPrintRoutine(PrintRoutine print, void *arg = 0)
    {
        this->print = print;
        this->print_arg = arg;
    }

    /// Restore default output routine
    void SetDefaultPrintRoutine();

    /// Log some text.
    void Print(const char *s);

    epicsMutex   lock;

private:
    PrintRoutine print;
    void         *print_arg;
};

// The gloablly available MsgLogger instance:
extern MsgLogger TheMsgLogger;

void LOG_MSG(const char *format, ...);

#ifdef CMLOG
# define LOG_ASSERT(e)                                              \
    if (! (e))                                                      \
    {                                                               \
        if (log_cmlog)                                              \
	        cmlog_assert( #e, __FILE__, __LINE__ );             \
        LOG_MSG("\nASSERT '%s' FAILED:\n%s (%d)\n\n",               \
                #e, __FILE__, __LINE__);                            \
        exit(42);                                                   \
    }
#else
# define LOG_ASSERT(e)                                              \
    if (! (e))                                                      \
    {                                                               \
        LOG_MSG("\nASSERT '%s' FAILED:\n%s (%d)\n\n",               \
                #e, __FILE__, __LINE__);                            \
        exit(42);                                                   \
    }
#endif

#endif // !defined(_TRACER_H_)
