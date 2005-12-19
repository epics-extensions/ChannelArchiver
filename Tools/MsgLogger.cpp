// System
#include <stdarg.h>
#include <stdio.h>
// Tools
#include "MsgLogger.h"
#include "GenericException.h"
#include "epicsTimeHelper.h"

// The global tracer object:
MsgLogger *MsgLogger::TheMsgLogger = 0;

MsgLogger::MsgLogger(const char *filename)
        : prev_logger(TheMsgLogger), f(0)
{
    if (filename)
    {
        f.set(fopen(filename, "w"));
        if (! f)
            throw GenericException(__FILE__, __LINE__,
                                   "Cannot open '%s' for writing",
                                   filename);
    }
    // Make this logger the current one.
    TheMsgLogger = this;
}

MsgLogger::~MsgLogger()
{
    // Restore previous logger
    TheMsgLogger = prev_logger;
}

void MsgLogger::print(const char *s)
{
    if (f)
        fprintf(f, "%s", s);
    else
        fprintf(stderr, "%s", s);
}

void MsgLogger::log(const char *format, va_list ap)
{
    char buffer[2048];
    stdString s;
    epicsTime2string(epicsTime::getCurrent(), s);
    int chars = vsnprintf(buffer, sizeof(buffer), format, ap);
    if (chars < 1  ||  (size_t)chars > sizeof(buffer))
        print("MsgLogger: Buffer overrun\n");
    print(s.substr(0, 19).c_str());
    print(" ");
    print(buffer);
}

void LOG_MSG(const char *format, ...)
{
    if (MsgLogger::TheMsgLogger == 0)
    {   // Initialize when first used
        try
        {
            MsgLogger::TheMsgLogger = new MsgLogger();
        }
        catch (...)
        {
            MsgLogger::TheMsgLogger = 0;
        }
    }
    if (! MsgLogger::TheMsgLogger)
    {   // Error
        fprintf(stderr, "LOG_MSG: No message logger!\n");
        return;
    }
    va_list ap;
    va_start(ap, format);
    MsgLogger::TheMsgLogger->log(format, ap);
    va_end(ap);
}

// EOF MsgLogger.cpp
