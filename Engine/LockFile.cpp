// LockFile.cpp: implementation of the LockFile class.
//////////////////////////////////////////////////////////////////////

#include "LockFile.h"
#include <stdio.h>
#include <strstream>
#include <fstream>
#include "osiTimeHelper.h"

// Try to generate lock file.
// Result: succesful?
bool Lockfile::Lock (const stdString &prog_name)
{
	// Check for existing file
    std::ifstream ifile;
	ifile.open (_filename.c_str());
#	ifdef __HP_aCC
	if (! ifile.fail())
#	else
	if (ifile.is_open ())
#	endif
	{
		char line[80];
		ifile.getline (line, sizeof (line));

        std::cerr << "Found an existing lock file '" << _filename << "':\n";
		std::cerr << line << "\n";
		ifile.close ();

		return false;
	}

    std::ofstream ofile;
	ofile.open (_filename.c_str());
#	ifdef __HP_aCC
	if (ofile.fail())
#	else
	if (! ofile.is_open ())
#	endif
	{
		std::cerr << "cannot open lock file '" << _filename << "'\n";
		return false;
	}
	osiTime now;
	now = osiTime::getCurrent ();
	ofile << prog_name << " started on " << now << "\n";
	ofile << "\n";
	ofile << "If you can read this, the program is still running or exited ungracefully...\n";
	ofile.close ();

	return true;
}

// Remove lock file
void Lockfile::Unlock ()
{
	remove (_filename.c_str());
}



