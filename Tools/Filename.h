// FilenameTool: Routines for handling file names,
// should support Unix and Win32
// -kuk- 1999

#ifndef __FILENAMETOOL_H__
#define __FILENAMETOOL_H__

#include <ToolsConfig.h>

//CLASS Filename
// Filename provides basename/dirname/someday maybe more
// methods for file names.
// 
// On UNIX systems, the filenames are build using
// slashes (/),
// on WIN32 systems both slashes and backslashes (\)
// are allowed for input.
// The generated names are always build using slashes
// since both the UNIX and WIN32 system routines
// can handle those.
//
class Filename
{
public:
	//* Build filename from dir. and basename
	static void build (const stdString &dirname, const stdString &basename,
		stdString &filename);

	//* Get dir. or basename from full pathname
	static void getDirname  (const stdString &filename, stdString &dirname);
	static void getBasename (const stdString &filename, stdString &basename);
};

#endif //__FILENAMETOOL_H__









