// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __HTMLPAGE_H__
#define __HTMLPAGE_H__
#include "ArchiveTypes.h"
#ifdef solaris
// Hack around clash of struct map in inet headers with std::map
#define map xxxMapxxx
#endif
#include <osiSock.h>
#ifdef solaris
#undef map
#endif

class HTMLPage
{
public:
	HTMLPage (SOCKET socket, const char *title, int refresh=0);

	virtual ~HTMLPage ();

	void line (const char *line);
	void line (const stdString &line);

	void out (const char *line);
	void out (const char *line, size_t length);
	void out (const stdString &line);

	// Last column name must be 0
	void openTable (size_t colspan, const char *column, ...);
	void tableLine (const char *item, ...);
	void closeTable ();

        static bool _nocfg;
protected:
	SOCKET _socket;
	int _refresh;
};

inline void HTMLPage::out (const char *line, size_t length)
{	
	// un-const for compatibility w/ HP_aCC
	::send (_socket, (char *)line, length, 0);
}

inline void HTMLPage::out (const char *line)
{	out (line, strlen(line));	}

inline void HTMLPage::out (const stdString &line)
{	out (line.c_str(), line.length ());	}

#endif //__HTMLPAGE_H__
