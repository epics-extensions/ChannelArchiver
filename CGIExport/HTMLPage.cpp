// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "HTMLPage.h"

static const char *start_file = "cgi_body_start.txt";
static const char *end_file = "cgi_body_end.txt";

HTMLPage::HTMLPage()
{
    _started = false;
    _glob = true;
    _fill = true;
    _status = false;
    _reduce = true;
    _use_logscale = false;
    _interpol = 0;
}

void HTMLPage::start()
{
    printf("Content-type: text/html\n"
           "Pragma: no-cache\n"
           "\n"
           "<HTML>\n"
           "<META Creator: EPICS ChannelArchiver CGIExport>\n"
           "<HEAD>\n"
           "<TITLE>%s</TITLE>\n", _title.c_str());
    FILE *f = fopen(start_file, "rt");
    if (f)
    {
        char line[200];
        while (fgets(line, sizeof(line), f))
            fputs(line, stdout);
    }
    else
    {
        printf("\n<! default header --------->\n"
               "<BODY bgcolor=\"#FFFFFF\">\n"
               "<FONT face=\"Comic Sans MS, Arial, Helvetica\">\n"
               "<BLOCKQUOTE>\n"
               "<! create '%s' to replace this default header ---->\n\n",
               start_file);
    }
    _started = true;
}

static void makeSelect(const char *name, int start, int end, int select)
{
    printf("           <SELECT NAME=%s SIZE=1>\n", name);
    for (int i=start; i<=end; ++i)
    {
        printf("<OPTION");
        if (i==select)
            printf(" SELECTED");
        printf("> ");
        if (i<10)
            printf("0");
        printf("%d</OPTION> ", i);
    }
    printf("\n           </SELECT>\n");
}

// Print the interface stuff
// (funny name because interface was a reserved word in some compiler)
void HTMLPage::interFace() const
{
    size_t i;
    int year, month, day, hour, min, sec;
    unsigned long nano;

    printf("<SCRIPT LANGUAGE=\"JavaScript\">\n");
    printf("function clrNames () { document.f.NAMES.value=\"\"; }\n");
    printf("</SCRIPT>\n");

    printf("<FORM METHOD=\"GET\" ACTION=\"%s\" NAME=\"f\">\n", _cgi_path.c_str());
    printf("  <INPUT TYPE=\"HIDDEN\" NAME=\"DIRECTORY\" VALUE=\"%s\">\n", _directory.c_str());
    // Table: 5 columns!
    // Start/end/Get | Format | Config. description | config input | Clear/Empty
    printf("  <TABLE cellpadding=1>\n");
    printf("  <TR valign=top>\n");
    printf("      <TD>Pattern:<br>\n");
    printf("          <INPUT name=GLOB type=checkbox value=ON");
    if (_glob)
        printf(" checked=1");
    printf(           ">file glob\n");
    printf("      </TD>\n");
    printf("      <TD colspan=4><INPUT NAME=\"PATTERN\" VALUE=\"%s\" SIZE=40 MAXLENGTH=100>\n",
           _pattern.c_str());
    printf("          <INPUT TYPE=submit NAME=COMMAND VALUE=LIST>\n");
    printf("          <INPUT TYPE=submit NAME=COMMAND VALUE=INFO>\n");
    printf("      </TD>\n");
    printf("  </TR>\n");
    printf("  <TR>\n");
    printf("      <TD valign=\"top\">Names:</TD>\n");
    printf("      <TD colspan=4><TEXTAREA NAME=NAMES ROWS=5 COLS=40>");
    for (i=0; i<_names.size(); ++i)
        printf("%s\n", _names[i].c_str());
    printf("</TEXTAREA>\n");
    printf("<input type=button value=\"CLEAR\" onClick=\"clrNames()\">\n");
    printf("      </TD>\n");
    printf("  </TR>\n");

    epicsTime2vals(_start, year, month, day, hour, min, sec, nano);
    printf("  <TR>\n");
    printf("      <TD>Start:</TD><TD colspan=4>Day (m/d/y)\n");
    makeSelect("STARTMONTH",    1,   12, month);
    makeSelect("STARTDAY"  ,    1,   31, day);
    makeSelect("STARTYEAR" , 1998, 2005, year);
    printf("Time (h:m:s)");
    makeSelect("STARTHOUR" ,    0,   23, hour);
    makeSelect("STARTMINUTE" ,  0,   59, min);
    makeSelect("STARTSECOND" ,  0,   59, sec);
    printf("      </TD>\n");
    printf("  </TR>\n");

    epicsTime2vals(_end, year, month, day, hour, min, sec, nano);
    printf("  <TR>\n");
    printf("      <TD>End:</TD><TD colspan=4>Day (m/d/y)\n");
    makeSelect("ENDMONTH",    1,   12, month);
    makeSelect("ENDDAY"  ,    1,   31, day);
    makeSelect("ENDYEAR" , 1998, 2010, year);
    printf("Time (h:m:s)");
    makeSelect("ENDHOUR" ,    0,   23, hour);
    makeSelect("ENDMINUTE" ,  0,   59, min);
    makeSelect("ENDSECOND" ,  0,   59, sec);
    printf("      </TD>\n");
    printf("  </TR>\n");

    printf("  <TR>\n");
    printf("      <TD><input TYPE=submit NAME=COMMAND VALUE=GET></TD>\n");
    printf("      <TD><input type=radio name=FORMAT ");
    if (_format == "PLOT")
        printf("checked=1 ");
    printf(           "value=PLOT>Plot, y limits\n");
    printf("<input maxLength=10 name=Y0 size=3 value=%g>\n", _y0);
    printf("<input maxLength=10 name=Y1 size=3 value=%g>\n", _y1);
    printf("      </TD>\n");
    printf("      <TD align=right>All Data:</TD>\n");
    printf("      <TD><input name=REDUCE type=checkbox value=ON %s>"
           " (default: reduced to plot size)</TD>\n",
           (!_reduce?" checked=1":""));
    printf("      <TD>\n");
    printf("      </TD>\n");
    printf("  </TR>\n");
    printf("  <TR>\n");
    printf("      <TD></TD>\n");
    printf("      <TD><input type=radio name=FORMAT ");
    if (_format.empty() || _format == "SPREADSHEET")
        printf("checked=1 ");
    printf(           "value=SPREADSHEET>Spreadsheet</TD>\n");
    printf("      <TD align=right>Status:</TD>\n");
    printf("      <TD><input name=STATUS type=checkbox value=ON%s>"
           "(show channel status)</TD>\n",
           (_status?" checked=1":""));
    printf("      <TD></TD>\n");
    printf("  </TR>\n");
    printf("  <TR>\n");
    printf("      <TD></TD>\n");
    printf("      <TD><input type=radio name=FORMAT ");
    if (_format == "MLSHEET")
        printf("checked=1 ");
    printf(          "value=MLSHEET>Matlab-Spreadsheet</TD>\n");
    printf("      <TD align=right>Fill:</TD>\n");
    printf("      <TD><input name=FILL type=checkbox value=ON%s>"
           " (step-func. interpolation)</TD>\n",
           (_fill?" checked=1":""));
    printf("      <TD></TD>\n");
    printf("  </TR>\n");
    printf("  <TR>\n");
    printf("      <TD></TD>\n");
    printf("      <TD><input type=radio name=FORMAT ");
    if (_format == "MATLAB")
        printf("checked=1 ");
    printf(          "value=MATLAB>Matlab</TD>\n");
    printf("      <TD align=right>Interpolate:</TD>\n");
    printf("      <TD><input maxLength=10 name=INTERPOL size=5 value=%g> secs (linear)</TD>\n",
           _interpol);
    printf("      <TD></TD>\n");
    printf("      <TD></TD>\n");
    printf("  </TR>\n");
    printf("  <TR>\n");
    printf("      <TD></TD>\n");
    printf("      <TD></TD>\n");
    printf("      <TD align=right>Log Scale:</TD>\n");
    printf("      <TD><input name=LOGY type=checkbox value=ON%s></TD>\n",
           (_use_logscale ?" checked=1":""));
    printf("      <TD></TD>\n");
    printf("  </TR>\n");
    printf("  </TABLE>\n");
    printf("</FORM>\n");
}

HTMLPage::~HTMLPage()
{
    if (_started)
    {
        FILE *f = fopen(end_file, "rt");
        if (f)
        {
            char line[200];
            while (fgets(line, sizeof(line), f))
                fputs(line, stdout);
        }
        else
        {
            printf("\n<! default footer --------->\n");
            printf("</BLOCKQUOTE>\n");
            printf("</FONT>\n");
            printf("</BODY>\n");
            printf("<! create '%s' to replace this default footer ---->\n\n", end_file);
        }
        printf("</HTML>\n");
    }
}

void HTMLPage::header(const stdString &text, int level) const
{
    printf("<H%d>%s</H%d>\n", level, text.c_str(), level);
}





