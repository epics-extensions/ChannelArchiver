// LockFile.cpp: implementation of the LockFile class.
//////////////////////////////////////////////////////////////////////

#include "LockFile.h"
#include <stdio.h>
#include <strstream>
#include <fstream>
#include "osiTimeHelper.h"

using namespace std;

// Try to generate lock file.
// Result: succesful?
bool Lockfile::Lock (const stdString &prog_name)
{
	// Check for existing file
	ifstream ifile;
	ifile.open (_filename.c_str());
	if (ifile.is_open ())
	{
		char line[80];
		ifile.getline (line, sizeof (line));

		cerr << "Found an existing lock file '" << _filename << "':\n";
		cerr << line << "\n";
		ifile.close ();

		return false;
	}

	ofstream ofile;
	ofile.open (_filename.c_str());
	if (! ofile.is_open ())
	{
		cerr << "cannot open lock file '" << _filename << "'\n";
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



