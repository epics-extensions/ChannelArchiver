// LockFile.h: interface for the LockFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_LOCKFILE_H_)
#define _LOCKFILE_H_

#include <ToolsConfig.h>

//CLASS Lockfile
// Generate a lock file containing the generation time.
// (The name LockFile would be nicer but there is a WIN32 system call of that name)
class Lockfile  
{
public:
	Lockfile (const stdString &filename) : _filename (filename) {}

	// Try to generate lock file.
	// Result: succesful?
	bool Lock (const stdString &prog_name);

	// Remove lock file
	void Unlock ();

private:
	stdString _filename;
};

#endif
