// Filename: Routines for handling file names,
// should support Unix and Win32
// -kuk- 1999

#include "Filename.h"

static const char *current_dir = ".";

void Filename::build(const stdString &dirname, const stdString &basename,
                     stdString &filename)
{
	if (dirname.empty()  ||  dirname == current_dir)
	{
		filename = basename;
		return;
	}
	filename.reserve(dirname.length() + basename.length() + 1);
	filename = dirname;
#ifdef WIN32
	if (dirname.find('\\') != stdString::npos)
		filename += '\\';
	else
#endif	
		filename += '/';
	filename += basename;
}

bool Filename::containsPath(const stdString &filename)
{
    if (filename.find('/') != stdString::npos)
        return true;
#ifdef WIN32
	if (filename.find('\\') != stdString::npos)
		return true;
#endif	
    return false;
}

// Find the directory portion of given filename.
void Filename::getDirname(const stdString &filename, stdString &dirname)
{
    if (filename.empty())
	{
		dirname.assign((const char *)0, 0);
        return;
	} 
    stdString::size_type dir = filename.find_last_of('/');
#ifdef WIN32
    // For WIN32, both '/' and '\\' are possible:
    if (dir == filename.npos)
        dir = filename.find_last_of('\\');
#endif
    if (dir == filename.npos)
        dirname.assign((const char *)0, 0);
 	else
		dirname = filename.substr(0, dir);
}                     

void Filename::getBasename(const stdString &filename, stdString &basename)
{
    if (filename.empty())
	{
		basename.assign((const char *)0, 0);
		return;
	}
    stdString::size_type base = filename.find_last_of('/');
#ifdef WIN32
    // For WIN32, both '/' and '\\' are possible:
    if (base == filename.npos)
        base = filename.find_last_of('\\');
#endif
    if (base != filename.npos)
    {
		basename = filename.substr(base+1);
		return;
    }
    basename = filename;
}
                            
