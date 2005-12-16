// -*- c++ -*-

#ifndef __AUTO_FILE_PRT_H__
#define __AUTO_FILE_PRT_H__

// System
#include <stdio.h>

/// \ingroup Tools

/// Auto-close FILE pointer wrapper.
class AutoFilePtr
{
public:
    AutoFilePtr(const char *filename, const char *mode)
    {
        f = fopen(filename, mode);
    }

    AutoFilePtr(FILE *f) : f(f) {}
    
    ~AutoFilePtr()
    {
        if (f)
            fclose(f);
    }

    operator bool () const
    {
        return f != 0;
    }
    
    operator FILE * () const
    {
        return f;
    }
private:
    FILE *f;

    // Not implemented, don't use these
    AutoFilePtr();
    AutoFilePtr & operator = (const AutoFilePtr &rhs);
};

#endif
