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
#include <iostream>
#include <fstream>
#include "osiTimeHelper.h"

static const char *start_file = "cgi_body_start.txt";
static const char *end_file = "cgi_body_end.txt";

HTMLPage::HTMLPage()
{
    _started = false;
    _glob = true;
    _fill = true;
    _status = false;
    _reduce = true;
    _interpol = 0;
}

void HTMLPage::start()
{
    std::cout << "Content-type: text/html\n"
              << "Pragma: no-cache\n"
              << "\n"
              << "<HTML>\n"
              << "<META Creator: EPICS ChannelArchiver CGIExport>\n"
              << "<HEAD>\n"
              << "<TITLE>" << _title << "</TITLE>\n";

    std::ifstream file;
    file.open(start_file);
#ifdef  HP_UX
    if (! file.fail())
#else
    if (file.is_open())
#endif
    {
        char line[200];
        while (! file.eof())
        {
            file.getline(line, sizeof (line));
            std::cout << line << "\n";
        }
    }
    else
    {
        std::cout << "\n<! default header --------->\n"
                  << "<BODY bgcolor=\"#FFFFFF\">\n"
                  << "<FONT face=\"Comic Sans MS, Arial, Helvetica\">\n"
                  << "<BLOCKQUOTE>\n"
                  << "<! create '" << start_file
                  << "' to replace this default header ---->\n\n";
    }
    _started = true;
}

static void makeSelect(const char *name, int start, int end, int select)
{
    std::cout << "           <SELECT NAME=" << name << " SIZE=1>\n";

    for (int i=start; i<=end; ++i)
    {
        std::cout << "<OPTION";
        if (i==select)
            std::cout << " SELECTED";
        std::cout << "> ";
        if (i<10)
            std::cout << "0";
        std::cout << i << "</OPTION> ";
    }
    std::cout << "\n           </SELECT>\n";
}

// Print the interface stuff
void HTMLPage::interFace() const
{
    size_t i;
    int year, month, day, hour, min, sec;
    unsigned long nano;

    std::cout << "<SCRIPT LANGUAGE=\"JavaScript\">\n";
    std::cout << "function clrNames () { document.f.NAMES.value=\"\"; }\n";
    std::cout << "</SCRIPT>\n";

    std::cout << "<FORM METHOD=\"GET\" ACTION=\"" << _cgi_path << "\" NAME=\"f\">\n";
    std::cout << "  <INPUT TYPE=\"HIDDEN\" NAME=\"DIRECTORY\" VALUE=\""
              << _directory << "\">\n";
    std::cout << "  <TABLE cellpadding=1>\n";
    std::cout << "  <TR valign=top>\n";
    std::cout << "      <TD>Pattern:<br>\n";
    std::cout << "          <INPUT name=GLOB type=checkbox value=ON";
    if (_glob)
        std::cout << " checked=1";
    std::cout <<            ">file glob\n";
    std::cout << "      </TD>\n";
    std::cout << "      <TD colspan=4><INPUT NAME=\"PATTERN\" VALUE=\""
              <<            _pattern << "\" SIZE=40 MAXLENGTH=100>\n";
    std::cout << "          <INPUT TYPE=submit NAME=COMMAND VALUE=LIST>\n";
    std::cout << "          <INPUT TYPE=submit NAME=COMMAND VALUE=INFO>\n";
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";
    std::cout << "  <TR>\n";
    std::cout << "      <TD valign=\"top\">Names:</TD>\n";
    std::cout << "      <TD colspan=4><TEXTAREA NAME=NAMES ROWS=5 COLS=40>";
    for (i=0; i<_names.size(); ++i)
        std::cout << _names[i] << "\n";
    std::cout << "</TEXTAREA>\n";
    std::cout << "<input type=button value=\"CLEAR\" onClick=\"clrNames()\">\n";
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";

    osiTime2vals(_start, year, month, day, hour, min, sec, nano);
    std::cout << "  <TR>\n";
    std::cout << "      <TD>Start:</TD><TD colspan=4>Day (m/d/y)\n";
    makeSelect("STARTMONTH",    1,   12, month);
    makeSelect("STARTDAY"  ,    1,   31, day);
    makeSelect("STARTYEAR" , 1998, 2005, year);
    std::cout << "Time (h:m:s)";
    makeSelect("STARTHOUR" ,    0,   23, hour);
    makeSelect("STARTMINUTE" ,  0,   59, min);
    makeSelect("STARTSECOND" ,  0,   59, sec);
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";

    osiTime2vals(_end, year, month, day, hour, min, sec, nano);
    std::cout << "  <TR>\n";
    std::cout << "      <TD>End:</TD><TD colspan=4>Day (m/d/y)\n";
    makeSelect("ENDMONTH",    1,   12, month);
    makeSelect("ENDDAY"  ,    1,   31, day);
    makeSelect("ENDYEAR" , 1998, 2010, year);
    std::cout << "Time (h:m:s)";
    makeSelect("ENDHOUR" ,    0,   23, hour);
    makeSelect("ENDMINUTE" ,  0,   59, min);
    makeSelect("ENDSECOND" ,  0,   59, sec);
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";

    std::cout << "  <TR>\n";
    std::cout << "      <TD><input TYPE=submit NAME=COMMAND VALUE=GET></TD>\n";
    std::cout << "      <TD><input type=radio name=FORMAT ";
    if (_format == "PLOT")
        std::cout << "checked=1 ";
    std::cout <<            "value=PLOT>Plot</TD>\n";
    std::cout << "      <TD align=right>All Data:</TD>\n";
    std::cout << "      <TD><input name=REDUCE type=checkbox value=ON"
	      << (!_reduce?" checked=1":"") << "> (plot data is reduced otherwise)</TD>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "  </TR>\n";
    std::cout << "  <TR>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "      <TD><input type=radio name=FORMAT ";
    if (_format.empty() || _format == "SPREADSHEET")
        std::cout << "checked=1 ";
    std::cout <<            "value=SPREADSHEET>Spreadsheet</TD>\n";
    std::cout << "      <TD align=right>Status:</TD>\n";
    std::cout << "      <TD><input name=STATUS type=checkbox value=ON"
	      << (_status?" checked=1":"") << "> (show channel status)</TD>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "  </TR>\n";
    std::cout << "  <TR>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "      <TD><input type=radio name=FORMAT ";
    if (_format == "EXCEL")
        std::cout << "checked=1 ";
    std::cout <<           "value=EXCEL>Excel-File</TD>\n";
    std::cout << "      <TD align=right>Fill:</TD>\n";
    std::cout << "      <TD><input name=FILL type=checkbox value=ON"
	      << (_fill?" checked=1":"") << "> (step-func. interpolation)</TD>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "  </TR>\n";
    std::cout << "  <TR>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "      <TD><input type=radio name=FORMAT ";
    if (_format == "MATLAB")
        std::cout << "checked=1 ";
    std::cout <<           "value=MATLAB>Matlab</TD>\n";
    std::cout << "      <TD align=right>Interpolate:</TD>\n";
    std::cout << "      <TD><input maxLength=10 name=INTERPOL size=5 value="
	      << _interpol << "> secs (linear)</TD>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "      <TD></TD>\n";
    std::cout << "  </TR>\n";
    std::cout << "  </TABLE>\n";
    std::cout << "</FORM>\n";
}

HTMLPage::~HTMLPage()
{
    if (_started)
    {
        std::ifstream file;
        file.open(end_file);
#ifdef  HP_UX
        if (! file.fail())
#else
        if (file.is_open())
#endif
        {
            char line[200];
            while (! file.eof())
            {
                file.getline(line, sizeof (line));
                std::cout << line << "\n";
            }
        }
        else
        {
            std::cout << "\n<! default footer --------->\n";
            std::cout << "</BLOCKQUOTE>\n";
            std::cout << "</FONT>\n";
            std::cout << "</BODY>\n";
            std::cout << "<! create '" << end_file << "' to replace this default footer ---->\n\n";
        }
        std::cout << "</HTML>\n";
    }
}

void HTMLPage::header(const stdString &text, int level) const
{
    std::cout << "<H" << level << ">" << text << "</H" << level << ">\n";
}




