// LockFile.cpp: implementation of the LockFile class.
//////////////////////////////////////////////////////////////////////

#include "LockFile.h"
#include <stdio.h>
#include "osiTimeHelper.h"
#include "MsgLogger.h"

// Try to generate lock file.
// Result: succesful?
bool Lockfile::Lock (const stdString &prog_name)
{
	// Check for existing file
    FILE *f = fopen(_filename.c_str(), "rt");
	if (f)
	{
		char line[80];
        fgets(line, sizeof (line), f);
        LOG_MSG("Found an existing lock file '%s':\n%s\n",
                _filename.c_str(), line);
		fclose(f);
		return false;
	}

	f = fopen(_filename.c_str(), "wt");
    if (!f)
	{
		LOG_MSG("cannot open lock file '%s'\n", _filename.c_str());
		return false;
	}
    stdString t;
    osiTime2string(osiTime::getCurrent(), t);
    fprintf(f, "%s started on %s\n\n", prog_name.c_str(), t.c_str());
	fprintf(f, "If you can read this, the program is still running "
            "or exited ungracefully...\n");
	fclose(f);

	return true;
}

// Remove lock file
void Lockfile::Unlock ()
{
	remove (_filename.c_str());
}

