
#if !defined(_ERRORINFO_H_)
#define _ERRORINFO_H_

#include <ToolsConfig.h>

class ErrorInfo
{
public:
    ErrorInfo();

    /// Set 'true' if an error occurred.
    bool error;

    /// String that describes the error
    stdString info;

    /// printf-type way of setting error and info.
    void set(const char *format, ...);
};

#endif // !defined(_ERRORINFO_H_)
