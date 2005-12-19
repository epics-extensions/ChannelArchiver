// -*- c++ -*-
#if !defined(__MSG_LOGGER_H__)
#define __MSG_LOGGER_H__

// System
#include <stdio.h>
// Tools
#include <GenericException.h>
#include <AutoFilePtr.h>

/// \ingroup Tools
/// The MsgLogger is a trace or logging facility.
///
/// The following printf and assert-like macros
/// routines and macros will call TheMsgLogger->log:
///
/// \code
///  LOG_MSG("in init()\n");
///  LOG_MSG("Value of i is %d\n", i);
///  LOG_ASSERT(i > 2);
/// \endcode
///
/// If no logger is created by the application,
/// LOG_... will create a default logger for stderr.
class MsgLogger
{
public:
    /// Construct a new logger.
    ///
    /// This logger replaces the existing logger,
    /// in case there is one.
    /// If no filename (null or empty string)
    /// is supplied, stderr is used.
    ///
    /// @exception GenericException if file fails to open.
    MsgLogger(const char *filename = 0);

    /// Destructor.
    ///
    /// Will restore whatever previous logger was
    /// in place.
    virtual ~MsgLogger();

    /// Log some text.
    ///
    /// Prepends info with time stamp,
    /// then invokes print.
    void log(const char *format, va_list ap);

protected:
    /// The current MsgLogger.
    static MsgLogger *TheMsgLogger;

    /// Each MsgLogger keeps track of the previous
    /// logger so that it can be restored when
    /// this logger is closed.
    MsgLogger *prev_logger;

    friend void ::LOG_MSG(const char *format, ...);

    /// The file used by print in this logger.
    AutoFilePtr f;

    /// Derived classes can override this to redirect the
    /// messages, and then point TheMsgLogger to the
    /// custom MsgLogger.
    virtual void print(const char *s);
};

void LOG_MSG(const char *format, ...);

#define LOG_ASSERT(e)                                                \
    if (! (e))                                                       \
    {                                                                \
        LOG_MSG("\nASSERT '%s' FAILED:\n%s (%d)\n\n",                \
                #e, __FILE__, __LINE__);                             \
        throw GenericException(__FILE__, __LINE__,                   \
                               "ASSERT '%s' FAILED", #e);            \
    }

#endif

