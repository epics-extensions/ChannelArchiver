// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include <stdarg.h>
#include <ToolsConfig.h>
#include <epicsTimeHelper.h>
#include "../ArchiverConfig.h"
#include "NetTools.h"
#include "HTMLPage.h"

bool HTMLPage::_nocfg = false;

HTMLPage::HTMLPage(SOCKET socket, const char *title, int refresh)
{
	_socket = socket;
	_refresh= refresh;

	line("HTTP/1.0 200 OK");
	line("Server: ArchiveEngine");
	line("Content-type: text/html");
	line("");

	line("<HTML>");
	if (refresh > 0)
	{
        char linebuf[60];
        sprintf(linebuf, "<META HTTP-EQUIV=\"Refresh\" CONTENT=%d>",
                refresh);
		line(linebuf);
	}

	line("");
	line("<HEAD>");
	out ("<TITLE>");
	out(title);
	line("</TITLE>");
	line("</HEAD>");
	line("");
	line("<BODY BGCOLOR=#AEC9D2 LINK=#0000FF VLINK=#0000FF ALINK=#0000FF>");
	line("<FONT FACE=\"Arial, Comic Sans MS, Helvetica\">");
	line("<BLOCKQUOTE>");

	line("<TABLE BORDER=3>");
	line("<TR><TD BGCOLOR=#FFFFFF><FONT SIZE=5>");
	line(title);
	line("</FONT></TD></TR>");
	line("</TABLE>");
	line("");
	line("<P>");
	line("");
}

HTMLPage::~HTMLPage()
{
	line("<P><HR WIDTH=50% ALIGN=LEFT>");

	line("<A HREF=\"/\">-Main-</A>  ");
	line("<A HREF=\"/groups\">-Groups-</A>  ");
	if (!_nocfg)
        line("<A HREF=\"/config\">-Config.-</A>  ");
	line("<BR>");
    
    char linebuf[100];
	if (_refresh > 0)
	{
		sprintf(linebuf, "This page will update every %d seconds...",
                _refresh);
		line(linebuf);
	}
	else
    {
        int year, month, day, hour, min, sec;
        unsigned long nano;
        epicsTime2vals(epicsTime::getCurrent(),
                       year, month, day, hour, min, sec, nano);
		sprintf(linebuf,
                "(Status for %02d/%02d/%04d %02d:%02d:%02d. "
                "Use <I>Reload</I> from the Browser's menu for updates)",
                month, day, year, hour, min, sec);
		line(linebuf);
    }
	line("</BLOCKQUOTE>");
	line("</FONT>");
	line("</BODY>");
	line("</HTML>");
}

void HTMLPage::line(const char *line)
{
	out(line);
	out("\r\n", 2);
}

void HTMLPage::line(const stdString &line)
{
	out(line);
	out("\r\n", 2);
}

// Last column name must be 0
void HTMLPage::openTable(size_t colspan, const char *column, ...)
{
	va_list	ap;
	const char *name = column;

	va_start(ap, column);
	line("<TABLE BORDER=0 CELLPADDING=5>");
	out("<TR>");
	while (name)
	{
		if (colspan > 1)
		{
			char buf[80];
			sprintf(buf, "<TH COLSPAN=%d BGCOLOR=#000000><FONT COLOR=#FFFFFF>",
                    colspan);
			out(buf);
		}
		else
			out("<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>");
		out(name);
		out("</FONT></TH>");

		colspan = va_arg(ap, size_t);
		if (!colspan)
			break;
		name = va_arg(ap, const char *);
	}
	line("</TR>");
	va_end(ap);
}

void HTMLPage::tableLine(const char *item, ...)
{
	va_list	ap;
	const char *name = item;
	bool first = true;

	va_start(ap, item);
	out("<TR>");
	while (name)
	{
		if (first)
			out("<TH BGCOLOR=#FFFFFF>");
		else
			out("<TD ALIGN=CENTER>");
		out(name);
		if (first)
		{
			out("</TH>");
			first = false;
		}
		else
			out("</TD>");
		name = va_arg(ap, const char *);
	}

	line("</TR>");
	va_end(ap);
}

void HTMLPage::closeTable()
{
	line("</TABLE>");
}
