#include "HTMLPage.h"
#include <strstream>
#include <stdarg.h>

using namespace std;

HTMLPage::HTMLPage (SOCKET socket, const char *title, int refresh)
{
	_socket = socket;
	_refresh= refresh;

	line ("HTTP/1.0 200 OK");
	line ("Server: ArchiveEngine");
	line ("Content-type: text/html");
	line ("");

	line ("<HTML>");
	if (refresh > 0)
	{
		strstream	linebuf;
		linebuf << "<META HTTP-EQUIV=\"Refresh\" CONTENT=" << refresh << ">" << '\0';
		line (linebuf.str());
		linebuf.freeze (false);
	}

	line ("");
	line ("<HEAD>");
	out  ("<TITLE>");
	out (title);
	line ("</TITLE>");
	line ("</HEAD>");
	line ("");
	line ("<BODY BGCOLOR=#AEC9D2>");
	line ("<FONT FACE=\"Arial, Comic Sans MS, Helvetica\" SIZE=1>");
	line ("<BLOCKQUOTE>");

	line ("<TABLE BORDER=3>");
	line ("<TR><TD BGCOLOR=#FFFFFF><FONT SIZE=4>");
	line (title);
	line ("</FONT></TD></TR>");
	line ("</TABLE>");
	line ("");
	line ("<P>");
	line ("");
}

HTMLPage::~HTMLPage ()
{
	line ("");
	line ("<BR>");
	line ("<FONT SIZE=1>");
	if (_refresh > 0)
	{
		strstream	linebuf;
		linebuf << "This page will update every " << _refresh << " seconds..." << '\0';
		line (linebuf.str());
		linebuf.freeze (false);
	}
	else
		line ("(Use <I>Reload</I> from the Browser's menu for updates)");
	line ("</FONT>");
	line ("</BLOCKQUOTE>");
	line ("</FONT>");
	line ("</BODY>");
	line ("</HTML>");
}

void HTMLPage::line (const char *line)
{
	out (line);
	out ("\r\n", 2);
}

void HTMLPage::line (const stdString &line)
{
	out (line);
	out ("\r\n", 2);
}

// Last column name must be 0
void HTMLPage::openTable (size_t colspan, const char *column, ...)
{
	va_list	ap;
	const char *name = column;

	va_start (ap, column);
	line ("<TABLE BORDER=0 CELLPADDING=5>");
	out ("<TR>");
	while (name)
	{
		if (colspan > 1)
		{
			strstream buf;
			buf << "<TH COLSPAN=" << colspan << " BGCOLOR=#000000><FONT COLOR=#FFFFFF>" << '\0';
			out (buf.str());
		}
		else
			out ("<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>");
		out (name);
		out ("</FONT></TH>");

		colspan = va_arg (ap, size_t);
		if (!colspan)
			break;
		name = va_arg (ap, const char *);
	}

	line ("</TR>");
	va_end (ap);
}

void HTMLPage::tableLine (const char *item, ...)
{
	va_list	ap;
	const char *name = item;
	bool first = true;

	va_start (ap, item);
	out ("<TR>");
	while (name)
	{
		if (first)
			out ("<TH BGCOLOR=#FFFFFF>");
		else
			out ("<TD ALIGN=CENTER>");
		out (name);
		if (first)
		{
			out ("</TH>");
			first = false;
		}
		else
			out ("</TD>");
		name = va_arg (ap, const char *);
	}

	line ("</TR>");
	va_end (ap);
}

void HTMLPage::closeTable ()
{
	line ("</TABLE>");
}

void HTMLPage::indexLink (const char *URL, const char *info)
{
	line ("<P><HR WIDTH=50% ALIGN=LEFT>");
	out ("<A HREF=\"");
	out (URL);
	out ("\">");
	out (info);
	line ("</A>");
}

