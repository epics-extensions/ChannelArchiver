#ifndef __HTMLPAGE_H__
#define __HTMLPAGE_H__
#include "ArchiveTypes.h"
#include "osiSock.h"

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

	void indexLink (const char *URL, const char *info);

protected:
	SOCKET _socket;
	int _refresh;
};

inline void HTMLPage::out (const char *line, size_t length)
{	::send (_socket, line, length, 0);	}

inline void HTMLPage::out (const char *line)
{	out (line, strlen(line));	}

inline void HTMLPage::out (const stdString &line)
{	out (line.c_str(), line.length ());	}

#endif //__HTMLPAGE_H__
