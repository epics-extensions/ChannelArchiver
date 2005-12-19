// LockFile.cpp: implementation of the LockFile class.

// Tools
#include "Lockfile.h"
#include "AutoFilePtr.h"
#include "epicsTimeHelper.h"
#include "GenericException.h"

// Try to generate lock file.
// Result: succesful?
void Lockfile::Lock(const stdString &prog_name)
{
    // Check for existing file
    {
        AutoFilePtr f(filename.c_str(), "rt");
        if (f)
        {
            char line[80];
            line[0] = '\0';
            fgets(line, sizeof (line), f);
            throw GenericException(__FILE__, __LINE__,
                                   "Found an existing lock file '%s':\n%s\n",
                                   filename.c_str(), line);
        }
    }
    AutoFilePtr f(filename.c_str(), "wt");
    if (!f)
    {
        throw GenericException(__FILE__, __LINE__,
                               "cannot open lock file '%s'\n", filename.c_str());
    }
    stdString t;
    epicsTime2string(epicsTime::getCurrent(), t);
    fprintf(f, "%s started on %s\n\n", prog_name.c_str(), t.c_str());
    fprintf(f, "If you can read this, the program is still running "
            "or exited ungracefully...\n");
}

void Lockfile::Unlock ()
{
    remove(filename.c_str());
}

