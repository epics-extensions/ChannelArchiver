#include <stdarg.h>
#include <stdio.h>
#include "MsgLogger.h"
#include "epicsTimeHelper.h"
#ifdef WIN32
#include <windows.h>
#endif

// The global tracer object:
MsgLogger TheMsgLogger;

MsgLogger::MsgLogger()
{
	SetDefaultPrintRoutine();
}

void MsgLogger::Print(const char *s)
{
	_print(_print_arg, s);
}

#ifdef WIN32
static void DefaultPrintRoutine(void *arg, const char *text)
{
    OutputDebugString(text);
    printf("%s", text);
}
#else
static void DefaultPrintRoutine(void *arg, const char *text)
{
    printf("%s", text);
}
#endif

void MsgLogger::SetDefaultPrintRoutine()
{
	_print = DefaultPrintRoutine;
	_print_arg = 0; // not used
}

static void LOG_MSG_NL(const char *format, va_list ap)
{
    enum { BufferSize = 2048 };
    char buffer[BufferSize];

    stdString s;
    epicsTime2string(epicsTime::getCurrent(), s);
    TheMsgLogger.Print(s.substr(0, 19).c_str());
    TheMsgLogger.Print(" ");
    vsprintf(buffer, format, ap);
    if (strlen(buffer) >= BufferSize)
        TheMsgLogger.Print("LOG_MSG_NL: Buffer overrun\n");
    TheMsgLogger.Print(buffer);
#ifdef CMLOG
    if (log_cmlog)
        cmlog_log(buffer);
#endif
}

void LOG_MSG(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    TheMsgLogger.Lock();
    LOG_MSG_NL(format, ap);
    TheMsgLogger.Unlock();
    va_end(ap);
}

// EOF MsgLogger.cc
