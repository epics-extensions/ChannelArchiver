#ifndef THROTTLEDMSGLOGGER_H_
#define THROTTLEDMSGLOGGER_H_

// System
#include <stdarg.h>
#include <stdio.h>
// Tools
#include <Throttle.h>
#include <MsgLogger.h>

/// \ingroup Tools
/// A throttled MsgLogger.
///
/// Messages that arrive slower than seconds_between_messages threshold
/// specifies are simply printed.
/// When more messages arrive within the threshold time,
/// only one indication "More ..." is printed.
class ThrottledMsgLogger : public Throttle
{
public:
    /// Create a throttled logger.
    /// @param name The name of the throttle, used for the suppression message.
    /// @param seconds_between_messages The throttle delay.
    ThrottledMsgLogger(const char *name, double seconds_between_messages)
       : Throttle(seconds_between_messages), name(name), too_many(false)
    {
    }
    
    /// Log a throttled message.
    void LOG_MSG(const char *format, ...)
        __attribute__ ((format (printf, 2, 3)));
private:
    stdString name;
    bool   too_many;
};

#endif /*THROTTLEDMSGLOGGER_H_*/