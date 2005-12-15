// System
#include <stdarg.h>
#include <stdio.h>

// Local
#include "ErrorInfo.h"

ErrorInfo::ErrorInfo() : error(false)
{}

void ErrorInfo::set(const char *format, ...)
{
    char buffer[160];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    
    info = buffer;
    error = true;
}
