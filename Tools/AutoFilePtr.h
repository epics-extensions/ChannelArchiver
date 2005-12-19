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
    /// Construct AutoFilePtr for given filename and mode.
    AutoFilePtr(const char *filename, const char *mode)
    {
        f = fopen(filename, mode);
    }

    /// Construct AutoFilePtr for existing FILE,
    /// which is now controlled by the AutoFilePtr.
    AutoFilePtr(FILE *f = 0) : f(f) {}

    /// Destructor closes the FILE under control of this AutoFilePtr.
    ~AutoFilePtr()
    {
        set(0);
    }

    /// Release control of the current file, closing it,
    /// and switch to a new file.
    void set(FILE *new_f)
    {
        if (f)
            fclose(f);
        f = new_f;
    }

    /// Is there an open file?
    operator bool () const
    {
        return f != 0;
    }

    /// Obtain the current FILE.
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
